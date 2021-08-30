#pragma once

#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/log.h"
#include "components/component.h"
// #include "components/mesh.h"

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace components {

    // we have to impose some restrictions on the model's material format, other material formats won't be supported
    // 1. a valid material can either use properties only, or textures only, but not both. e.g. if a material has a metallic texture map, we would ignore its float metallic value (if there's any), if a material has a diffuse map, we would discard its vector3 albedo value (if there's any), etc.
    // 2. all meshes in the model must be consistent, in the sense that their materials must have the same structure. e.g. if one mesh's material has textures, other materials must also have textures of the same types, if one mesh's material uses properties only, other materials must also have properties of the same types (their texture maps won't be processed, if any). In particular, if one mesh does not have a aiMaterial* pointer, the entire model is assumed to be material-free.
    // 3. we assume there's only one texture map of each type. e.g. if a material has multiple albedo maps or specular maps, only the first one will be loaded (others are discarded)
    // 4. we only support 2D textures, so each type of texture map must use the target `GL_TEXTURE_2D`

    // forward declaration
    class Mesh;
    class Texture;

    // import quality preset
    enum class Quality : uint32_t {
        Low    = aiProcessPreset_TargetRealtime_Fast,
        Medium = aiProcessPreset_TargetRealtime_Quality,
        High   = aiProcessPreset_TargetRealtime_MaxQuality
    };

    // material format
    enum class MTLFormat : uint8_t {
        Unknown,
        None,      // raw mesh data without materials
        Property,  // material uses value type properties (float metallic, vec3 albedo...), no textures
        Texture,   // material uses texture maps
        Color,     // material uses vertex colors (legacy stuff, rarely used today)
        Embedded   // textures are embedded in the material, no separate texture files
    };

    // supported property variants
    using property_variant = std::variant<
        int,        // index 0
        bool,       // index 1
        float,      // index 2
        glm::vec2,  // index 3
        glm::vec3,  // index 4
        glm::vec4,  // index 5
        glm::mat2,  // index 6
        glm::mat3,  // index 7
        glm::mat4   // index 8
    >;

    static_assert(std::variant_size_v<property_variant> == 9);

    class Model : public Component {
      private:
        const aiScene* ai_root = nullptr;

        unsigned int meshes_count = 0;
        unsigned int vertices_count = 0;
        unsigned int materials_count = 0;

        std::set<std::string> textures_set;
        std::set<std::string> properties_set;
        std::unordered_map<std::string, GLuint> materials_cache;

        void ProcessNode(aiNode* ai_node);
        void ProcessMesh(aiMesh* ai_mesh);
        void ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh);

      public:
        Quality quality;
        std::string directory;

        std::bitset<6> vtx_format;
        MTLFormat mtl_format = MTLFormat::Unknown;

        std::vector<Mesh> meshes;
        std::unordered_map<GLuint, std::vector<property_variant>> properties;
        std::unordered_map<GLuint, std::vector<asset_ref<Texture>>> textures;

      public:
        Model(const std::string& filepath, Quality quality = Quality::Low);
        ~Model() {}

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        Model(Model&& other) = default;
        Model& operator=(Model&& other) = default;

        void Report();
    };

}

// about the MTL file format contents
// https://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library
// https://www.loc.gov/preservation/digital/formats/fdd/fdd000508.shtml


