#include "pch.h"

#include "core/base.h"
#include "core/log.h"
#include "component/model.h"
#include "component/mesh.h"
#include "component/material.h"
#include "component/animator.h"
#include "utils/ext.h"

using namespace utils;

namespace component {

    // for skeleton animation, each vertex can be influenced by up to 4 bones, this is
    // called the "4 bones per vertex" rule, which exists due to the limitation of old
    // hardware, but it's still widely adopted by engines today as it fits nicely into
    // 4-component vectors (ivec4 and vec4) so is computationally friendly to hardware
    constexpr unsigned int max_vtx_bones = 4;

    static inline glm::mat4 AssimpMat2GLM(const aiMatrix4x4& m) {
        return glm::transpose(glm::make_mat4(&m.a1));  // aiMatrix4x4 is in row-major order so we transpose
    }

    Node::Node(int nid, int pid, const std::string& name) : nid(nid), pid(pid), bid(-1), name(name), alive(0) {
        CORE_ASERT(nid >= 0 && pid < nid, "Parent node is not processed before its children!");
    }

    bool Node::IsBone() const {
        return bid >= 0;
    }

    bool Node::Animated() const {
        return (bid >= 0) && alive;
    }

    Model::Model(const std::string& filepath, Quality quality, bool animate) : Component(), animated(animate) {
        this->vtx_format.reset();
        this->meshes.clear();
        this->materials.clear();

        unsigned int import_options = static_cast<unsigned int>(quality)
            | aiProcess_FlipUVs
            | aiProcess_Triangulate
            | aiProcess_GenSmoothNormals
            | aiProcess_FindInvalidData
            | aiProcess_ValidateDataStructure
            | aiProcess_CalcTangentSpace
            | aiProcess_LimitBoneWeights
            ;

        // for static models, let Assimp pre-transform all vertices for us (will lose the hierarchy)
        if (!animated) {
            import_options |= aiProcess_PreTransformVertices;
        }

        Assimp::Importer importer;
        importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);  // stick to "4 bones per vertex" rule
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, false);

        CORE_TRACE("Start loading model: {0}...", filepath);
        auto start_time = std::chrono::high_resolution_clock::now();
        
        this->ai_root = importer.ReadFile(filepath, import_options);

        if (!ai_root || ai_root->mRootNode == nullptr || ai_root->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            CORE_ERROR("Failed to import model: {0}", filepath);
            CORE_ERROR("Assimp error: {0}", importer.GetErrorString());
            SP_DBG_BREAK();
            return;
        }

        ProcessTree(ai_root->mRootNode, -1);  // recursively process and store the hierarchy info
        ProcessNode(ai_root->mRootNode);      // recursively process every node before return

        if (animated) {
            unsigned int cnt = ranges::count_if(nodes, [](const Node& node) { return node.bid >= 0; });
            CORE_ASERT(n_bones == cnt, "Corrupted data: duplicate or missing bones!");
            CORE_ASERT(n_bones <= 150, "Animation can only support up to 100 bones!");
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> loading_time = end_time - start_time;
        CORE_TRACE("Model import complete! Total loading time: {0:.2f} ms", loading_time.count());
        ai_root = nullptr;

        // the assimp importer will automatically free the aiScene upon function return so we cannot
        // free the root resources manually (in case the second `delete` leads to undefined behavior)
        if constexpr (false) {
            delete ai_root;
            importer.FreeScene();
        }

        CORE_TRACE("Generating model loading report...... (for reference)");
        CORE_TRACE("-----------------------------------------------------");

        CORE_DEBUG("total # of meshes:     {0}",            n_meshes);
        CORE_DEBUG("total # of vertices:   {0:.2f}k ({1})", n_verts * 0.001f, n_verts);
        CORE_DEBUG("total # of triangles:  {0:.2f}k ({1})", n_tris * 0.001f, n_tris);
        CORE_TRACE("-----------------------------------------------------");

        CORE_DEBUG("vertex has position ? [{0}]", vtx_format.test(0) ? "Y" : "N");
        CORE_DEBUG("vertex has normal   ? [{0}]", vtx_format.test(1) ? "Y" : "N");
        CORE_DEBUG("vertex has uv set 1 ? [{0}]", vtx_format.test(2) ? "Y" : "N");
        CORE_DEBUG("vertex has uv set 2 ? [{0}]", vtx_format.test(3) ? "Y" : "N");
        CORE_DEBUG("vertex has tan/btan ? [{0}]", vtx_format.test(4) ? "Y" : "N");
        CORE_TRACE("-----------------------------------------------------");

        std::string all_mtls = "not available";

        if (!materials_cache.empty()) {
            auto it = materials_cache.begin();
            all_mtls = it->first + " (id = " + std::to_string(it->second) + ")";
            std::advance(it, 1);
            while (it != materials_cache.end()) {
                all_mtls += (", " + it->first + " (id = " + std::to_string(it->second) + ")");
                std::advance(it, 1);
            }
        }

        CORE_DEBUG("internal materials: {0}", all_mtls);
        CORE_TRACE("-----------------------------------------------------");
    }

    void Model::ProcessTree(aiNode* ai_node, int parent) {
        // recursively traverse the hierarchy of nodes in a depth-first search (DFS) order and
        // store the hierarchy info of each node into a vector, notice that a parent node will
        // always be processed before its children, this will make the calculation of skeleton
        // animation transforms much easier, moreover, when we retrieve a node from the vector
        // the index into the vector is also the id of that node, as such, finding a node only
        // takes O(1) time, there's no need for binary search, loops or string name comparison

        aiString& ai_name = ai_node->mName;
        auto& node = nodes.emplace_back(n_nodes++, parent, ai_name.length == 0 ? "unnamed" : ai_name.C_Str());
        node.n2p = AssimpMat2GLM(ai_node->mTransformation);
        int next_parent = n_nodes - 1;  // each recursive call has a separate local copy

        for (unsigned int i = 0; i < ai_node->mNumChildren; i++) {
            aiNode* child_node = ai_node->mChildren[i];
            ProcessTree(child_node, next_parent);
        }
    }

    void Model::ProcessNode(aiNode* ai_node) {
        // allocate storage for meshes upfront in every recursion
        meshes.reserve(meshes.size() + ai_node->mNumMeshes);

        // iteratively process every mesh in the current node
        for (unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
            unsigned int mesh_id = ai_node->mMeshes[i];
            aiMesh* ai_mesh = ai_root->mMeshes[mesh_id];
            ProcessMesh(ai_mesh);
        }

        // recursively process all children of the current node
        for (unsigned int i = 0; i < ai_node->mNumChildren; i++) {
            aiNode* child_node = ai_node->mChildren[i];
            ProcessNode(child_node);
        }
    }

    void Model::ProcessMesh(aiMesh* ai_mesh) {
        std::vector<Mesh::Vertex> vertices;
        std::vector<GLuint> indices;
        std::bitset<6> local_format;

        vertices.reserve(ai_mesh->mNumVertices);  // reserve storage upfront
        indices.reserve(ai_mesh->mNumFaces * 3);  // our polygons are always triangles

        // determine local vertex format for this mesh
        local_format.set(0, ai_mesh->HasPositions());
        local_format.set(1, ai_mesh->HasNormals());
        local_format.set(2, ai_mesh->HasTextureCoords(0));
        local_format.set(3, ai_mesh->HasTextureCoords(1) && ai_mesh->GetNumUVChannels() > 1);
        local_format.set(4, ai_mesh->HasTangentsAndBitangents());
        local_format.set(5, ai_mesh->HasTangentsAndBitangents());

        if (n_verts == 0) {
            vtx_format = local_format;
        }

        static bool warned = false;
        if (vtx_format != local_format && !warned) {
            CORE_WARN("Inconsistent vertex format! Some meshes have attributes missing...");
            warned = true;
        }

        vtx_format |= local_format;  // bitwise or on every pair of bits

        // construct mesh vertices, w/o bones data
        for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {
            Mesh::Vertex vertex {};
            vertex.bone_id = ivec4(-1);  // initialize bone id to -1 instead of 0

            if (local_format.test(0)) {
                aiVector3D& ai_position = ai_mesh->mVertices[i];
                vertex.position = glm::vec3(ai_position.x, ai_position.y, ai_position.z);
            }

            if (local_format.test(1)) {
                aiVector3D& ai_normal = ai_mesh->mNormals[i];
                vertex.normal = glm::vec3(ai_normal.x, ai_normal.y, ai_normal.z);
            }

            if (local_format.test(2)) {
                aiVector3D& ai_uv = ai_mesh->mTextureCoords[0][i];  // 1st UV set
                vertex.uv = glm::vec2(ai_uv.x, ai_uv.y);
            }

            if (local_format.test(3)) {
                aiVector3D& ai_uv2 = ai_mesh->mTextureCoords[1][i];  // 2nd UV set
                vertex.uv2 = glm::vec2(ai_uv2.x, ai_uv2.y);
            }

            // tangents and bitangents always come in pairs, if one exists, so does the other
            if (local_format.test(4) && local_format.test(5)) {
                aiVector3D& ai_tangent = ai_mesh->mTangents[i];
                aiVector3D& ai_binormal = ai_mesh->mBitangents[i];
                vertex.tangent  = glm::vec3(ai_tangent.x, ai_tangent.y, ai_tangent.z);
                vertex.binormal = glm::vec3(ai_binormal.x, ai_binormal.y, ai_binormal.z);
            }

            vertices.push_back(vertex);
            n_verts++;
        }

        // construct mesh indices
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace& triangle = ai_mesh->mFaces[i];
            CORE_ASERT(triangle.mNumIndices == 3, "This polygon is not a triangle!");

            // assimp's default winding order agrees with OpenGL (which is CCW)
            indices.push_back(triangle.mIndices[0]);
            indices.push_back(triangle.mIndices[1]);
            indices.push_back(triangle.mIndices[2]);
            n_tris++;
        }

        // fill out the missing bones data in vertices
        if (animated) {
            for (unsigned int i = 0; i < ai_mesh->mNumBones; i++) {
                aiBone* ai_bone = ai_mesh->mBones[i];
                std::string name = ai_bone->mName.C_Str();

                auto it = ranges::find_if(nodes, [&name](const Node& node) { return name == node.name; });
                CORE_ASERT(it != nodes.end(), "Invalid bone, cannot find a match in the nodes hierarchy!");

                // get a non-const ref to update the node (cannot update via const iterator `it`)
                Node& node = nodes[it->nid];  // vector `nodes` is indexed by node id

                // the 1st time we see a bone, give it a bone id and update the m2n matrix, otherwise
                // bid >= 0 means that it's already updated in other meshes so we only need to handle
                // the bone weights for this new mesh. If this is the case, it implies that this bone
                // is going to affect vertices in multiple meshes, it's often a top node in the tree

                if (node.bid < 0) {  // new bone
                    node.m2n = AssimpMat2GLM(ai_bone->mOffsetMatrix);
                    node.bid = n_bones++;
                }

                for (int j = 0; j < ai_bone->mNumWeights; j++) {
                    unsigned int vtx_id = ai_bone->mWeights[j].mVertexId;
                    const float  weight = ai_bone->mWeights[j].mWeight;
                    CORE_ASERT(vtx_id < vertices.size(), "Vertex id out of bound!");

                    auto& vertex = vertices[vtx_id];
                    bool full = glm::all(glm::greaterThanEqual(vertex.bone_id, ivec4(0)));
                    CORE_ASERT(!full, "Found more than 4 bones per vertex, check the import settings!");

                    for (int k = 0; k < max_vtx_bones; k++) {
                        if (vertex.bone_id[k] < 0) {
                            vertex.bone_id[k] = node.bid;
                            vertex.bone_wt[k] = weight;
                            break;  // one bone only sets one slot
                        }
                    }
                }
            }
        }

        auto& mesh = meshes.emplace_back(vertices, indices);  // move construct mesh in-place
        n_meshes++;

        // establish the association between mesh and material
        aiMaterial* ai_material = ai_root->mMaterials[ai_mesh->mMaterialIndex];
        ProcessMaterial(ai_material, mesh);
    }

    void Model::ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh) {
        CORE_ASERT(ai_material != nullptr, "Corrupted assimp data: material is nullptr!");

        aiString name;
        if (ai_material->Get(AI_MATKEY_NAME, name) != AI_SUCCESS) {
            CORE_ERROR("Unable to load mesh's material (VAO = {0})...", mesh.vao->ID());
            return;
        }

        std::string matkey { name.C_Str() };

        // check if the matkey already exists in local cache
        if (materials_cache.find(matkey) != materials_cache.end()) {
            GLuint matid = materials_cache[matkey];
            mesh.SetMaterialID(matid);  // reuse the previous matid since the material is shared
            return;
        }

        // new material, store the matkey in local cache
        GLuint matid = mesh.material_id;
        materials_cache[matkey] = matid;
    }

    Material& Model::SetMaterial(const std::string& matkey, asset_ref<Material>&& material) {
        CORE_ASERT(materials_cache.count(matkey) > 0, "Invalid material key: {0}", matkey);

        // notice that we expect the material param to be a temporary rvalue that is a copy of
        // the original asset_ref in the asset manager, hence we can directly move it in place

        GLuint matid = materials_cache[matkey];
        materials.insert_or_assign(matid, std::move(material));
        return materials.at(matid);
    }

    void Model::AttachMotion(const std::string& filepath) {
        if (!animated) {
            CORE_ERROR("Cannot attach animation to the model, model must be animated...");
        }

        const unsigned int import_options = 0
            | aiProcess_FlipUVs
            | aiProcess_Triangulate
            | aiProcess_GenSmoothNormals
            | aiProcess_FindInvalidData
            | aiProcess_ValidateDataStructure
            | aiProcess_CalcTangentSpace
            | aiProcess_LimitBoneWeights
            // | aiProcess_PreTransformVertices  // this flag must be disabled to load animation
            ;

        Assimp::Importer importer;
        importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);

        CORE_TRACE("Start loading animation from: {0}...", filepath);
        const aiScene* scene = importer.ReadFile(filepath, import_options);

        // note that we don't need to check `scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE`
        // here because the file can contain animations ONLY (without vertices, meshes)
        // in which case the scene will be incomplete but the animation is still valid

        if (!scene || scene->mRootNode == nullptr) {
            CORE_ERROR("Failed to import animation: {0}", filepath);
            CORE_ERROR("Assimp error: {0}", importer.GetErrorString());
            SP_DBG_BREAK();
            return;
        }

        animation = std::make_unique<Animation>(scene, this);
    }

}
