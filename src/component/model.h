#pragma once

#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "core/base.h"
#include "component/component.h"

namespace component {

    /* due to the variety of file formats and conventions of using materials, writing a universal
       model loading module is really hard. In order to observe consistent behavior across model
       formats, it's necessary to impose some restrictions on the models we are going to support.
       
       in the high-level picture, a model typically consists of multiple meshes, each mesh could
       have either 0 or 1 material. A material can be shared by multiple meshes, so the number of
       materials must be <= the number of meshes. The complexity of model loading really lies at
       the underlying structure of materials. Without materials, a model is merely a collection
       of meshes in the form of a tree data structure.

       # hierarchical mesh
       
       for this demo, we are not going to implement a feature-complete data-oriented scene graph,
       we won't deal with complex parent-child relationships in a hierarchy graph since our main
       focus is on rendering instead of physics or character animation. As a result, the meshes
       will simply be stored in a vector, which are considered static buffer data to be feeded to
       our rendering pipeline (only subject to transforms by the single model matrix).

       # vertex format

       all meshes in the model must be consistent, in the sense that their vertex attributes must
       have the same format. This format will be used as the ground truth for shading, in case it
       varies from mesh to mesh, you can tweak the quality settings to make them agree.

       e.g. if one mesh has a second UV channel, all meshes must have a UV2 vertex attribute
       e.g. if one mesh does not have a vertex tangent, the entire model must have no tangents

       # importing materials

       due to the various conventions of using material, there's no silver bullet for all models.
       for example, FBX models often come with separate (mostly PBR) texture files, but a blender
       model can have meshes and textures packed into a single file, many OBJ models do not have
       materials, but when they do, the materials are normally encoded in a separate ".mtl" file,
       and the texture files are hardcoded in relative path, which means that you can't move them
       around without updating the path.

       for simplicity, this class will only read vertices data and report the names of materials
       being used, and we assume that users always know the materials of the model. Once a model
       has been loaded, it's then the responsibility of the user to set a material for each mesh,
       users are expected to know what values the attributes should be set to, where the textures
       are located, so as to setup uniforms and textures properly. Simply put, this class doesn't
       read any attribute or texture (referenced or embedded) for the materials, users must setup
       them manually at a later time, based on the reported material keys.

       despite that our design is a bit over-simplified, it does help keep our class much cleaner.
       in fact, in most cases it's ideal to have the ability of tuning materials manually, which
       not only allows us to tweak the model's material, but we can also rename the texture files
       however we like, and store them in any folder without breaking things. Most model loading
       modules have a similar "import settings" section that will override the automatic loading
       result, and the fact that it's being used most of the time makes the automatic mode nearly
       worthless since it doesn't save us anything. Although auto mode can have its benefit in a
       fixed workflow where formats are largely standardized, there is likely to be a discrepancy
       between the artist's original intention of the model and the way "Assimp" interprets it,
       so we always prefer loading materials manually.

       note that each mesh will have a separate material setup manually by the user, so that each
       mesh can use a different shader and shading model, but such flexibility also implies that
       the number of meshes needs to be manageable, preferably less than 20.

       # material id

       most of the time we want to assign the same PBR material to all meshes in a model, so the
       same PBR shader is applied to every mesh. Given a single shader source, what if we were to
       shade each mesh differently? for example, some meshes have an emission map but other parts
       of the model do not? The simplest solution is to create a separate material for each mesh,
       obviously this involves a lot of tedious work of copying and duplication, not to mention
       the context switching cost on performance. This is where the material id comes into play.

       each mesh is uniquely identified by a material id, this is often the id of that mesh's VAO
       unless other meshes use the same material (in which case it'll be the VAO id of the first
       loaded mesh). In GLSL, there's a corresponding built-in uniform `e_material_id`, which can
       be used for branching the shader code in order to shade every mesh differently.
    */

    enum class Quality : uint32_t {  // import quality preset
        Auto   = 0x0,
        Low    = aiProcessPreset_TargetRealtime_Fast,
        Medium = aiProcessPreset_TargetRealtime_Quality,
        High   = aiProcessPreset_TargetRealtime_MaxQuality
    };

    class Mesh;
    class Material;

    class Model : public Component {
      private:
        const aiScene* ai_root = nullptr;
        std::bitset<6> vtx_format;
        std::unordered_map<std::string, GLuint> materials_cache;  // matkey : matid

        void ProcessNode(aiNode* ai_node);
        void ProcessMesh(aiMesh* ai_mesh);
        void ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh);

      public:
        unsigned int n_verts = 0;
        unsigned int n_tris = 0;
        unsigned int n_meshes = 0;
        std::vector<Mesh> meshes;
        std::unordered_map<GLuint, Material> materials;  // matid : material

      public:
        Model(const std::string& filepath, Quality quality);

        Material& SetMaterial(const std::string& matkey, asset_ref<Material>&& material);
    };

}
