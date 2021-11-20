#pragma once

#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "buffer/texture.h"
#include "components/component.h"

namespace components {

    /* due to the various formats and conventions of using materials, model loading is a hard topic.
       in order to observe consistent behavior for most models, we have to impose some restrictions
       on the model's material format that we are going to support, here's our plan:
       
       in the high-level picture, a model typically consists of multiple meshes, each mesh may have
       either zero or one material. A material can be shared by multiple meshes, so the number of
       materials must be no greater than the number of meshes. At the end of loading, no matter how
       many meshes and materials we have, the entity who owns this model will only have 1 `Material`
       component being assigned, which is reused for every mesh in the model, in other words, we
       need to write a shader that applies to all meshes, this implies that all meshes should have
       a similar material structure, or you know the materials of each mesh when writing the shader.
       
       the way we apply the shader to all meshes is by introducing a uniform `material_id`, which
       is an integer that controls code branching in GLSL. During the rendering stage, the renderer
       will iterate over every mesh in the model, find its material id and update the uniform, bind
       the properties and textures of that id, and then draw the mesh. Again, the material component
       and shader will be reused and updated for all meshes. Without a dedicated asset manager that
       claims the responsibility of managing all forms of assets, this is probably the best design.
       
       let's imagine the alternatives, what if each mesh uses its own shader and material component?
       well, while it does allow for the maximum flexibity of each mesh, this class would have too
       much burden and clutter, not to mention the bandwidth cost of repeated context switching.
       what if we don't put any restrictions? in that case, it would be very hard to write a general
       shader that can handle every possible combination of materials for every mesh. For GLSL, this
       means that we would need a whole bunch of #ifdef and #elif directives in the shader.
       
       so, here is the list of assumptions/guidelines we are going to make/follow:
       
       [1] a valid material may contain properties, or textures, or both, or neither of them, the
           loading report will list the details of each material's content. In most cases, the
           shader should use only one of them, e.g. either a metallic map, or a vector3 albedo, but
           not both. That said, if you know what materials each mesh has, you can use the uniform
           `material_id` to branch the shader code for different meshes.
      
       [2] aside from value type properties and textures, other types of materials ain't supported,
           in particular, embedded textures are not handled, 3D textures or texture arrays won't be
           allowed, each texture must use the target `GL_TEXTURE_2D`, and should have either 3 or 4
           channels (RGB/RGBA for most textures), or a single channel (for PBR flavor metallic maps,
           roughness maps, etc), but never 2 or 5+ channels.
      
       [3] assume there's only one texture map of each type. e.g. if a material has multiple albedo
           maps or specular maps, only the first one is loaded (all others are silently discarded).
       
       [4] all meshes in the model must be consistent, in the sense that their vertex attributes
           must have the same format, which is used as the only truth for writing the shader. If
           this is not true, you can tweak the quality settings to generate normals, tangents, etc.
           e.g. if one mesh has a second UV channel, every mesh must have a UV2 attribute
           e.g. if one mesh doesn't have a tangent, the entire model is deemed tangent-space-free
       
       [5] when both textures and properties are present in a mesh, textures should take precedence.
       
       [6] users have the option to import textures manually into the model by use of the `Import()`
           function, this will overwrite the (often incorrectly loaded) materials and partially
           circumvent the rules mentioned above, to allow for more control and flexibility.

       due to the complexity of various model formats and different conventions of using materials,
       there can be a discrepancy between the artist's original intention about the model and the
       way assimp interprets the model, so we provide a `Report()` function to report the loading
       details, which can be used as a reference to write up a compatible shader for the model.

       depending on the model asset, assimp may not be able to correctly load textures every time,
       this problem comes up quite often for FBX models with separate (mostly PBR) textures, one
       can easily observe it by reading the loading report in the console. Since there is no silver
       bullet for all models, we decide to provide a `Import()` function to give users the option
       to import textures manually into the model. By default, auto loading textures is disabled
       unless the `manual` flag in ctor is set to false. But either way, `Import()` will overwrite
       the textures of the specified material name.
    */

    // forward declaration
    class Mesh;

    // import quality preset
    enum class Quality : uint32_t {
        Auto   = 0x0,
        Low    = aiProcessPreset_TargetRealtime_Fast,
        Medium = aiProcessPreset_TargetRealtime_Quality,
        High   = aiProcessPreset_TargetRealtime_MaxQuality
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

        bool manual = false;  // manual import mode: skip the loading of textures
        unsigned int n_verts = 0;
        unsigned int n_meshes = 0;
        unsigned int n_materials = 0;

        std::set<GLuint> imported_materials;  // material ids in this set are imported by the user
        std::unordered_map<std::string, GLuint> materials_cache;  // material name : material id

        std::unordered_map<GLuint, std::vector<std::string>> property_names;  // for report
        std::unordered_map<GLuint, std::vector<std::string>> texture_names;   // for report

        void ProcessNode(aiNode* ai_node);
        void ProcessMesh(aiMesh* ai_mesh);
        void ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh);

      public:
        Quality quality;
        std::string directory;
        std::bitset<6> vtx_format;

        std::vector<Mesh> meshes;
        std::unordered_map<GLuint, std::vector<property_variant>> properties;
        std::unordered_map<GLuint, std::vector<buffer_ref<Texture>>> textures;

      public:
        Model(const std::string& filepath, Quality quality = Quality::Auto, bool manual = true);
        ~Model() {}

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        Model(Model&& other) noexcept = default;
        Model& operator=(Model&& other) noexcept = default;

        void Report();
        void Import(const std::string& material_name, std::vector<buffer_ref<Texture>>& tex_refs);
    };

}
