#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "components/mesh.h"

using namespace core;

namespace components {

    void Mesh::CreateSphere(float radius) {
        static constexpr float PI = 3.141592653589f;

        // mesh grid size (default LOD = 500x500 vertices)
        unsigned int n_rows = 500;
        unsigned int n_cols = 500;

        for (unsigned int col = 0; col <= n_cols; ++col) {
            for (unsigned int row = 0; row <= n_rows; ++row) {
                // set mesh uv range to always be [0, 1], regardless of the value of radius
                // later in the fragment shader, we can scale uv coordinates however we want
                // e.g.: repeat uv 10 times if the texture wrap mode is set to `GL_REPEAT`
                //       => texture(sampler, uv * 10);
                float u = (float)row / (float)n_rows;
                float v = (float)col / (float)n_cols;

                // xyz coordinates scale with radius
                float x = cos(u * PI * 2) * sin(v * PI) * radius;
                float y = cos(v * PI) * radius;
                float z = sin(u * PI * 2) * sin(v * PI) * radius;

                Vertex vertex {};
                vertex.position  = glm::vec3(x, y, z);
                vertex.normal    = glm::vec3(x, y, z);  // sphere centered at the origin, normal = position
                vertex.uv        = glm::vec2(u, v);
                vertex.tangent   = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);

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
    }

    void Mesh::CreateCube(float size) {
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
            vertex.tangent   = glm::vec3(0.0f);
            vertex.bitangent = glm::vec3(0.0f);

            vertices.push_back(vertex);
        }
    }

    void Mesh::CreateCylinder(float radius) {
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
    }

    void Mesh::CreatePlane(float size) {
        static const glm::vec3 _(0.0f);
        static const glm::vec3 up { 0.0f, 1.0f, 0.0f };

        Vertex v_arr[8] = {};

        // positive y face
        v_arr[0] = { glm::vec3(-1, 0, +1) * size, +up, glm::vec2(0.0f, 0.0f), _, _ };
        v_arr[1] = { glm::vec3(+1, 0, +1) * size, +up, glm::vec2(size, 0.0f), _, _ };
        v_arr[2] = { glm::vec3(+1, 0, -1) * size, +up, glm::vec2(size, size), _, _ };
        v_arr[3] = { glm::vec3(-1, 0, -1) * size, +up, glm::vec2(0.0f, size), _, _ };

        // negative y face
        v_arr[4] = { glm::vec3(-1, 0, +1) * size, -up, glm::vec2(0.0f, size), _, _ };
        v_arr[5] = { glm::vec3(+1, 0, +1) * size, -up, glm::vec2(size, size), _, _ };
        v_arr[6] = { glm::vec3(+1, 0, -1) * size, -up, glm::vec2(size, 0.0f), _, _ };
        v_arr[7] = { glm::vec3(-1, 0, -1) * size, -up, glm::vec2(0.0f, 0.0f), _, _ };

        for (Vertex& v : v_arr) {
            vertices.push_back(v);
        }

        // counter-clockwise winding order
        indices = { 0, 1, 2, 2, 3, 0, 2, 1, 0, 0, 3, 2 };
    }

    Mesh::Mesh(Primitive object) {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // populate the vertices and indices vector
        switch (object) {
            case Primitive::Sphere:   CreateSphere();    break;
            case Primitive::Cube:     CreateCube();      break;
            case Primitive::Cylinder: CreateCylinder();  break;
            case Primitive::Plane:    CreatePlane();     break;

            default:
                CORE_ERROR("Undefined primitive mesh...");
                std::cin.get();  // pause the console before exiting
                exit(EXIT_FAILURE);
        }

        BindBuffer();
    }

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
        : vertices(vertices), indices(indices) {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        BindBuffer();
    }

    Mesh::~Mesh() {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // log message to the console so that we are aware of the *hidden* destructor calls
        // this can be super useful in case our data accidentally goes out of scope
        if (VAO > 0) {
            CORE_WARN("Destructing mesh data (VAO = {0})!", VAO);
        }

        glDeleteBuffers(1, &IBO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);

        vertices.clear();
        vertices.shrink_to_fit();

        indices.clear();
        indices.shrink_to_fit();
    }

    Mesh::Mesh(Mesh&& other) noexcept {
        *this = std::move(other);
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if (this != &other) {
            // free resources, reset this mesh to a clean null state
            glDeleteBuffers(1, &IBO);
            glDeleteBuffers(1, &VBO);
            glDeleteVertexArrays(1, &VAO);

            VAO = VBO = IBO = 0;

            vertices.clear();
            vertices.shrink_to_fit();

            indices.clear();
            indices.shrink_to_fit();

            // transfer ownership from other to this
            std::swap(VAO, other.VAO);
            std::swap(VBO, other.VBO);
            std::swap(IBO, other.IBO);
            std::swap(vertices, other.vertices);
            std::swap(indices, other.indices);
        }

        return *this;
    }

    void Mesh::BindBuffer() {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);  // position
        glEnableVertexAttribArray(1);  // normal
        glEnableVertexAttribArray(2);  // uv
        glEnableVertexAttribArray(3);  // tangent
        glEnableVertexAttribArray(4);  // bitangent

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
        // glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind VBO, this is optional (actually not desired)

        glGenBuffers(1, &IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // DO NOT unbind IBO until VAO has been unbound first

        glBindVertexArray(0);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // now it's safe to unbind IBO, but not recommended
    }

    void Mesh::Draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}
