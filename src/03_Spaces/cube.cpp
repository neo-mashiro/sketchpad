#include "define.h"
#include "utils.h"

Window window{};

GLuint VAO;  // vertex array object
GLuint VBO;  // vertex buffer object
GLuint IBO;  // index buffer object
GLuint PO;   // program object

GLuint mvp_uid;
glm::mat4 MVP;

// a cube has 8 vertices
static const float vertex_data[] = {
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

// a cube has 6 sides, 12 triangles
static const GLuint index_data[] = {
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

glm::mat4 ModelViewProjection() {
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
    return P * V * M;
}

void SetupWindow() {
    window.title = "Perspective Cube";
    SetupDefaultWindow(&window);
}

void Init() {
    // create VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // create VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // create IBO
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_data), index_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // context bindings
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);  // position
    glEnableVertexAttribArray(1);  // color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * 8));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    glBindVertexArray(0);  // unbind the VAO

    // create shader program
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);

    // query uniform location
    mvp_uid = glGetUniformLocation(PO, "MVP");

    // init the MVP matrix
    MVP = ModelViewProjection();

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
        glUniformMatrix4fv(mvp_uid, 1, GL_FALSE, glm::value_ptr(MVP));
        glDrawElements(GL_TRIANGLES, std::size(index_data), GL_UNSIGNED_INT, (void*)0);
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
