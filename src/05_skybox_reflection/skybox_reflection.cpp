#include "define.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Window window{};

GLuint VAO, VBO;
GLuint skybox_VAO, skybox_VBO, skybox_texture;

GLuint PO;   // shader program
GLuint SPO;  // skybox shader program

glm::vec3 camera_position;
glm::mat4 M, V, P;

static const float skybox_vertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

static const float vertices[] = {
    // positions          // normals
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

void LoadFace(const std::string& path, GLenum face) {
    stbi_set_flip_vertically_on_load(0);

    int width, height, channels;
    unsigned char* buffer = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!buffer) {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    else {
        glTexImage2D(face, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }

    stbi_image_free(buffer);
}

GLuint LoadCubeMap(const std::string& path) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    LoadFace(path + "posx.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
    LoadFace(path + "posy.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
    LoadFace(path + "posz.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
    LoadFace(path + "negx.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
    LoadFace(path + "negy.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
    LoadFace(path + "negz.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture_id;
}

void SetupWindow() {
    window.title = "Skybox Reflection";
    SetupDefaultWindow();
}

void Init() {
    // cube setup
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);  // position
    glEnableVertexAttribArray(1);  // normal
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (GLvoid*)(sizeof(float) * 3));

    glBindVertexArray(0);

    // skybox setup
    glGenVertexArrays(1, &skybox_VAO);
    glBindVertexArray(skybox_VAO);

    glGenBuffers(1, &skybox_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    glBindVertexArray(0);

    // load textures and shaders
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);
    SPO = CreateProgram(dir + "skybox\\");
    skybox_texture = LoadCubeMap(dir + "cubemap\\");
    
    // init model view projection
    camera_position = glm::vec3(0, 0.5f, 1);
    P = glm::perspective(glm::radians(90.0f), window.aspect_ratio, 0.1f, 100.0f);
    V = glm::lookAt(camera_position, glm::vec3(0, 0.25f, 0), glm::vec3(0, 1, 0));
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

    // draw the cube first
    glUseProgram(PO);
    glBindVertexArray(VAO);
    glDisable(GL_CULL_FACE);

    {
        float radius = 2.0f;
        float seconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        camera_position = glm::vec3(sin(seconds), 0.5f, cos(seconds)) * radius;

        V = glm::lookAt(camera_position, glm::vec3(0, 0.25f, 0), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(PO, "mvp"), 1, GL_FALSE, glm::value_ptr(P * V * M));

        glUniform3fv(glGetUniformLocation(PO, "camera_pos"), 1, glm::value_ptr(camera_position));
        glUniform1i(glGetUniformLocation(PO, "skybox"), 0);  // bind to texture unit 0 (optional)

        glActiveTexture(GL_TEXTURE0);  // activate texture unit 0
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);

        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    // draw the skybox last (it's safe because depth test will fail where pixels are already drawn)
    glUseProgram(SPO);
    glBindVertexArray(skybox_VAO);
    glEnable(GL_CULL_FACE);

    {
        glm::mat4 skybox_V = glm::mat4(glm::mat3(V));  // translation removed from the camera view matrix
        glUniformMatrix4fv(glGetUniformLocation(SPO, "view"), 1, GL_FALSE, glm::value_ptr(skybox_V));
        glUniformMatrix4fv(glGetUniformLocation(SPO, "projection"), 1, GL_FALSE, glm::value_ptr(P));

        glUniform1i(glGetUniformLocation(SPO, "skybox"), 0);  // texture unit 0 of the skybox shader program

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
    glDeleteTextures(1, &skybox_texture);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &skybox_VBO);
    glDeleteProgram(PO);
    glDeleteProgram(SPO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &skybox_VAO);
}