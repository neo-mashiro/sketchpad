#include "pch.h"

#include "core/log.h"
#include "components/texture.h"
#include "components/mesh.h"
#include "components/model.h"
#include "utils/path.h"

namespace components {

    // list of currently supported texture types
    static const std::vector<std::string> texture_types {
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

    // list of currently supported properties
    static const std::vector<std::string> property_keys {
        "$clr.ambient",
        "$clr.diffuse",
        "$clr.specular",
        "$mat.shininess",
        "$mat.opacity",
        "$mat.reflectivity"
    };

    Model::Model(const std::string& filepath, Quality quality) : quality(quality) {
        directory = filepath.substr(0, filepath.find_last_of('/'));
        vtx_format.reset();

        auto start_time = std::chrono::high_resolution_clock::now();

        Assimp::Importer importer;
        ai_root = importer.ReadFile(filepath, static_cast<unsigned int>(quality));

        if (!ai_root || ai_root->mRootNode == nullptr || ai_root->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            CORE_ERROR("Failed to import model: {0}", filepath);
            CORE_ERROR("Assimp error: {0}", importer.GetErrorString());
            return;
        }

        std::string quality_level = (quality == Quality::Low) ? "Low"
            : (quality == Quality::Medium) ? "Medium" : "High";

        CORE_TRACE("Quality level is set to \"{0}\"", quality_level);
        CORE_TRACE("Start loading model: {0}...", filepath);

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

        CORE_TRACE("Model import complete! Total loading time: {0} ms", loading_time.count());

        Report();
    }

    void Model::Report() {
        CORE_TRACE("------------------------MODEL LOADING REPORT-------------------------");

        CORE_TRACE("Number of meshes:    {0}"    , meshes_count);
        CORE_TRACE("Number of vertices:  {0:.2f}k", static_cast<float>(vertices_count * 0.001f));
        CORE_TRACE("Number of materials: {0:.2f}k", static_cast<float>(materials_count * 0.001f));

        // report vertices metadata
        CORE_TRACE("VTX-format: position  [{0}]", vtx_format.test(0) ? "Y" : "N");
        CORE_TRACE("VTX-format: normal    [{0}]", vtx_format.test(1) ? "Y" : "N");
        CORE_TRACE("VTX-format: UV        [{0}]", vtx_format.test(2) ? "Y" : "N");
        CORE_TRACE("VTX-format: UV2       [{0}]", vtx_format.test(3) ? "Y" : "N");
        CORE_TRACE("VTX-format: tangent   [{0}]", vtx_format.test(4) ? "Y" : "N");
        CORE_TRACE("VTX-format: bitangent [{0}]", vtx_format.test(5) ? "Y" : "N");

        // report materials metadata
        switch (mtl_format) {
            case MTLFormat::None:     CORE_TRACE("MTL-format: no materials.");     break;
            case MTLFormat::Property: CORE_TRACE("MTL-format: using properties."); break;
            case MTLFormat::Texture:  CORE_TRACE("MTL-format: using textures.");   break;
            default:                  CORE_ASERT(false, "Internal error detected, check your Model class!");
        }

        // report properties in the material
        if (mtl_format == MTLFormat::Property) {
            auto it = properties_set.begin();
            std::string all_props = *it;

            while (it != properties_set.end()) {
                it++;
                all_props += (", " + (*it).substr(5));
            }

            CORE_TRACE("MTL properties: {0}", all_props);
        }

        // report textures in the material
        if (mtl_format == MTLFormat::Texture) {
            auto it = textures_set.begin();
            std::string all_textures = *it;

            while (it != textures_set.end()) {
                it++;
                all_textures += (", " + *it);
            }

            CORE_TRACE("MTL textures: {0}", all_textures);
        }

        CORE_TRACE("----------------------------END OF REPORT----------------------------");
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
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        // construct vertices data
        for (unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {
            Vertex vertex {};

            if (ai_mesh->HasPositions()) {
                aiVector3D& ai_position = ai_mesh->mVertices[i];
                vertex.position = { ai_position.x, ai_position.y, ai_position.z };
            }

            if (ai_mesh->HasNormals()) {
                aiVector3D& ai_normal = ai_mesh->mNormals[i];
                vertex.normal = { ai_normal.x, ai_normal.y, ai_normal.z };
            }

            // assimp allows a model to have up to 8 UV sets, but we only need the first set.
            // for more info, see multiple UV sets and multi-texturing in Maya and Unreal.
            if (ai_mesh->HasTextureCoords(0)) {
                aiVector3D& ai_uv = ai_mesh->mTextureCoords[0][i];
                vertex.uv = { ai_uv.x, ai_uv.y };
            }

            if (ai_mesh->GetNumUVChannels() > 1 && ai_mesh->HasTextureCoords(1)) {
                aiVector3D& ai_uv2 = ai_mesh->mTextureCoords[1][i];
                vertex.uv2 = { ai_uv2.x, ai_uv2.y };
            }

            if (ai_mesh->HasTangentsAndBitangents()) {
                aiVector3D& ai_tangent = ai_mesh->mTangents[i];
                aiVector3D& ai_bitangent = ai_mesh->mBitangents[i];
                vertex.tangent   = { ai_tangent.x, ai_tangent.y, ai_tangent.z };
                vertex.bitangent = { ai_bitangent.x, ai_bitangent.y, ai_bitangent.z };
            }

            vertices.push_back(vertex);
            vertices_count++;
        }

        // construct indices data
        for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++) {
            aiFace& triangle = ai_mesh->mFaces[i];

            // assimp's default winding order agrees with OpenGL (which is CCW)
            for (unsigned int j = 0; j < triangle.mNumIndices; j++) {
                indices.push_back(triangle.mIndices[j]);
            }
        }

        auto& mesh = meshes.emplace_back(vertices, indices);  // construct mesh in-place
        meshes_count++;
        // after this operation, vertices and indices will become empty vectors (moved)

        aiMaterial* ai_material = ai_root->mMaterials[ai_mesh->mMaterialIndex];
        ProcessMaterial(ai_material, mesh);
    }

    void Model::ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh) {
        if (mtl_format == MTLFormat::None) {
            return;
        }

        if (ai_material == nullptr) {
            mtl_format = MTLFormat::None;
            return;
        }

        aiString buffer;
        if (ai_material->Get(AI_MATKEY_NAME, buffer) != AI_SUCCESS) {
            CORE_ERROR("Unable to load mesh's material (VAO = {0})...", mesh.VAO);
            __debugbreak();
        }

        std::string material { buffer.C_Str() };

        // check if the material exists in local cache
        if (materials_cache.find(material) != materials_cache.end()) {
            GLuint material_id = materials_cache[material];
            mesh.SetMaterialID(material_id);
            CORE_INFO("Reusing previously loaded material (id = {0})", material_id);
            return;
        }

        // new material, load for the first time
        GLuint material_id = mesh.material_id;
        materials_cache[material] = material_id;
        CORE_INFO("Loading material: {0} (id = {1})", material, material_id);

        // Mtl format: using textures
        if (mtl_format == MTLFormat::Unknown || mtl_format == MTLFormat::Texture) {
            // loop through every type of texture map
            for (unsigned int i = 1; i < texture_types.size(); i++) {
                std::string texture_name = texture_types[i];

                if (ai_material->GetTextureCount(static_cast<aiTextureType>(i)) == 0) {
                    CORE_ASERT(!textures_set.count(texture_name), "Missing texture {0}!", texture_name);
                    continue;
                }

                // for each type of texture map, we only load the first one (index = 0)
                aiString filename;
                ai_material->GetTexture(static_cast<aiTextureType>(i), 0, &filename);
                std::string texture_path = directory + '/' + std::string(filename.C_Str());

                if (ai_root->GetEmbeddedTexture(filename.C_Str()) != nullptr) {
                    CORE_ERROR("Models with embedded textures are not supported!");
                    mtl_format = MTLFormat::Embedded;
                    break;
                }

                // if we are here, the material has a regular texture map of this type
                if (materials_count == 0) {
                    textures_set.insert(texture_name);
                }

                CORE_ASERT(textures_set.count(texture_name) == 1, "Inconsistent material format!");
                textures[material_id].push_back(LoadAsset<Texture>(GL_TEXTURE_2D, texture_path));
            }

            if (!textures_set.empty()) {
                if (materials_count == 0) {
                    mtl_format = MTLFormat::Texture;
                }
                materials_count++;
                return;
            }
        }

        // Mtl format: using properties
        if (mtl_format == MTLFormat::Unknown || mtl_format == MTLFormat::Property) {
            aiColor3D c_buff { 0.0f };
            float f_buff { 0.0f };
            std::vector<property_variant>& props = properties[material_id];

            // loop through every type of property: "$clr.specular", "$mat.opacity"....
            for (auto& prop : property_keys) {
                std::string prop_type = prop.substr(1, 4);  // "clr" or "mat"
                std::string prop_name = prop.substr(5);     // "diffuse", "opacity"....

                // color3 property
                if (prop_type == "clr") {
                    if (ai_material->Get(prop.c_str(), 0, 0, c_buff) == AI_SUCCESS) {
                        if (materials_count == 0) {
                            properties_set.insert(prop_name);
                        }
                        CORE_ASERT(properties_set.count(prop_name), "Inconsistent material format!");
                        props.emplace_back(std::in_place_index<4>, c_buff.r, c_buff.g, c_buff.b);
                    }
                    else {
                        CORE_ASERT(!properties_set.count(prop_name), "Missing property {0}!", prop_name);
                    }
                }

                // float property
                else if (prop_type == "mat") {
                    if (ai_material->Get(prop.c_str(), 0, 0, f_buff) == AI_SUCCESS) {
                        if (materials_count == 0) {
                            properties_set.insert(prop_name);
                        }
                        CORE_ASERT(properties_set.count(prop_name), "Inconsistent material format!");
                        props.emplace_back(std::in_place_index<2>, f_buff);
                    }
                    else {
                        CORE_ASERT(!properties_set.count(prop_name), "Missing property {0}!", prop_name);
                    }
                }
            }

            if (!properties_set.empty()) {
                if (materials_count == 0) {
                    mtl_format = MTLFormat::Property;
                }
                materials_count++;
                return;
            }
        }

        // if we are here, this material is completely empty (no properties, no textures, perhaps
        // there are embedded textures but we won't support), which means that we only have raw
        // meshes data available, so we should treat the model as if it was a mesh component, and
        // then add a material component with a shader of any flavor we like.
        mtl_format = MTLFormat::None;
    }

}
