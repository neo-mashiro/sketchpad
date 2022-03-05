#include "pch.h"

#include "core/base.h"
#include "core/log.h"
#include "component/model.h"
#include "component/mesh.h"
#include "component/material.h"

namespace component {

    Model::Model(const std::string& filepath, Quality quality) : Component() {
        this->vtx_format.reset();
        this->meshes.clear();
        this->materials.clear();

        CORE_TRACE("Start loading model: {0}...", filepath);
        auto start_time = std::chrono::high_resolution_clock::now();

        static unsigned int import_options = static_cast<unsigned int>(quality)
            | aiProcess_FlipUVs
            | aiProcess_Triangulate
            | aiProcess_PreTransformVertices
            | aiProcess_GenSmoothNormals
            | aiProcess_FindInvalidData
            | aiProcess_ValidateDataStructure
            | aiProcess_CalcTangentSpace
            ;

        Assimp::Importer importer;
        this->ai_root = importer.ReadFile(filepath, import_options);

        if (!ai_root || ai_root->mRootNode == nullptr || ai_root->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            CORE_ERROR("Failed to import model: {0}", filepath);
            CORE_ERROR("Assimp error: {0}", importer.GetErrorString());
            SP_DBG_BREAK();
            return;
        }

        ProcessNode(ai_root->mRootNode);  // recursively process every node before return

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> loading_time = end_time - start_time;
        CORE_TRACE("Model import complete! Total loading time: {0:.2f} ms", loading_time.count());

        // the assimp importer will automatically free the aiScene upon function return so we shouldn't
        // free the root resources manually (in case the second `delete` leads to undefined behavior)
        if constexpr (false) {
            ai_root = nullptr;
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

    void Model::ProcessNode(aiNode* ai_node) {
        // allocate storage upfront in every recursion
        meshes.reserve(meshes.size() + ai_node->mNumMeshes);

        // iteratively process every mesh in the current node
        for (unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
            unsigned int mesh_index = ai_node->mMeshes[i];
            aiMesh* ai_mesh = ai_root->mMeshes[mesh_index];
            ProcessMesh(ai_mesh);
        }

        // recursively process all children nodes of the current node
        for (unsigned int i = 0; i < ai_node->mNumChildren; i++) {
            aiNode* child_node = ai_node->mChildren[i];
            ProcessNode(child_node);
        }
    }

    void Model::ProcessMesh(aiMesh* ai_mesh) {
        // determine vertex format in the first run (from the first mesh)
        if (n_verts == 0) {
            vtx_format.set(0, ai_mesh->HasPositions());
            vtx_format.set(1, ai_mesh->HasNormals());
            vtx_format.set(2, ai_mesh->HasTextureCoords(0));
            // vtx_format.set(3, ai_mesh->HasTextureCoords(1) && ai_mesh->GetNumUVChannels() > 1);
            vtx_format.set(4, ai_mesh->HasTangentsAndBitangents());
            vtx_format.set(5, ai_mesh->HasTangentsAndBitangents());
        }

        // for subsequent meshes, check if the vertex format stays the same
        bool condition =
            vtx_format.test(0) == ai_mesh->HasPositions() &&
            vtx_format.test(1) == ai_mesh->HasNormals() &&
            vtx_format.test(2) == ai_mesh->HasTextureCoords(0) &&
            // vtx_format.test(3) == (ai_mesh->HasTextureCoords(1) && ai_mesh->GetNumUVChannels() > 1) &&
            vtx_format.test(4) == ai_mesh->HasTangentsAndBitangents() &&
            vtx_format.test(5) == ai_mesh->HasTangentsAndBitangents();

        CORE_ASERT(condition, "Inconsistent vertex format! Mesh data is corrupted...");

        // create local vectors to hold vertices and indices, reserve storage upfront
        std::vector<Mesh::Vertex> vertices;
        std::vector<GLuint> indices;
        vertices.reserve(ai_mesh->mNumVertices);
        indices.reserve(ai_mesh->mNumFaces * 3);  // our polygons are always triangles

        // construct vertices data
        for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {
            Mesh::Vertex vertex {};

            if (vtx_format.test(0)) {
                aiVector3D& ai_position = ai_mesh->mVertices[i];
                vertex.position = glm::vec3(ai_position.x, ai_position.y, ai_position.z);
            }

            if (vtx_format.test(1)) {
                aiVector3D& ai_normal = ai_mesh->mNormals[i];
                vertex.normal = glm::vec3(ai_normal.x, ai_normal.y, ai_normal.z);
            }

            if (vtx_format.test(2)) {
                aiVector3D& ai_uv = ai_mesh->mTextureCoords[0][i];  // 1st UV set
                vertex.uv = glm::vec2(ai_uv.x, ai_uv.y);
            }

            if (vtx_format.test(3)) {
                aiVector3D& ai_uv2 = ai_mesh->mTextureCoords[1][i];  // 2nd UV set
                vertex.uv2 = glm::vec2(ai_uv2.x, ai_uv2.y);
            }

            // tangents and bitangents always come in pairs, if one exists, so does the other
            if (vtx_format.test(4) && vtx_format.test(5)) {
                aiVector3D& ai_tangent = ai_mesh->mTangents[i];
                aiVector3D& ai_binormal = ai_mesh->mBitangents[i];
                vertex.tangent  = glm::vec3(ai_tangent.x, ai_tangent.y, ai_tangent.z);
                vertex.binormal = glm::vec3(ai_binormal.x, ai_binormal.y, ai_binormal.z);
            }

            vertices.push_back(vertex);
            n_verts++;
        }

        // construct indices data
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace& triangle = ai_mesh->mFaces[i];
            CORE_ASERT(triangle.mNumIndices == 3, "This polygon is not a triangle!");

            // assimp's default winding order agrees with OpenGL (which is CCW)
            indices.push_back(triangle.mIndices[0]);
            indices.push_back(triangle.mIndices[1]);
            indices.push_back(triangle.mIndices[2]);
            n_tris++;
        }

        auto& mesh = meshes.emplace_back(vertices, indices);  // move construct mesh in-place
        n_meshes++;

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

}
