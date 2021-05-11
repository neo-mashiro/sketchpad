#pragma once

#include "canvas.h"
#include "shader.h"
#include "texture.h"

namespace Sketchpad {

    constexpr float PI = 3.141592653589f;

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    enum class Primitive { Sphere, Cube, Cylinder, Plane };

    class Mesh {
      private:
        GLuint VAO, VBO, IBO;

        void BindBuffer();
        void BindTexture(const Shader& shader, bool layout_bind) const;

        // safely move assign the passed in textures vector to our class member
        // without breaking the global OpenGL texture binding states
        void SafeMoveTextures(std::vector<Texture>& source_textures);

        // -------------------------------------------------------------------------------------------
        // The member functions below can be used to create some primitive objects with correct vertex
        // info, such as a sphere of radius `r`, or a cube of size `n`. By applying transformations to
        // them (via the model matrix `M`), we can generalize the cube to a cuboid with unequal sides,
        // or scale the square plane into a rectangle plane, it's also possible to combine spheres and
        // cylinders to make a capsule. All of these can be done by applying a proper model matrix `M`.
        //
        // However, be aware that affine transforms may not preserve orthogonality. Generally speaking,
        // rotations and translations are always orthogonal, but non-uniform scaling and shear are not.
        // As a result, the model matrix `M` is not orthogonal, so after the transform, `M * normal`
        // are no longer the correct normal vectors, so tangents and shading would break as well.
        // To fix this, we must manually recompute the normals, possibly in the geometry shader before
        // the fragment shader, by multiplying them by the transpose of the inverse of matrix `M`:
        // => n' = (M^(-1))^T * n
        //
        // That said, it is recommended not to shear or scale non-uniformly.
        // -------------------------------------------------------------------------------------------
        void CreateSphere(float radius = 1.0f);
        void CreateCube(float size = 1.0f);
        void CreateCylinder(float radius = 1.0f);
        void CreatePlane(float size = 100.0f, float elevation = -2.0f);
        void CreatePrimitive(Primitive object);

      public:
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        glm::mat4 M;  // every mesh has its own model matrix M, responsible for 3D transformations.
                      // upon instantiation, M is initialized to the identity matrix, only external
                      // callers should update it later, M is not supposed to be mutated internally

        // std::vector<Texture>& cannot be marked "const" as we need to std::move() assign it
        Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
            std::vector<Texture>& textures);

        Mesh(Primitive object, std::vector<Texture>& textures);
        Mesh(Primitive object);

        ~Mesh();

        // forbid the copying of instances because they encapsulate global OpenGL resources and states.
        // when that happens, the old instance would probably be destructed and ruin the global states,
        // so we end up with a copy that points to an OpenGL object that has already been destroyed.
        // compiler-provided copy constructor and assignment operator only perform shallow copy.

        Mesh(const Mesh&) = delete;             // delete the copy constructor
        Mesh& operator=(const Mesh&) = delete;  // delete the assignment operator

        // it may be possible to override the copy constructor and assignment operator to make deep
        // copies and put certain constraints on the destructor, so as to keep the OpenGL states alive,
        // but that could be incredibly expensive or even impractical.

        // a better option is to use move semantics:
        Mesh(Mesh&& other) noexcept;             // move constructor
        Mesh& operator=(Mesh&& other) noexcept;  // move assignment operator

        void Draw(const Shader& shader, bool layout_bind = false) const;
    };
}
