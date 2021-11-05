#pragma once

#include <vector>
#include "buffer/buffer.h"
#include "components/component.h"

// forward declaration
namespace buffer {
    class VAO;
    class VBO;
    class IBO;
}

namespace components {

    enum class Primitive : uint8_t {
        Sphere,
        Cube,
        Cylinder,
        Plane,
        Quad2D
    };

    enum class RenderMode : uint8_t {
        Triangle,
        Outline,
        Wireframe,
        Instance
    };

    using namespace buffer;

    /* our mesh class is used to construct mesh data for commonly used geometry surfaces, it
       is also compatible with the model class to load from data imported via "Assimp". Here
       we have offered 3 overloaded ctors for the user, you can build a mesh by specifying a
       vertices vector and an indices vector, you can also provide a buffer reference (shared
       pointer) to an existing VAO if both geometry are of the same type. The purpose of this
       is to reuse vertices data as much as possible to save memory on the GPU side (think of
       it as an example of GPU instancing). Besides, you can also build a mesh from one of
       our built-in primitives, which includes a unit cube, a unit sphere, a rectangle plane,
       a cylinder and a 2D quad which is useful for drawing frame buffers.
       
       TODO:

       in the near future (when I have time), I will also add support for some commonly used
       geometric shapes such as toruses, cones, box frames, triangular and hexagonal prisms, 
       capsules, solid angles, ellipsoids, octahedrons, pyramids and Bezier surfaces. It is
       often hard to build such primitives from explicit functions as they are quite complex,
       but instead, we can make use of SDF (signed distance field) functions and raymarching
       in the ray tracying rendering pipeline.

       note that currently we do not support procedural meshes, even if we do, they should be
       created inside compute shaders rather than being handled by this class.

       there are many ways to use OpenGL buffers, but here we will take the simplest approach
       that each mesh has a unique VAO, which owns a unique VBO and IBO, such that all mesh
       data is stored in a single buffer, and then we only need to bind VAO before draw calls.
       alternatively, we can use a single VAO that dynamically binds VBO for different meshes,
       or use multiple VBOs per mesh (one for each vertex attribute), or share VBOs between
       multiple VAOs and meshes to save memory space, coupled with dynamic and stream usage
       hints to gain slightly better performance, but I'd rather not add too much complexity.

       for this demo, we will never need to update these buffers once they are setup. All the
       vertex attributes and triangle indices are measured in local model space, which means
       that these buffer data will be static. Users can use a transform matrix to manipulate
       the mesh however they like (translate the vertices, scale the uv coordinates, etc), but
       the internal buffer data never changes, so we will use `GL_STATIC_DRAW` as the hint.
       as soon as the ctor returns or input vectors get destructed (out of scope), the memory
       of these buffers' data would be freed on the CPU side, which forces OpenGL to upload
       the buffer data immediately to the GPU (VRAM to be specific). This is a bit slower but
       it happens only once to construct the mesh, so that later we would gain the benefit of
       saving bandwidth between frame updates.
       
       our code is based on the assumption that the buffer data is always static, if that was
       not the case, it's more efficient to map the buffer to the client address space in C++
       and set up buffer data via that pointer. There are many ways to handle dynamic vertex
       data, for example, you can hint OpenGL that the data is dynamic or coming in the form
       of streams, or even better, you can construct a VBO from an SSBO that is processed by
       a compute shader, that will be super fast because the data is already in GPU, but you
       need to manually create VBO and setup a custom VAO layout...
       
       for extremely large meshes with millions of vertices, you may not be able to send the
       vertices data to the GPU as OpenGL has a limit on how much you can send with a single
       call. In this case, you can work around it by splitting up the vertex attributes into
       two or more VBOs and send each of them separately, but multiple VBOs should still be
       governed by the single unique VAO. While this trick works, really the wisest course of
       action is to perform mesh simplification, that is, downsample the resolution of the
       mesh via algorithms such as "Quadric Error of Edge Collapse", this can help reduce the
       number of vertices drastically but is still able to preserve most details of the mesh,
       there will be no apparent visual distinction. Actually in this class, we will restrict
       the mesh data size to fit into only one VBO, do not extend it unless you have to.

       this class only has one `Draw()` method, but it is able to handle multiple operations.
       how this draw call behaves is dependent on the current render mode, which is triangles
       by default, but can also be "outline" (draw object outlines using a stencil buffer),
       "wireframe" (draw a mesh's wireframe by running a geometry shader), "instance" (batch
       rendering with GPU instancing) or more render modes that will come in the future.
    */

    class Mesh : public Component {
      public:
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 uv;
            glm::vec2 uv2;  // second UV channel
            glm::vec3 tangent;
            glm::vec3 bitangent;
        };

        static_assert(sizeof(Vertex) == 16 * sizeof(float));
        static const std::vector<GLint> vertex_attribute_size;
        static const std::vector<GLint> vertex_attribute_offset;

      private:
        buffer_ref<VAO> vao;
        buffer_ref<VBO> vbo;
        buffer_ref<IBO> ibo;

      private:
        void CreateBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
        void CreateSphere(float radius = 1.0f);
        void CreateCube(float size = 1.0f);
        void CreateCylinder(float radius = 1.0f);
        void CreatePlane(float size = 10.0f);
        void Create2DQuad(float size = 1.0f);

      public:
        size_t n_verts, n_tris;
        RenderMode render_mode = RenderMode::Triangle;

        Mesh(Primitive object);
        Mesh(buffer_ref<VAO> vao, size_t n_verts);
        Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        buffer_ref<VAO> GetVAO() const;

        void SetRenderMode(RenderMode mode);
        void Draw() const;

        // this field is only used by meshes that are loaded from external models
        mutable GLuint material_id;
        void SetMaterialID(GLuint mid) const;

    };
}
