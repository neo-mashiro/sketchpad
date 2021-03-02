#include "define.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Window window{};

GLuint VAO, VBO, IBO, PO;
GLuint base, overlay;  // textures

glm::mat4 M, V, P;

std::vector<glm::vec3> positions;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;
std::vector<unsigned int> indices;

std::vector<float> vertices;

void CreateSphereMesh() {
    // mesh grid size
    unsigned int n_rows = 500;
    unsigned int n_cols = 500;

    for (unsigned int col = 0; col <= n_cols; ++col) {
        for (unsigned int row = 0; row <= n_rows; ++row) {
            float u = (float)row / (float)n_rows;
            float v = (float)col / (float)n_cols;
            float x = cos(u * PI * 2) * sin(v * PI);
            float y = cos(v * PI);
            float z = sin(u * PI * 2) * sin(v * PI);

            positions.push_back(glm::vec3(x, y, z));
            uvs.push_back(glm::vec2(u, v));
            normals.push_back(glm::vec3(x, y, z));  // sphere centered at the origin, normal = position
        }
    }

    for (unsigned int col = 0; col < n_cols; ++col) {
        for (unsigned int row = 0; row < n_rows; ++row) {
            // counter-clockwise
            indices.push_back((col + 1) * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row + 1);

            // counter-clockwise
            indices.push_back((col + 1) * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row + 1);
            indices.push_back((col + 1) * (n_rows + 1) + row + 1);
        }
    }

    for (unsigned int i = 0; i < positions.size(); ++i) {
        vertices.push_back(positions[i].x);
        vertices.push_back(positions[i].y);
        vertices.push_back(positions[i].z);
        vertices.push_back(uvs[i].x);
        vertices.push_back(uvs[i].y);
        vertices.push_back(normals[i].x);
        vertices.push_back(normals[i].y);
        vertices.push_back(normals[i].z);
    }
}

void LoadTexture(const std::string& path) {
    stbi_set_flip_vertically_on_load(false);

    int width, height, n_channels;
    unsigned char* buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);

    if (!buffer) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(buffer);
        return;
    }

    if (n_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    }
    else if (n_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
    
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(buffer);
}

void SetupWindow() {
    window.title = "Multiple Textures Blending";
    SetupDefaultWindow();
}

void Init() {
    CreateSphereMesh();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);  // position
    glEnableVertexAttribArray(1);  // uv
    glEnableVertexAttribArray(2);  // normal
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 5));

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);

    // load base texture, generate mipmaps
    glGenTextures(1, &base);
    glBindTexture(GL_TEXTURE_2D, base);  // now the lines below will be tied to base

    LoadTexture(dir + "textures\\color.jpg");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering

    glBindTexture(GL_TEXTURE_2D, 0);

    // load normal map
    glGenTextures(1, &overlay);
    glBindTexture(GL_TEXTURE_2D, overlay);  // now the lines below will be tied to normal_map

    LoadTexture(dir + "textures\\normal.jpg");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    // set texture uniforms
    glUseProgram(PO);
    glUniform1i(glGetUniformLocation(PO, "base"), 0);     // bind to texture unit 0
    glUniform1i(glGetUniformLocation(PO, "overlay"), 1);  // bind to texture unit 1

    // init model view projection
    P = glm::perspective(glm::radians(90.0f), window.aspect_ratio, 0.1f, 100.0f);
    V = glm::lookAt(glm::vec3(0, 0.5f, 2.5f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    M = glm::mat4(1.0f);

    // face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(PO);
    glBindVertexArray(VAO);

    {
        M = glm::rotate(M, glm::radians(0.1f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(PO, "mvp"), 1, GL_FALSE, glm::value_ptr(P * V * M));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, base);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, overlay);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
    glutPostRedisplay();
}

void Reshape(int width, int height) {
    DefaultReshapeCallback(width, height);
}

void Keyboard(unsigned char key, int x, int y) {
    DefaultKeyboardCallback(key, x, y);
}

void Mouse(int button, int state, int x, int y) {};
void Idle(void) {};
void Motion(int x, int y) {};
void PassiveMotion(int x, int y) {};

void Cleanup() {
    glDeleteTextures(1, &base);
    glDeleteTextures(1, &overlay);
    glDeleteProgram(PO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
    glDeleteVertexArrays(1, &VAO);
}