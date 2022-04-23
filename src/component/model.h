/* 
   adapted and refactored based on https://learnopengl.com/ by Joey de Vries

   due to the variety of file formats and conventions of using materials, writing a universal
   model loading module is really hard. In order to observe consistent behavior across model
   formats, it's necessary to impose some restrictions on the models we are going to support.

   in the high-level picture, a model typically consists of multiple meshes, each mesh could
   have either 0 or 1 material. A material can be shared by multiple meshes, so the number of
   materials must be <= the number of meshes. The complexity of model loading really lies at
   the underlying structure of materials. Without materials, a model is merely a collection
   of meshes in the form of a tree data structure.

   # hierarchical mesh

   since we won't be dealing with a feature-complete data-oriented scene graph in this demo,
   the hierarchical data of nodes is saved, but only used by skeleton animation not the mesh.
   the meshes are simply stored in a vector without any parent-child transform relationships,
   they are treated as static data (in model space or bind pose) to build up the VBO buffers.
   using the cached nodes vector, users can implement hierarchy transforms on their own, or
   have the vertices displaced in vertex shader, but we do not provide these as built-in.

   # vertex format

   all meshes in the model must be consistent, in the sense that their vertex attributes must
   agree on the format. For example, if one mesh has tangent vectors, all other meshes in the
   model should have tangents as well, otherwise we'd have to differentiate between meshes in
   the shader using material ids, thus introducing additional branches.
   
   if consistent vertex format is not observed between meshes, we can still proceed but it's
   often a hint that we need to fix our model. This often occurs to relatively large complex
   models with a lot of meshes, and is most likely caused by missing UVs in some part of the
   model. When this happens, the artist's intention is usually to apply textures to only some
   of the meshes, while others are expected to be shaded with plain color. As for users, they
   must have clear knowledge of which mesh is textured and which is not.

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
   them manually at a later time, based on the reported material keys. This is appropriate as
   long as we stick to the physically based shading model.

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

   most of the time we want to assign the same PBR shader to all meshes in a model, but given
   a single shader source, what if we were to shade each mesh differently? For example, some
   meshes may have an emission map while other parts of the model do not, how to handle this?
   this is where the material id comes into play.

   each mesh is uniquely identified by a material id, this is often the id of that mesh's VAO
   unless other meshes use the same material (in which case it'll be the VAO id of the first
   loaded mesh). In GLSL, there's a corresponding built-in uniform `self.material_id`, which
   can be used for branching the shader code in order to shade every mesh differently.

   # skeleton animation

   users can optionally attach animations (motions) to the imported model. Ideally, animation
   should be included in the same file as the model (often FBX), so that the bone structures
   in model and animation can exactly match. Although it's allowed to attach animation from a
   separate file, it may not be compatible with the model as the bone names and structure can
   vary wildly from case to case, so we highly recommend to bake animation into the model.
   
   for details on how the bones and animation data are structured and used, see `animator.h`
*/

#pragma once

#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "core/base.h"
#include "component/component.h"

namespace component {

    class Animation;
    class Mesh;
    class Material;

    class Node {
      public:
        Node(int nid, int pid, const std::string& name);
        bool IsBone() const;
        bool Animated() const;

        int nid = -1;    // node id, must >= 0
        int pid = -1;    // node id of the parent, must < nid, for the root node, this is -1
        int bid = -1;    // bone id, if node is not a bone node, this is -1
        bool alive = 0;  // is bone node && influenced by a channel

        std::string name;
        glm::mat4 n2p;   // node space -> parent space (local transform relative to the parent)
        glm::mat4 m2n;   // model space (bind pose) -> node space (bone space), for bone nodes only
        glm::mat4 n2m;   // bone space -> model space, updated at runtime, N/A if not alive
    };

    enum class Quality : uint32_t {  // import quality preset
        Auto   = 0x0,
        Low    = aiProcessPreset_TargetRealtime_Fast,
        Medium = aiProcessPreset_TargetRealtime_Quality,
        High   = aiProcessPreset_TargetRealtime_MaxQuality
    };

    class Model : public Component {
      private:
        const aiScene* ai_root = nullptr;
        std::bitset<6> vtx_format;
        std::unordered_map<std::string, GLuint> materials_cache;  // matkey : matid

      public:
        unsigned int n_nodes = 0, n_bones = 0;
        unsigned int n_meshes = 0, n_verts = 0, n_tris = 0;
        bool animated = false;

        std::vector<Node> nodes;
        std::vector<Mesh> meshes;
        std::unordered_map<GLuint, Material> materials;  // matid : material
        std::unique_ptr<Animation> animation;

      private:
        void ProcessTree(aiNode* ai_node, int parent);
        void ProcessNode(aiNode* ai_node);
        void ProcessMesh(aiMesh* ai_mesh);
        void ProcessMaterial(aiMaterial* ai_material, const Mesh& mesh);

      public:
        Model(const std::string& filepath, Quality quality, bool animate = false);
        Material& SetMaterial(const std::string& matkey, asset_ref<Material>&& material);
        void AttachMotion(const std::string& filepath);
    };

}
