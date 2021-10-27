#include "pch.h"

#include "core/log.h"
#include "buffer/vao.h"
#include "buffer/texture.h"
#include "components/mesh.h"
#include "components/model.h"
#include "utils/filesystem.h"

namespace components {

    Model::Model(const std::string& filepath, Quality quality) : quality(quality) {
        directory = filepath.substr(0, filepath.rfind("\\")) + "\\";
        vtx_format.reset();

        auto start_time = std::chrono::high_resolution_clock::now();

        static unsigned int core_steps =
            aiProcess_FlipUVs |
            aiProcess_Triangulate |
            aiProcess_PreTransformVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_FindInvalidData |
            aiProcess_ValidateDataStructure |
            aiProcess_CalcTangentSpace;

        Assimp::Importer importer;
        ai_root = importer.ReadFile(filepath, static_cast<unsigned int>(quality) | core_steps);

        if (!ai_root || ai_root->mRootNode == nullptr || ai_root->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            CORE_ERROR("Failed to import model: {0}", filepath);
            CORE_ERROR("Assimp error: {0}", importer.GetErrorString());
            return;
        }

        std::string quality_level;

        switch (quality) {
            case Quality::Auto:   quality_level = "Auto";   break;
            case Quality::Low:    quality_level = "Low";    break;
            case Quality::Medium: quality_level = "Medium"; break;
            case Quality::High:   quality_level = "High";   break;
        }

        CORE_TRACE("Start loading model: {0}...", filepath);
        CORE_TRACE("Quality level is set to \"{0}\"", quality_level);

        ProcessNode(ai_root->mRootNode);  // recursively process every node before return

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> loading_time = end_time - start_time;

        // the assimp importer will automatically free the aiScene upon function return so we shouldn't
        // free the root resources manually (in case the second `delete` leads to undefined behavior)
        if (false) {
            ai_root = nullptr;
            delete ai_root;
            importer.FreeScene();
        }

        CORE_TRACE("Model import complete! Total loading time: {0:.2f} ms", loading_time.count());

        Report();
    }

    void Model::Report() {
        CORE_TRACE("Printing model loading report...... (for reference only)");

        CORE_TRACE("Number of meshes:    {0}", n_meshes);
        CORE_TRACE("Number of vertices:  {0:.2f}k ({1})", n_verts * 0.001f, n_verts);
        CORE_TRACE("Number of materials: {0:.2f}k ({1})", n_materials * 0.001f, n_materials);

        // report vertices metadata
        CORE_TRACE("VTX-format: position  [{0}]", vtx_format.test(0) ? "Y" : "N");
        CORE_TRACE("VTX-format: normal    [{0}]", vtx_format.test(1) ? "Y" : "N");
        CORE_TRACE("VTX-format: UV        [{0}]", vtx_format.test(2) ? "Y" : "N");
        CORE_TRACE("VTX-format: UV2       [{0}]", vtx_format.test(3) ? "Y" : "N");
        CORE_TRACE("VTX-format: tangent   [{0}]", vtx_format.test(4) ? "Y" : "N");
        CORE_TRACE("VTX-format: bitangent [{0}]", vtx_format.test(5) ? "Y" : "N");

        // report materials names in the model
        auto it = materials_cache.begin();
        std::string all_mtls = it->first + " (id = " + std::to_string(it->second) + ")";
        std::advance(it, 1);

        while (it != materials_cache.end()) {
            all_mtls += (", " + it->first + " (id = " + std::to_string(it->second) + ")");
            std::advance(it, 1);
        }

        CORE_TRACE("Model internal materials: {0}", all_mtls);

        // report material details
        for (auto& mtl : materials_cache) {
            std::string mtl_name = mtl.first;
            GLuint mtl_id = mtl.second;

            // properties
            auto pit = property_names[mtl_id].begin();
            std::string all_props = mtl_name + " properties: ";

            if (pit != property_names[mtl_id].end()) {
                all_props += *pit;
                std::advance(pit, 1);

                while (pit != property_names[mtl_id].end()) {
                    all_props += (", " + *pit);
                    std::advance(pit, 1);
                }
            }

            CORE_TRACE(all_props);

            // textures
            auto tit = texture_names[mtl_id].begin();
            std::string all_textures = mtl_name + " textures: ";

            if (tit != texture_names[mtl_id].end() && imported_materials.count(mtl_id) == 0) {
                all_textures += *tit;
                std::advance(tit, 1);

                while (tit != texture_names[mtl_id].end()) {
                    all_textures += (", " + *tit);
                    std::advance(tit, 1);
                }
            }
            else if (imported_materials.count(mtl_id) == 1) {
                all_textures += "manually imported by the user";
            }

            CORE_TRACE(all_textures);
        }
    }

    void Model::Import(const std::string& material_name, std::vector<buffer_ref<Texture>>& tex_refs) {
        if (materials_cache.count(material_name) == 0) {
            CORE_ERROR("Invalid material name: {0}", material_name);
            return;
        }

        GLuint material_id = materials_cache[material_name];

        textures[material_id].clear();
        textures[material_id] = std::move(tex_refs);
        imported_materials.insert(material_id);
    }

    void Model::ProcessNode(aiNode* ai_node) {
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
            vtx_format.set(3, ai_mesh->HasTextureCoords(1) && ai_mesh->GetNumUVChannels() > 1);
            vtx_format.set(4, ai_mesh->HasTangentsAndBitangents());
            vtx_format.set(5, ai_mesh->HasTangentsAndBitangents());
        }

        // for subsequent meshes, check if the vertex format stays the same
        bool condition =
            vtx_format.test(0) == ai_mesh->HasPositions() &&
            vtx_format.test(1) == ai_mesh->HasNormals() &&
            vtx_format.test(2) == ai_mesh->HasTextureCoords(0) &&
            vtx_format.test(3) == (ai_mesh->HasTextureCoords(1) && ai_mesh->GetNumUVChannels() > 1) &&
            vtx_format.test(4) == ai_mesh->HasTangentsAndBitangents() &&
            vtx_format.test(5) == ai_mesh->HasTangentsAndBitangents();

        CORE_ASERT(condition, "Inconsistent vertex format! Mesh data is corrupted...");

        // allocate storage for vertices and indices upfront (std::vector dynamic resizing is expensive)
        std::vector<Mesh::Vertex> vertices;
        std::vector<GLuint> indices;
        vertices.reserve(ai_mesh->mNumVertices);
        indices.reserve(ai_mesh->mNumFaces * 3);  // our polygons are always triangles

        // construct vertices data
        for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {
            Mesh::Vertex vertex {};

            if (vtx_format.test(0)) {
                aiVector3D& ai_position = ai_mesh->mVertices[i];
                vertex.position = { ai_position.x, ai_position.y, ai_position.z };
            }

            if (vtx_format.test(1)) {
                aiVector3D& ai_normal = ai_mesh->mNormals[i];
                vertex.normal = { ai_normal.x, ai_normal.y, ai_normal.z };
            }

            if (vtx_format.test(2)) {
                // assimp allows a model to have up to 8 UV sets, but we only need the first two.
                // for more info, see multiple UV sets and multi-texturing in Maya and Unreal.
                aiVector3D& ai_uv = ai_mesh->mTextureCoords[0][i];
                vertex.uv = { ai_uv.x, ai_uv.y };
            }

            if (vtx_format.test(3)) {
                // the second UV set is primarily used for lightmaps in this demo
                aiVector3D& ai_uv2 = ai_mesh->mTextureCoords[1][i];
                vertex.uv2 = { ai_uv2.x, ai_uv2.y };
            }

            if (vtx_format.test(4)) {
                // tangents and bitangents always come in pairs, if one exists, so does the other
                aiVector3D& ai_tangent = ai_mesh->mTangents[i];
                aiVector3D& ai_bitangent = ai_mesh->mBitangents[i];
                vertex.tangent   = { ai_tangent.x, ai_tangent.y, ai_tangent.z };
                vertex.bitangent = { ai_bitangent.x, ai_bitangent.y, ai_bitangent.z };
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
        }

        auto& mesh = meshes.emplace_back(vertices, indices);  // move construct mesh in-place
        n_meshes++;

        aiMaterial* ai_material = ai_root->mMaterials[ai_mesh->mMaterialIndex];
        ProcessMaterial(ai_material, mesh);
    }

    void Model::ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh) {
        CORE_ASERT(ai_material != nullptr, "Corrupted assimp data: material is nullptr!");

        static const std::vector<std::string> supported_textures {
            "None",                  // index 0  (index maps to aiTextureType, don't modify)
            "Diffuse",               // index 1  (aka albedo map, non-PBR)
            "Specular",              // index 2  (aka metallic map, non-PBR)
            "Ambient",               // index 3
            "Emissive",              // index 4  (emission map)
            "Height",                // index 5  (normal map if only has this one, else heightmap / bump map?)
            "Normals",               // index 6  (normal map, tangent-space?)
            "Glossiness",            // index 7  (aka roughness map, non-PBR)
            "Opacity",               // index 8
            "Displacement",          // index 9
            "Lightmap",              // index 10 (use the second UV channel, UV2)
            "Reflection",            // index 11 (index placeholder, don't use even if it's present)
            "PBR_Base_Color",        // index 12 (aka albedo map, PBR industry standard)
            "PBR_Normal_Camera",     // index 13 (PBR industry standard)
            "PBR_Emission_Color",    // index 14 (PBR industry standard)
            "PBR_Metalness",         // index 15 (PBR industry standard)
            "PBR_Diffuse_Roughness", // index 16 (PBR industry standard)
            "PBR_Ambient_Occlusion"  // index 17 (PBR industry standard)
        };

        static const std::vector<std::string> supported_properties {
            "$clr.ambient",
            "$clr.diffuse",
            "$clr.specular",
            "$mat.shininess",
            "$mat.opacity",
            "$mat.reflectivity"
        };

        aiString buffer;
        if (ai_material->Get(AI_MATKEY_NAME, buffer) != AI_SUCCESS) {
            CORE_ERROR("Unable to load mesh's material (VAO = {0})...", mesh.GetVAO()->GetID());
            __debugbreak();
        }

        std::string material { buffer.C_Str() };

        // check if the material exists in local cache
        if (materials_cache.find(material) != materials_cache.end()) {
            GLuint material_id = materials_cache[material];
            mesh.SetMaterialID(material_id);  // reuse previously loaded material
            return;
        }

        // new material, load for the first time
        GLuint material_id = mesh.material_id;
        materials_cache[material] = material_id;

        CORE_TRACE("Loading material: {0} (id = {1})", material, material_id);

        // find textures in the material
        auto& txtr_names = texture_names[material_id];
        auto& txtr_values = textures[material_id];

        for (unsigned int i = 1; i < supported_textures.size(); i++) {
            std::string texture_name = supported_textures[i];

            if (ai_material->GetTextureCount(static_cast<aiTextureType>(i)) == 0) {
                continue;
            }

            // for each type of texture map, we only load the first one (index = 0)
            aiString filename;
            ai_material->GetTexture(static_cast<aiTextureType>(i), 0, &filename);
            std::string texture_path = directory + std::string(filename.C_Str());

            if (ai_root->GetEmbeddedTexture(filename.C_Str()) != nullptr) {
                CORE_ERROR("Embedded textures are not supported! {0} ignored...", texture_name);
                continue;
            }

            // if we are here, the material has a regular texture map of this type
            txtr_names.push_back(texture_name);
            txtr_values.push_back(LoadAsset<Texture>(texture_path));
        }

        // find properties in the material
        auto& prop_names = property_names[material_id];
        auto& prop_values = properties[material_id];

        aiColor3D c_buff { 0.0f };
        float f_buff { 0.0f };

        // loop through every type of property: "$clr.specular", "$mat.opacity"....
        for (auto& prop : supported_properties) {
            std::string prop_type = prop.substr(1, 3);  // "clr" or "mat"
            std::string prop_name = prop.substr(5);     // "diffuse", "opacity"....

            // color3 property
            if (prop_type == "clr" && ai_material->Get(prop.c_str(), 0, 0, c_buff) == AI_SUCCESS) {
                prop_names.push_back(prop_name);
                prop_values.emplace_back(std::in_place_index<4>, c_buff.r, c_buff.g, c_buff.b);
            }

            // float property
            else if (prop_type == "mat" && ai_material->Get(prop.c_str(), 0, 0, f_buff) == AI_SUCCESS) {
                prop_names.push_back(prop_name);
                prop_values.emplace_back(std::in_place_index<2>, f_buff);
            }
        }

        if (!txtr_names.empty() || !prop_names.empty()) {
            n_materials++;
        }

        // if we are here, this material is completely empty (no properties, no textures, perhaps
        // there are embedded textures but we won't support), which means that we only have raw
        // meshes data available, so we should treat the model as if it was a mesh component, and
        // then add a material component with a shader of any flavor we like.
    }

}
