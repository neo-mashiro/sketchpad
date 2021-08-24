#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace components {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    enum class Primitive {
        Sphere,
        Cube,
        Cylinder,
        Plane
    };

    class Mesh {
      private:
        GLuint VAO, VBO, IBO;
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

      private:
        void BindBuffer();
        void CreateSphere(float radius = 1.0f);
        void CreateCube(float size = 1.0f);
        void CreateCylinder(float radius = 1.0f);
        void CreatePlane(float size = 10.0f);

      public:
        Mesh(Primitive object);
        Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void Draw() const;
    };
}
