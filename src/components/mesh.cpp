#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "buffer/vao.h"
#include "buffer/vbo.h"
#include "buffer/ibo.h"
#include "components/mesh.h"

using namespace core;

namespace components {

    const std::vector<GLint> Mesh::vertex_attribute_offset = {
        offsetof(Vertex, position),
        offsetof(Vertex, normal),
        offsetof(Vertex, uv),
        offsetof(Vertex, uv2),
        offsetof(Vertex, tangent),
        offsetof(Vertex, bitangent)
    };

    const std::vector<GLint> Mesh::vertex_attribute_size = { 3, 3, 2, 2, 3, 3 };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Mesh::CreateSphere(float radius) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        static constexpr float PI = 3.141592653589f;

        // mesh grid size (default LOD = 100x100 vertices)
        unsigned int n_rows = 100;
        unsigned int n_cols = 100;

        for (unsigned int col = 0; col <= n_cols; ++col) {
            for (unsigned int row = 0; row <= n_rows; ++row) {
                // standard unscaled uv coordinates
                float u = (float)row / (float)n_rows;
                float v = (float)col / (float)n_cols;

                // xyz coordinates scale with radius
                float x = cos(u * PI * 2) * sin(v * PI) * radius;
                float y = cos(v * PI) * radius;
                float z = sin(u * PI * 2) * sin(v * PI) * radius;

                Vertex vertex {};
                vertex.position  = { x, y, z };
                vertex.normal    = { x, y, z };  // sphere centered at the origin, normal = position
                vertex.uv        = { u, v };
                vertex.uv2       = { 0.0f };
                vertex.tangent   = { 0.0f };
                vertex.bitangent = { 0.0f };

                vertices.push_back(vertex);
            }
        }

        for (unsigned int col = 0; col < n_cols; ++col) {
            for (unsigned int row = 0; row < n_rows; ++row) {
                // counter-clockwise winding order
                indices.push_back((col + 1) * (n_rows + 1) + row);
                indices.push_back(col * (n_rows + 1) + row);
                indices.push_back(col * (n_rows + 1) + row + 1);

                // counter-clockwise winding order
                indices.push_back((col + 1) * (n_rows + 1) + row);
                indices.push_back(col * (n_rows + 1) + row + 1);
                indices.push_back((col + 1) * (n_rows + 1) + row + 1);
            }
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreateCube(float size) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        // define a cube of size 1, which has 24 vertices (with redefinition)
        const static int n_vertices = 24;
        const static int stride = 8;  // 3 + 3 + 2

        const static float data[] = {
            // ----position----    -------normal------    ----uv----
            -1.0f, -1.0f, -1.0f,   +0.0f, -1.0f, +0.0f,   0.0f, 0.0f,
            -1.0f, -1.0f, +1.0f,   +0.0f, -1.0f, +0.0f,   0.0f, 1.0f,
            +1.0f, -1.0f, +1.0f,   +0.0f, -1.0f, +0.0f,   1.0f, 1.0f,
            +1.0f, -1.0f, -1.0f,   +0.0f, -1.0f, +0.0f,   1.0f, 0.0f,
            -1.0f, +1.0f, -1.0f,   +0.0f, +1.0f, +0.0f,   1.0f, 0.0f,
            -1.0f, +1.0f, +1.0f,   +0.0f, +1.0f, +0.0f,   1.0f, 1.0f,
            +1.0f, +1.0f, +1.0f,   +0.0f, +1.0f, +0.0f,   0.0f, 1.0f,
            +1.0f, +1.0f, -1.0f,   +0.0f, +1.0f, +0.0f,   0.0f, 0.0f,
            -1.0f, -1.0f, -1.0f,   +0.0f, +0.0f, -1.0f,   0.0f, 0.0f,
            -1.0f, +1.0f, -1.0f,   +0.0f, +0.0f, -1.0f,   0.0f, 1.0f,
            +1.0f, +1.0f, -1.0f,   +0.0f, +0.0f, -1.0f,   1.0f, 1.0f,
            +1.0f, -1.0f, -1.0f,   +0.0f, +0.0f, -1.0f,   1.0f, 0.0f,
            -1.0f, -1.0f, +1.0f,   +0.0f, +0.0f, +1.0f,   0.0f, 0.0f,
            -1.0f, +1.0f, +1.0f,   +0.0f, +0.0f, +1.0f,   0.0f, 1.0f,
            +1.0f, +1.0f, +1.0f,   +0.0f, +0.0f, +1.0f,   1.0f, 1.0f,
            +1.0f, -1.0f, +1.0f,   +0.0f, +0.0f, +1.0f,   1.0f, 0.0f,
            -1.0f, -1.0f, -1.0f,   -1.0f, +0.0f, +0.0f,   0.0f, 0.0f,
            -1.0f, -1.0f, +1.0f,   -1.0f, +0.0f, +0.0f,   0.0f, 1.0f,
            -1.0f, +1.0f, +1.0f,   -1.0f, +0.0f, +0.0f,   1.0f, 1.0f,
            -1.0f, +1.0f, -1.0f,   -1.0f, +0.0f, +0.0f,   1.0f, 0.0f,
            +1.0f, -1.0f, -1.0f,   +1.0f, +0.0f, +0.0f,   0.0f, 0.0f,
            +1.0f, -1.0f, +1.0f,   +1.0f, +0.0f, +0.0f,   0.0f, 1.0f,
            +1.0f, +1.0f, +1.0f,   +1.0f, +0.0f, +0.0f,   1.0f, 1.0f,
            +1.0f, +1.0f, -1.0f,   +1.0f, +0.0f, +0.0f,   1.0f, 0.0f
        };

        // counter-clockwise winding order
        indices = {
            +0, +2, +1,   +0, +3, +2,   +4, +5, +6,
            +4, +6, +7,   +8, +9, 10,   +8, 10, 11,
            12, 15, 14,   12, 14, 13,   16, 17, 18,
            16, 18, 19,   20, 23, 22,   20, 22, 21
        };

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position  = glm::vec3(data[offset + 0], data[offset + 1], data[offset + 2]) * size;
            vertex.normal    = glm::vec3(data[offset + 3], data[offset + 4], data[offset + 5]);
            vertex.uv        = glm::vec2(data[offset + 6], data[offset + 7]);  // keep in [0, 1] range
            vertex.uv2       = { 0.0f };
            vertex.tangent   = { 0.0f };
            vertex.bitangent = { 0.0f };

            vertices.push_back(vertex);
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreateCylinder(float radius) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        static constexpr float PI = 3.141592653589f;

        // todo: recursively divide the side quads of a cube to approximate a cylinder
        auto normalize = [](glm::vec3 vertex) {
            float k = static_cast<float>(sqrt(2)) / sqrt(vertex.x * vertex.x + vertex.z * vertex.z);
            return glm::vec3(k * vertex.x, vertex.y, k * vertex.z);
        };

        // how to make this lambda function recursive?
        // auto divide_quad = [](glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, int n) {
        //     // recursive case
        //     if (n > 0) {
        //         glm::vec3 u = normalize((v1.x + v4.x) / 2, (v1.y + v4.y) / 2, (v1.z + v4.z) / 2);
        //         glm::vec3 d = normalize((v2.x + v3.x) / 2, (v2.y + v3.y) / 2, (v2.z + v3.z) / 2);
        //
        //         divide_quad(v1, v2, d, u, n - 1);
        //         divide_quad(u, d, v3, v4, n - 1);
        //     }
        //     // base case
        //     else {
        //         // store the vertex info of (v1, v2, v3, v4)
        //         // store indices
        //     }
        // }

        // for each quad of a cube {
        //     divide_quad(4 vertices of the quad, 10);
        // }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreatePlane(float size) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        const static int n_vertices = 8;
        const static int stride = 8;  // 3 + 3 + 2

        const static float data[] = {
            // ---position----    ------normal-----    ----uv----
            -1.0f, 0.0f, +1.0f,   0.0f, +1.0f, 0.0f,   0.0f, 0.0f,
            +1.0f, 0.0f, +1.0f,   0.0f, +1.0f, 0.0f,   1.0f, 0.0f,
            +1.0f, 0.0f, -1.0f,   0.0f, +1.0f, 0.0f,   1.0f, 1.0f,
            -1.0f, 0.0f, -1.0f,   0.0f, +1.0f, 0.0f,   0.0f, 1.0f,
            -1.0f, 0.0f, +1.0f,   0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
            +1.0f, 0.0f, +1.0f,   0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
            +1.0f, 0.0f, -1.0f,   0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,   0.0f, -1.0f, 0.0f,   0.0f, 0.0f
        };

        // counter-clockwise winding order
        indices = { 0, 1, 2, 2, 3, 0, 6, 5, 4, 4, 7, 6 };

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position  = glm::vec3(data[offset + 0], data[offset + 1], data[offset + 2]) * size;
            vertex.normal    = glm::vec3(data[offset + 3], data[offset + 4], data[offset + 5]);
            vertex.uv        = glm::vec2(data[offset + 6], data[offset + 7]);  // keep in [0, 1] range
            vertex.uv2       = { 0.0f };
            vertex.tangent   = { 0.0f };
            vertex.bitangent = { 0.0f };

            vertices.push_back(vertex);
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::Create2DQuad(float size) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        // in our 3D demo, the 2D quad is primarily used for drawing framebuffers
        const static int n_vertices = 4;
        const static int stride = 4;  // 2 + 2

        const static float data[] = {
            // position        uv
            -1.0f, -1.0f,  0.0f, 0.0f,
            +1.0f, -1.0f,  1.0f, 0.0f,
            +1.0f, +1.0f,  1.0f, 1.0f,
            -1.0f, +1.0f,  0.0f, 1.0f
        };

        // counter-clockwise winding order
        indices = { 0, 1, 2, 2, 3, 0 };

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position = glm::vec3(data[offset + 0], data[offset + 1], 0.0f) * size;
            vertex.uv       = glm::vec2(data[offset + 2], data[offset + 3]);  // keep in [0, 1] range
            vertices.push_back(vertex);
        }

        CreateBuffers(vertices, indices);
    }

    Mesh::Mesh(Primitive object) {
        switch (object) {
            case Primitive::Sphere:   CreateSphere();    break;
            case Primitive::Cube:     CreateCube();      break;
            case Primitive::Cylinder: CreateCylinder();  break;
            case Primitive::Plane:    CreatePlane();     break;
            case Primitive::Quad2D:   Create2DQuad();    break;
            default:
                CORE_ASERT(false, "Unable to construct the mesh, primitive undefined...");
        }
    }

    Mesh::Mesh(buffer_ref<VAO> vao, size_t n_verts)
        : vao(vao), n_verts(n_verts), n_tris(n_verts / 3), render_mode(RenderMode::Triangle) {}

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        CreateBuffers(vertices, indices);
        material_id = vao->GetID();  // only this ctor will be called when loading external models
    }

    Mesh::~Mesh() {
        // log message to the console so that we are aware of the *hidden* destructor calls
        if (vao != nullptr) {
            CORE_WARN("Destructing mesh data (VAO = {0})!", vao->GetID());
        }
    }

    Mesh::Mesh(Mesh&& other) noexcept {
        *this = std::move(other);
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if (this != &other) {
            // free resources, reset this mesh to a clean null state
            vao = nullptr;
            vbo = nullptr;
            ibo = nullptr;

            n_verts = n_tris = material_id = 0;
            render_mode = RenderMode::Triangle;

            // transfer ownership of all members
            std::swap(vao, other.vao);
            std::swap(vbo, other.vbo);
            std::swap(ibo, other.ibo);
            std::swap(n_verts, other.n_verts);
            std::swap(n_tris, other.n_tris);
            std::swap(material_id, other.material_id);
            std::swap(render_mode, other.render_mode);
        }

        return *this;
    }

    void Mesh::CreateBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        vao = LoadBuffer<VAO>();
        vbo = LoadBuffer<VBO>();
        ibo = LoadBuffer<IBO>();

        vbo->SetData(vertices.size() * sizeof(Vertex), &vertices[0]);
        ibo->SetData(indices.size() * sizeof(GLuint), &indices[0]);

        GLuint vbo_id = vbo->GetID();
        GLuint ibo_id = ibo->GetID();

        for (GLuint i = 0; i < 6; i++) {
            vao->SetVBO(vbo_id, i, vertex_attribute_offset[i], vertex_attribute_size[i], sizeof(Vertex));
        }

        vao->SetIBO(ibo_id);

        n_verts = indices.size();
        n_tris = n_verts / 3;

        SetRenderMode(RenderMode::Triangle);
    }

    buffer_ref<VAO> Mesh::GetVAO() const {
        return vao;
    }

    void Mesh::SetRenderMode(RenderMode mode) {
        render_mode = mode;
    }

    void Mesh::Draw() const {
        vao->Bind();

        if (render_mode == RenderMode::Triangle) {
            glDrawElements(GL_TRIANGLES, n_verts, GL_UNSIGNED_INT, 0);
        }
        else if (render_mode == RenderMode::Outline) {
            // use stencil buffer and raycasting
            // todo: implement outline.glsl (use textureProjOffset), use that shader to draw again
        }
        else if (render_mode == RenderMode::Wireframe) {
            // use internal geometry shader
        }
        else if (render_mode == RenderMode::Instance) {
            // draw with GPU instancing
        }
        
        vao->Unbind();
    }

    void Mesh::SetMaterialID(GLuint mid) const {
        material_id = mid;
    }
    
}
