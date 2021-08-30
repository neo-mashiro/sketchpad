#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "components/component.h"

namespace components {

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec2 uv2;  // allow two UV channels
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    enum class Primitive {
        Sphere,
        Cube,
        Cylinder,
        Plane
    };

    class Mesh : public Component {
      private:
        GLuint VBO, IBO;
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

      private:
        void BindBuffer();
        void CreateSphere(float radius = 1.0f);
        void CreateCube(float size = 1.0f);
        void CreateCylinder(float radius = 1.0f);
        void CreatePlane(float size = 10.0f);

      public:
        GLuint VAO;

        Mesh(Primitive object);
        Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void Draw() const;

        // this field is only used by meshes that are loaded from external models
        mutable GLuint material_id;
        void SetMaterialID(GLuint mid) const;
    };
}
