#include "pch.h"

#include "core/base.h"
#include "core/app.h"
#include "core/debug.h"
#include "core/log.h"
#include "component/mesh.h"

using namespace glm;
using namespace core;
using namespace asset;

namespace component {

    static const std::vector<GLint> va_offset = {  // vertex attribute offset
        offsetof(Mesh::Vertex, position),
        offsetof(Mesh::Vertex, normal),
        offsetof(Mesh::Vertex, uv),
        offsetof(Mesh::Vertex, uv2),
        offsetof(Mesh::Vertex, tangent),
        offsetof(Mesh::Vertex, binormal),
        offsetof(Mesh::Vertex, bone_id),
        offsetof(Mesh::Vertex, bone_wt)
    };

    static const std::vector<GLint> va_size = { 3, 3, 2, 2, 3, 3, 4, 4 };  // vertex attribute size

    static asset_tmp<VAO> internal_vao;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Mesh::CreateSphere(float radius) {
        constexpr float PI = glm::pi<float>();
        constexpr float PI_2 = glm::half_pi<float>();

        // default LOD = 100x100 mesh grid size
        unsigned int n_rows  = 100;
        unsigned int n_cols  = 100;
        unsigned int n_verts = (n_rows + 1) * (n_cols + 1);
        unsigned int n_tris  = n_rows * n_cols * 2;

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        vertices.reserve(n_verts);
        indices.reserve(n_tris * 3);

        for (unsigned int col = 0; col <= n_cols; ++col) {
            for (unsigned int row = 0; row <= n_rows; ++row) {
                // unscaled uv coordinates ~ [0, 1]
                float u = static_cast<float>(col) / n_cols;
                float v = static_cast<float>(row) / n_rows;

                float theta = PI * v - PI_2;  // ~ [-PI/2, PI/2], latitude from south to north pole
                float phi = PI * 2 * u;       // ~ [0, 2PI], longitude around the equator circle

                float x = cos(phi) * cos(theta);
                float y = sin(theta);
                float z = sin(phi) * cos(theta) * (-1);

                // for a unit sphere centered at the origin, normal = position
                // binormal is normal rotated by 90 degrees along the latitude (+theta)
                theta += PI_2;
                float r = cos(phi) * cos(theta);
                float s = sin(theta);
                float t = sin(phi) * cos(theta) * (-1);

                Vertex vertex {};
                vertex.position = vec3(x, y, z) * radius;
                vertex.normal   = vec3(x, y, z);
                vertex.uv       = vec2(u, v);
                vertex.binormal = vec3(r, s, t);
                vertex.tangent  = glm::cross(vertex.binormal, vertex.normal);

                vertices.push_back(vertex);
            }
        }

        for (unsigned int col = 0; col < n_cols; ++col) {
            for (unsigned int row = 0; row < n_rows; ++row) {
                auto index = col * (n_rows + 1);

                // counter-clockwise winding order
                indices.push_back(index + row + 1);
                indices.push_back(index + row);
                indices.push_back(index + row + 1 + n_rows);

                // counter-clockwise winding order
                indices.push_back(index + row + 1 + n_rows + 1);
                indices.push_back(index + row + 1);
                indices.push_back(index + row + 1 + n_rows);
            }
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreateCube(float size) {
        constexpr int n_vertices = 24;  // we only need 24 vertices to triangulate the 6 faces
        constexpr int stride = 8;  // 3 + 3 + 2

        std::vector<Vertex> vertices;
        vertices.reserve(n_vertices);

        static const float data[] = {
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

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position = vec3(data[offset + 0], data[offset + 1], data[offset + 2]) * size;
            vertex.normal   = vec3(data[offset + 3], data[offset + 4], data[offset + 5]);
            vertex.uv       = vec2(data[offset + 6], data[offset + 7]);

            vertices.push_back(vertex);
        }

        // counter-clockwise winding order
        std::vector<GLuint> indices {
            +0, +2, +1,   +0, +3, +2,   +4, +5, +6,
            +4, +6, +7,   +8, +9, 10,   +8, 10, 11,
            12, 15, 14,   12, 14, 13,   16, 17, 18,
            16, 18, 19,   20, 23, 22,   20, 22, 21
        };

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreatePlane(float size) {
        constexpr int n_vertices = 8;
        constexpr int stride = 8;  // 3 + 3 + 2

        static const float data[] = {
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

        std::vector<Vertex> vertices;
        vertices.reserve(n_vertices);

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position = vec3(data[offset + 0], data[offset + 1], data[offset + 2]) * size;
            vertex.normal   = vec3(data[offset + 3], data[offset + 4], data[offset + 5]);
            vertex.uv       = vec2(data[offset + 6], data[offset + 7]);

            vertices.push_back(vertex);
        }

        // counter-clockwise winding order
        std::vector<GLuint> indices { 0, 1, 2, 2, 3, 0, 6, 5, 4, 4, 7, 6 };

        CreateBuffers(vertices, indices);
    }

    void Mesh::Create2DQuad(float size) {
        constexpr int n_vertices = 4;
        constexpr int stride = 4;  // 2 + 2

        static const float data[] = {
            // position        uv
            -1.0f, -1.0f,  0.0f, 0.0f,
            +1.0f, -1.0f,  1.0f, 0.0f,
            +1.0f, +1.0f,  1.0f, 1.0f,
            -1.0f, +1.0f,  0.0f, 1.0f
        };

        std::vector<Vertex> vertices;
        vertices.reserve(n_vertices);

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;
            Vertex vertex {};
            vertex.position = vec3(data[offset + 0], data[offset + 1], 0.0f) * size;
            vertex.uv       = vec2(data[offset + 2], data[offset + 3]);
            vertices.push_back(vertex);
        }

        // counter-clockwise winding order
        std::vector<GLuint> indices { 0, 1, 2, 2, 3, 0 };

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreateTorus(float R, float r) {
        // default LOD = 60x60 faces, step size = 6 degrees
        GLuint n_rings = 60;
        GLuint n_sides = 60;
        GLuint n_faces = n_sides * n_rings;  // quad face (2 triangles)
        GLuint n_verts = n_sides * n_rings + n_sides;

        float delta_phi   = glm::two_pi<float>() / n_rings;
        float delta_theta = glm::two_pi<float>() / n_sides;

        std::vector<Vertex> vertices;
        vertices.reserve(n_verts);

        for (GLuint ring = 0; ring <= n_rings; ring++) {
            float phi = ring * delta_phi;
            float cos_phi = cos(phi);
            float sin_phi = sin(phi);

            for (GLuint side = 0; side < n_sides; side++) {
                float theta = side * delta_theta;
                float cos_theta = cos(theta);
                float sin_theta = sin(theta);

                float d = (R + r * cos_theta);  // distance from the vertex to the torus center

                float x = d * cos_phi;
                float y = d * sin_phi;
                float z = r * sin_theta;

                float a = d * cos_theta * cos_phi;
                float b = d * cos_theta * sin_phi;
                float c = d * sin_theta;

                float u = glm::one_over_two_pi<float>() * phi;
                float v = glm::one_over_two_pi<float>() * theta;
                
                Vertex vertex {};
                vertex.position = vec3(x, y, z);
                vertex.normal   = normalize(vec3(a, b, c));
                vertex.uv       = vec2(u, v);

                vertices.push_back(vertex);
            }
        }

        std::vector<GLuint> indices;
        indices.reserve(n_faces * 6);

        for (GLuint ring = 0; ring < n_rings; ring++) {
            GLuint offset = n_sides * ring;

            for (GLuint side = 0; side < n_sides; side++) {
                GLuint next_side = (side + 1) % n_sides;

                indices.push_back(offset + side);
                indices.push_back(offset + n_sides + side);
                indices.push_back(offset + n_sides + next_side);

                indices.push_back(offset + side);
                indices.push_back(offset + next_side + n_sides);
                indices.push_back(offset + next_side);
            }
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreateCapsule(float a, float r) {
        // by default, the capsule is centered at the origin, standing upright
        // default LOD = 100x100, a = cylinder height, r = hemisphere radius
        constexpr float n_rows = 100.0f;
        constexpr float n_cols = 100.0f;
        constexpr float PI = glm::pi<float>();
        constexpr float PI_2 = glm::half_pi<float>();

        float half_a = a / 2;  // half the cylinder height

        auto cylinder = [&](float u, float v) -> Vertex {
            float x = cos(PI * 2.0f * u) * r;
            float z = sin(PI * 2.0f * u) * r * (-1);
            float y = (v - 0.5f) * a;  // ~ [-a/2, a/2]

            Vertex vertex {};
            vertex.position = vec3(x, y, z);
            vertex.normal   = normalize(vec3(x, 0.0f, z));
            vertex.uv       = vec2(u, v);
            vertex.binormal = vec3(0.0f, 1.0f, 0.0f);
            vertex.tangent  = glm::cross(vertex.binormal, vertex.normal);
            return vertex;
        };

        auto lower_hemisphere = [&](float u, float v) -> Vertex {
            float phi   = (PI * 2.0f) * u;        // ~ [0, 2PI]
            float theta = (PI / 2.0f) * (v - 1);  // ~ [-PI/2, 0]
            float x = cos(phi) * cos(theta) * r;
            float z = sin(phi) * cos(theta) * r * (-1);
            float y = sin(theta) * r;

            theta += PI_2;
            float r = cos(phi) * cos(theta);
            float t = sin(phi) * cos(theta) * (-1);
            float s = sin(theta);

            Vertex vertex {};
            vertex.position = vec3(x, y - half_a, z);
            vertex.normal   = normalize(vec3(x, y, z));
            vertex.uv       = vec2(u, v);
            vertex.binormal = vec3(r, s, t);
            vertex.tangent  = glm::cross(vertex.binormal, vertex.normal);

            return vertex;
        };

        auto upper_hemisphere = [&](float u, float v) -> Vertex {
            float phi   = (PI * 2.0f) * u;  // ~ [0, 2PI]
            float theta = (PI / 2.0f) * v;  // ~ [0, PI/2]
            float x = cos(phi) * cos(theta) * r;
            float z = sin(phi) * cos(theta) * r * (-1);
            float y = sin(theta) * r;

            theta += PI_2;
            float r = cos(phi) * cos(theta);
            float t = sin(phi) * cos(theta) * (-1);
            float s = sin(theta);

            Vertex vertex {};
            vertex.position = vec3(x, y + half_a, z);
            vertex.normal   = normalize(vec3(x, y, z));
            vertex.uv       = vec2(u, v);
            vertex.binormal = vec3(r, s, t);
            vertex.tangent = glm::cross(vertex.binormal, vertex.normal);

            return vertex;
        };

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        vertices.reserve(static_cast<int>(n_rows * n_cols) * 12);
        indices.reserve(static_cast<int>(n_rows * n_cols) * 18);

        auto insert_indices = [&indices](auto&&... e) {
            (indices.push_back(std::forward<decltype(e)>(e)), ...);
        };

        unsigned int index = 0;

        for (float col = 0; col < n_cols; ++col) {
            for (float row = 0; row < n_rows; ++row) {
                float u0 = col / n_cols;
                float v0 = row / n_rows;
                float u1 = u0 + (1 / n_cols);
                float v1 = v0 + (1 / n_rows);

                // construct cylinder
                vertices.push_back(cylinder(u0, v0));  // push back rvalue, implicitly moved
                vertices.push_back(cylinder(u0, v1));
                vertices.push_back(cylinder(u1, v0));
                vertices.push_back(cylinder(u1, v1));

                insert_indices(index + 2, index + 1, index + 0);  // 1st triangle
                insert_indices(index + 1, index + 2, index + 3);  // 2nd triangle
                index += 4;

                // construct lower hemisphere
                vertices.push_back(lower_hemisphere(u0, v0));
                vertices.push_back(lower_hemisphere(u0, v1));
                vertices.push_back(lower_hemisphere(u1, v0));
                vertices.push_back(lower_hemisphere(u1, v1));

                insert_indices(index + 2, index + 1, index + 0);  // 1st triangle
                insert_indices(index + 1, index + 2, index + 3);  // 2nd triangle
                index += 4;

                // construct upper hemisphere
                vertices.push_back(upper_hemisphere(u0, v0));
                vertices.push_back(upper_hemisphere(u0, v1));
                vertices.push_back(upper_hemisphere(u1, v0));
                vertices.push_back(upper_hemisphere(u1, v1));

                insert_indices(index + 2, index + 1, index + 0);  // 1st triangle
                insert_indices(index + 1, index + 2, index + 3);  // 2nd triangle
                index += 4;
            }
        }

        CreateBuffers(vertices, indices);
    }

    void Mesh::CreatePyramid(float s) {
        constexpr int n_vertices = 16;  // really just 5 vertices but 16 normal directions
        constexpr int stride = 8;  // 3 + 3 + 2

        std::vector<Vertex> vertices;
        vertices.reserve(n_vertices);

        // mesh data precomputed in Blender
        static const float data[] = {
            // ----position----   --------------normal--------------   ---------uv---------
            +0.5f, +0.0f, +0.5f,  +0.000000f, -1.000000f, -0.000000f,  0.224609f, 0.390625f,
            -0.5f, +0.0f, +0.5f,  +0.000000f, -1.000000f, -0.000000f,  0.656250f, 0.390625f,
            +0.5f, +0.0f, -0.5f,  +0.000000f, -1.000000f, -0.000000f,  0.224609f, 0.816406f,
            -0.5f, +0.0f, -0.5f,  +0.000000f, -1.000000f, -0.000000f,  0.656250f, 0.816406f,
            +0.5f, +0.0f, +0.5f,  +0.894406f, +0.447188f, -0.000000f,  0.222656f, 0.390625f,
            +0.5f, +0.0f, -0.5f,  +0.894406f, +0.447188f, -0.000000f,  0.000000f, 0.000000f,
            +0.0f, +1.0f, +0.0f,  +0.894406f, +0.447188f, -0.000000f,  0.445313f, 0.000000f,
            -0.5f, +0.0f, +0.5f,  +0.000000f, +0.447188f, +0.894406f,  0.653863f, 0.377007f,
            +0.5f, +0.0f, +0.5f,  +0.000000f, +0.447188f, +0.894406f,  0.223340f, 0.379275f,
            +0.0f, +1.0f, +0.0f,  +0.000000f, +0.447188f, +0.894406f,  0.442318f, 0.000000f,
            +0.5f, +0.0f, -0.5f,  +0.000000f, +0.447188f, -0.894406f,  0.447266f, 0.000000f,
            -0.5f, +0.0f, -0.5f,  +0.000000f, +0.447188f, -0.894406f,  0.882812f, 0.000000f,
            +0.0f, +1.0f, +0.0f,  +0.000000f, +0.447188f, -0.894406f,  0.656250f, 0.376953f,
            -0.5f, +0.0f, -0.5f,  -0.894406f, +0.447188f, -0.000000f,  0.000000f, 0.000000f,
            -0.5f, +0.0f, +0.5f,  -0.894406f, +0.447188f, -0.000000f,  0.226638f, 0.386567f,
            +0.0f, +1.0f, +0.0f,  -0.894406f, +0.447188f, -0.000000f,  0.446560f, 0.000000f,
        };

        for (unsigned int i = 0; i < n_vertices; i++) {
            unsigned int offset = i * stride;

            Vertex vertex {};
            vertex.position = vec3(data[offset + 0], data[offset + 1], data[offset + 2]) * s;
            vertex.normal   = vec3(data[offset + 3], data[offset + 4], data[offset + 5]);
            vertex.uv       = vec2(data[offset + 6], data[offset + 7]);
            vertex.normal   = normalize(vertex.normal);

            vertices.push_back(vertex);
        }

        // counter-clockwise winding order
        std::vector<GLuint> indices {
            +2, +0, +1, +2, +1, +3, +4, +5, +6,
            +7, +8, +9, 10, 11, 12, 13, 14, 15
        };

        CreateBuffers(vertices, indices);
    }

    Mesh::Mesh(Primitive object) : Component() {
        switch (object) {
            case Primitive::Sphere:      CreateSphere();    break;
            case Primitive::Cube:        CreateCube();      break;
            case Primitive::Plane:       CreatePlane();     break;
            case Primitive::Quad2D:      Create2DQuad();    break;
            case Primitive::Torus:       CreateTorus();     break;
            case Primitive::Capsule:     CreateCapsule();   break;
            case Primitive::Tetrahedron: CreatePyramid();   break;
            default: {
                CORE_ERROR("Unable to construct the mesh, primitive undefined...");
                throw core::NotImplementedError("Primitive definition not found...");
            }
        }
    }

    Mesh::Mesh(asset_ref<VAO> vao, size_t n_verts)
        : Component(), vao(vao), n_verts(n_verts), n_tris(n_verts / 3) {}

    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) : Component() {
        CreateBuffers(vertices, indices);
        material_id = vao->ID();  // only this ctor will be called when loading external models
    }

    Mesh::Mesh(const asset_ref<Mesh>& mesh_asset) : Mesh(*mesh_asset) {}  // calls copy ctor

    void Mesh::CreateBuffers(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
        vao = MakeAsset<VAO>();
        vbo = MakeAsset<VBO>(vertices.size() * sizeof(Vertex), &vertices[0]);
        ibo = MakeAsset<IBO>(indices.size() * sizeof(GLuint), &indices[0]);

        GLuint vbo_id = vbo->ID();
        GLuint ibo_id = ibo->ID();

        for (GLuint i = 0; i < 8; i++) {
            GLenum va_type = i == 6 ? GL_INT : GL_FLOAT;
            vao->SetVBO(vbo_id, i, va_offset[i], va_size[i], sizeof(Vertex), va_type);
        }

        vao->SetIBO(ibo_id);

        n_verts = vertices.size();
        n_tris = indices.size() / 3;
    }

    void Mesh::Draw() const {
        vao->Draw(GL_TRIANGLES, n_tris * 3);
    }

    void Mesh::DrawQuad() {
        // bufferless rendering allows us to draw a quad without using any mesh data
        // check out: https://trass3r.github.io/coding/2019/09/11/bufferless-rendering.html
        // check out: https://stackoverflow.com/a/59739538/10677643

        if (internal_vao == nullptr) {
            internal_vao = WrapAsset<VAO>();
        }

        internal_vao->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);  // 3 vertices, 3 vertex shader invocations
    }

    void Mesh::DrawGrid() {
        if (internal_vao == nullptr) {
            internal_vao = WrapAsset<VAO>();
        }

        internal_vao->Bind();
        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);  // 6 vertices, 6 invocations
    }

    void Mesh::SetMaterialID(GLuint mid) const {
        material_id = mid;
    }

}
