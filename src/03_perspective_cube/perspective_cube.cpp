#include "define.h"
#include "utils.h"

Window window{};

GLuint VAO, VBO, IBO, PO;

glm::mat4 mvp;

static const float vertices[] = {
    // a cube has 8 vertices
    // position attribute
    -1.0f, -1.0f, -1.0f,
    -1.0f, +1.0f, -1.0f,
    +1.0f, +1.0f, -1.0f,
    +1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, +1.0f,
    -1.0f, +1.0f, +1.0f,
    +1.0f, +1.0f, +1.0f,
    +1.0f, -1.0f, +1.0f,

    // color attribute
    0.971f,  0.572f,  0.833f,
    0.359f,  0.583f,  0.152f,
    0.393f,  0.621f,  0.362f,
    0.014f,  0.184f,  0.576f,
    0.583f,  0.771f,  0.014f,
    0.543f,  0.021f,  0.978f,
    0.435f,  0.602f,  0.223f,
    0.055f,  0.953f,  0.042f
};

static const GLuint indices[] = {
    // a cube has 6 sides, 12 triangles
    0, 1, 2,
    0, 2, 3,
    4, 5, 6,
    4, 6, 7,
    0, 4, 7,
    0, 7, 3,
    1, 5, 6,
    1, 6, 2,
    0, 4, 5,
    0, 5, 1,
    3, 7, 6,
    3, 6, 2
};

void ModelViewProjection() {
    // perspective view, 45 degrees FoV, 0.1 near clip, 100 far clip
    glm::mat4 P = glm::perspective(glm::radians(45.0f), window.aspect_ratio, 0.1f, 100.0f);

    // camera is at (3, 3, 3), look at (0, 0, 0), where the up direction is (0, 1, 0)
    glm::mat4 V = glm::lookAt(
        glm::vec3(3, 3, 3),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );

    // model space is simply the viewing cube centered at the origin
    glm::mat4 M = glm::mat4(1.0f);

    mvp = P * V * M;  // multiplication order must be reversed
}

void SetupWindow() {
    window.title = "Perspective Cube";
    SetupDefaultWindow();
}

void Init() {
    // create VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // create VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);  // position
    glEnableVertexAttribArray(1);  // color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 8));
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind VBO, this is optional (actually not desired)

    // create IBO
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // you must NOT unbind the IBO before VAO is unbound

    glBindVertexArray(0);  // unbind the VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // now it's safe to unbind IBO, but not recommended

    // create shader program
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);

    // init the MVP matrix
    ModelViewProjection();

    // face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    // depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);  // enable writing into the depth buffer
    glDepthFunc(GL_LEQUAL);  // use GL_LEQUAL instead of GL_LESS to allow for multi-pass algorithms
    glDepthRange(0.0f, 1.0f);  // define depth range, [0.0 ~ 1.0] = [near ~ far]
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(PO);
    glBindVertexArray(VAO);

    {
        glUniformMatrix4fv(glGetUniformLocation(PO, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, (GLvoid*)0);
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
    glDeleteBuffers(1, &IBO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(PO);
    glDeleteVertexArrays(1, &VAO);
}