#include "mesh.h"

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

    void BindBuffer() {
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

    void BindTexture(const Shader& shader, bool layout_bind = false) const {
        // assume that we only have one texture for each specific texture type of the following:
        // ambient, diffuse, specular, emission, normal, height, bump, metallic, roughness, opacity

        // let's also assume that the sampler uniforms names in GLSL exactly match texture types
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);  // activate texture unit `i`

            // if we know what the textures are and in what order they appear in the std::vector,
            // we can set up sampler bindings directly in GLSL, there's no need to set uniform in C++
            // e.g.: std::vector<Texture> textures = { ambient, diffuse };
            //       => we know the ambient map goes to texture unit 0, diffuse map goes to 1
            //       => so in the fragment shader we can specify the binding points:
            //       ...
            //       layout(binding = 0) uniform sampler2D ambient;
            //       layout(binding = 1) uniform sampler2D diffuse;
            //       ...
            if (!layout_bind) {
                shader.SetInt(textures[i].type, i);  // set sampler uniform
            }

            glBindTexture(textures[i].target, textures[i].id);  // bind texture in this unit
        }
    }

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

    void CreateSphere(float radius = 1.0f) {
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

    void CreateCube(float size = 1.0f) {
        // define a cube of size 1, which has 24 vertices (with redefinition)
        const static int n_vertices = 24;

        const static GLfloat c_positions[] = {
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f, +1.0f,
            +1.0f, -1.0f, -1.0f,
            -1.0f, +1.0f, -1.0f,
            -1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, +1.0f, -1.0f,
            +1.0f, +1.0f, -1.0f,
            +1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, +1.0f,
            -1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, +1.0f,
            +1.0f, -1.0f, +1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, +1.0f,
            -1.0f, +1.0f, +1.0f,
            -1.0f, +1.0f, -1.0f,
            +1.0f, -1.0f, -1.0f,
            +1.0f, -1.0f, +1.0f,
            +1.0f, +1.0f, +1.0f,
            +1.0f, +1.0f, -1.0f
        };

        const static GLfloat c_normals[] = {
            +0.0f, -1.0f, +0.0f,
            +0.0f, -1.0f, +0.0f,
            +0.0f, -1.0f, +0.0f,
            +0.0f, -1.0f, +0.0f,
            +0.0f, +1.0f, +0.0f,
            +0.0f, +1.0f, +0.0f,
            +0.0f, +1.0f, +0.0f,
            +0.0f, +1.0f, +0.0f,
            +0.0f, +0.0f, -1.0f,
            +0.0f, +0.0f, -1.0f,
            +0.0f, +0.0f, -1.0f,
            +0.0f, +0.0f, -1.0f,
            +0.0f, +0.0f, +1.0f,
            +0.0f, +0.0f, +1.0f,
            +0.0f, +0.0f, +1.0f,
            +0.0f, +0.0f, +1.0f,
            -1.0f, +0.0f, +0.0f,
            -1.0f, +0.0f, +0.0f,
            -1.0f, +0.0f, +0.0f,
            -1.0f, +0.0f, +0.0f,
            +1.0f, +0.0f, +0.0f,
            +1.0f, +0.0f, +0.0f,
            +1.0f, +0.0f, +0.0f,
            +1.0f, +0.0f, +0.0f
        };

        const static GLfloat c_uvs[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
            0.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f
        };

        const static GLuint c_indices[] = {
            +0, +2, +1,
            +0, +3, +2,
            +4, +5, +6,
            +4, +6, +7,
            +8, +9, 10,
            +8, 10, 11,
            12, 15, 14,
            12, 14, 13,
            16, 17, 18,
            16, 18, 19,
            20, 23, 22,
            20, 22, 21
        };

        for (int i = 0; i < n_vertices; i++) {
            int i3 = i * 3, i31 = i * 3 + 1, i32 = i * 3 + 2;
            int i2 = i * 2, i21 = i * 2 + 1;

            Vertex vertex {};
            vertex.position  = glm::vec3(c_positions[i3], c_positions[i31], c_positions[i32]) * size;
            vertex.normal    = glm::vec3(c_normals[i3], c_normals[i31], c_normals[i32]);
            vertex.uv        = glm::vec2(c_uvs[i2], c_uvs[i21]);  // keep in [0, 1] range
            vertex.tangent   = glm::vec3(0.0f);
            vertex.bitangent = glm::vec3(0.0f);

            vertices.push_back(vertex);
        }

        // counter-clockwise winding order
        for (auto index : c_indices) {
            indices.push_back(index);
        }
    }

    void CreateCylinder(float radius = 1.0f) {
        // TODO: recursively divide the side quads of a cube to approximate a cylinder
        auto normalize = [](glm::vec3 vertex) {
            float k = sqrt(2) / sqrt(vertex.x * vertex.x + vertex.z * vertex.z);
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

        // TODO: loop through vertices in circular order, find indices of triangles of the top/bottom circle
    }

    void CreatePlane(float size = 1.0f, float elevation = -2.0f) {
        const static glm::vec3 up   = glm::vec3(0.0f, +1.0f, 0.0f);
        const static glm::vec3 down = glm::vec3(0.0f, -1.0f, 0.0f);
        const static glm::vec3 zero = glm::vec3(0.0f);

        Vertex v_arr[8] = {};

        // positive y face
        v_arr[0] = { glm::vec3(-1.0f * size, elevation, +1.0f * size), up, glm::vec2(0.0f, 0.0f), zero, zero };
        v_arr[1] = { glm::vec3(+1.0f * size, elevation, +1.0f * size), up, glm::vec2(1.0f, 0.0f), zero, zero };
        v_arr[2] = { glm::vec3(+1.0f * size, elevation, -1.0f * size), up, glm::vec2(1.0f, 1.0f), zero, zero };
        v_arr[3] = { glm::vec3(-1.0f * size, elevation, -1.0f * size), up, glm::vec2(0.0f, 1.0f), zero, zero };

        // negative y face
        v_arr[4] = { glm::vec3(-1.0f * size, elevation, +1.0f * size), down, glm::vec2(0.0f, 1.0f), zero, zero };
        v_arr[5] = { glm::vec3(+1.0f * size, elevation, +1.0f * size), down, glm::vec2(1.0f, 1.0f), zero, zero };
        v_arr[6] = { glm::vec3(+1.0f * size, elevation, -1.0f * size), down, glm::vec2(1.0f, 0.0f), zero, zero };
        v_arr[7] = { glm::vec3(-1.0f * size, elevation, -1.0f * size), down, glm::vec2(0.0f, 0.0f), zero, zero };

        for (Vertex v : v_arr) {
            vertices.push_back(v);
        }

        // counter-clockwise winding order
        indices = { 0, 1, 2, 2, 3, 0, 2, 1, 0, 0, 3, 2 };
    }

  public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
        const std::vector<Texture>& textures) : vertices(vertices), indices(indices) {

        if (textures.size() > MAX_TEXTURE_UNITS) {
            // throw std::out_of_range("Exceeded maximum allowed texture units...");
            this->textures = { textures.begin(), textures.begin() + MAX_TEXTURE_UNITS - 1 };
            std::cout << "[WARNING] Exceeded maximum allowed texture units, "\
            "redundant textures are automatically discarded..." << std::endl;
        }
        else {
            this->textures = textures;
        }

        BindBuffer();
    }

    Mesh(Primitive object, const std::vector<Texture>& textures) {
        // populate vertices and indices vector
        switch (object) {
            case Primitive::Sphere:   CreateSphere();   break;
            case Primitive::Cube:     CreateCube();     break;
            case Primitive::Cylinder: CreateCylinder(); break;
            case Primitive::Plane:    CreatePlane();    break;

            default:
                std::cerr << "[ERROR] Undefined primitive mesh..." << std::endl; break;
                std::cin.get();  // pause the console before exiting
                exit(EXIT_FAILURE);
        }

        // store textures up to the GPU limit
        if (textures.size() > MAX_TEXTURE_UNITS) {
            // throw std::out_of_range("Exceeded maximum allowed texture units...");
            this->textures = { textures.begin(), textures.begin() + MAX_TEXTURE_UNITS - 1 };
            std::cout << "[WARNING] Exceeded maximum allowed texture units, "\
            "redundant textures are automatically discarded..." << std::endl;
        }
        else {
            this->textures = textures;
        }

        BindBuffer();
    }

    ~Mesh() {
        for (unsigned int i = 0; i < textures.size(); i++) {
            glDeleteTextures(1, &textures[i].id);
        }

        glDeleteBuffers(1, &IBO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);

        vertices.clear();
        vertices.shrink_to_fit();

        indices.clear();
        indices.shrink_to_fit();

        textures.clear();
        textures.shrink_to_fit();
    }

    void Draw(const Shader& shader, bool layout_bind = false) const {
        // ---------------------------------------------------------------------------------------
        // [Q] it is good practice to bind textures before each draw call, but why?
        // ---------------------------------------------------------------------------------------
        // [A] if we only have one mesh and the textures never change, it suffices to set things up
        //     only once, so we can move the `BindTexture()` call to the constructor, but in practice,
        //     textures may change dynamically before each fragment shader invocation, and it is
        //     commonplace to have hundreds of meshes in a scene, all of which share the same texture
        //     units in OpenGL, if we don't bind before each draw call, textures bound for one mesh
        //     would be applied to all other meshes, unless each mesh uses a disjoint set of units.
        // ---------------------------------------------------------------------------------------
        // [A] as an aside, if we were to boost performance by reducing the number of texture binding
        //     operations, then research into some advanced GLSL stuff and optimization techniques.
        // ---------------------------------------------------------------------------------------

        BindTexture(shader, layout_bind);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // restore to default so that our textures won't accidentally be applied to other meshes (recommended)
        for (unsigned int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
};
