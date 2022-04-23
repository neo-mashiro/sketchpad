/* 
   the mesh class is used to construct mesh data for commonly used geometry surfaces, it
   is also compatible with the model class to load from data imported via "Assimp". Here
   we have offered 3 overloaded ctors for the user, you can build a mesh by specifying a
   vertices vector and an indices vector, you can also provide a buffer reference (shared
   pointer) to an existing VAO if both geometry are of the same type. The purpose of this
   is to reuse vertices data as much as possible to save memory on the GPU side (think of
   it as an example of GPU instancing). Besides, you can also build a mesh from one of
   our built-in primitives, which includes a unit cube, a unit sphere, a rectangle plane,
   a cylinder and a 2D quad which is useful for drawing framebuffers.

   # mesh layout

   there are many ways to use OpenGL buffers, but here we will take the simplest approach
   that each mesh has a unique VAO, which owns a unique VBO and IBO, such that all mesh
   data is stored in a single buffer, and then we only need to bind VAO before draw calls.
   alternatively, we can use a single VAO that dynamically binds VBO for different meshes,
   or use multiple VBOs per mesh (one for each vertex attribute), or share VBOs between
   multiple VAOs and meshes to save memory space, coupled with dynamic and stream usage
   hints to gain slightly better performance, but I'd rather not add too much complexity.

   in this demo, we will never need to update these buffers once they are setup. All the
   vertex attributes and triangle indices are measured in local model space, which means
   that these buffer data will be static. Users can use a transform matrix to manipulate
   the mesh however they like (translate the vertices, scale the uv coordinates, etc), but
   the internal data never changes, so we use a NULL access flag when creating the buffer.

   as soon as the ctor returns or input vectors get destructed (out of scope), the memory
   of these buffers' data would be freed on the CPU side, which forces OpenGL to upload
   the buffer data immediately to the GPU (VRAM to be specific). This is a bit slower but
   it happens only once to construct the mesh, so that later we would gain the benefit of
   saving bandwidth between frame updates.

   # use a custom layout

   our code is based on the assumption that the buffer data is always static, if that was
   not the case, it's more efficient to map the buffer to the client address space in C++
   or update the data store on the GPU side. There are many ways to handle dynamic vertex
   data, for example, we can hint OpenGL that the data is dynamic or coming in the form
   of streams, or even better, we can have an SSBO that is processed by a compute shader,
   and treat the SSBO as if it was a VBO, that will be super fast as the data is already
   in GPU, but we need to setup a custom VAO layout, "scene04" is an example of this.

   # size limit on vertices data

   for extremely large meshes with millions of vertices, we may not be able to send the
   vertices data to the GPU as OpenGL has a limit on how much we can send with a single
   call. In this case, we can work around it by splitting up the vertex attributes into
   two or more VBOs and send each of them separately, but multiple VBOs should still be
   governed by the single unique VAO. While this trick works, really the wisest course of
   action is to perform mesh simplification, that is, downsample the resolution of the
   mesh via algorithms such as "Quadric Error of Edge Collapse" and "Loop Subdivision",
   this can help reduce the number of vertices drastically but is still able to preserve
   most details of the mesh, there will be no apparent visual distinction. In this class,
   we will restrict the mesh data size to fit into only one VBO.
*/

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "asset/vao.h"
#include "asset/buffer.h"
#include "component/component.h"

namespace component {

    enum class Primitive : uint8_t {
        Sphere, Cube, Plane, Quad2D, Torus, Capsule, Tetrahedron
    };

    class Mesh : public Component {
      public:
        struct Vertex {
            glm::vec3  position;
            glm::vec3  normal;
            glm::vec2  uv;
            glm::vec2  uv2;
            glm::vec3  tangent;
            glm::vec3  binormal;
            glm::ivec4 bone_id;  // 4 bones per vertex rule
            glm::vec4  bone_wt;  // the weight of each bone
        };

        static_assert(sizeof(Vertex) == 20 * sizeof(float) + 4 * sizeof(int));
        size_t n_verts, n_tris;

      private:
        friend class Model;
        asset_ref<asset::VAO> vao;
        asset_ref<asset::VBO> vbo;
        asset_ref<asset::IBO> ibo;

        void CreateSphere(float radius = 1.0f);
        void CreateCube(float size = 1.0f);
        void CreatePlane(float size = 10.0f);
        void Create2DQuad(float size = 1.0f);
        void CreateTorus(float R = 1.5f, float r = 0.5f);
        void CreateCapsule(float a = 2.0f, float r = 1.0f);
        void CreatePyramid(float s = 2.0f);
        void CreateBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);

      public:
        Mesh(Primitive object);
        Mesh(asset_ref<asset::VAO> vao, size_t n_verts);
        Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
        Mesh(const asset_ref<Mesh>& mesh_asset);

        void Draw() const;
        static void DrawQuad();
        static void DrawGrid();

        // this field is only used by meshes that are loaded from external models
        mutable GLuint material_id;
        void SetMaterialID(GLuint mid) const;
    };

}
