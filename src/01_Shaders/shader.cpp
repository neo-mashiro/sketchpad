#include "define.h"
#include "utils.h"

Window window{};

GLuint VAO;  // vertex array object
GLuint VBO;  // vertex buffer object
GLuint PO;   // program object

const float vertex_data[] = {
    // position attribute ~ [-1, 1]
    0.0f, 0.75f, 0.0f, 1.0f,
    0.95f * cos(30 * DEG2RAD), -0.95f * sin(30 * DEG2RAD) - 0.2f, 0.0f, 1.0f,
    -0.95f * cos(30 * DEG2RAD), -0.95f * sin(30 * DEG2RAD) - 0.2f, 0.0f, 1.0f,

    // color attribute
     1.0f, 0.0f, 0.0f, 1.0f,  // red
     0.0f, 1.0f, 0.0f, 1.0f,  // green
     0.0f, 0.0f, 1.0f, 1.0f,  // blue
};

void SetupWindow() {
    window.title = "Color Interpolation";
    SetupDefaultWindow();
}

void Init() {
    // create VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // create VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind

    // create shader program
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(PO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);  // position attribute index
    glEnableVertexAttribArray(1);  // color attribute index
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(vertex_data) / 2));

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glUseProgram(0);

    glutSwapBuffers();
    glutPostRedisplay();  // flush the display (continuous updates of the screen)
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
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(PO);
    glDeleteVertexArrays(1, &VAO);
}