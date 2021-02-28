#include "define.h"
#include "utils.h"

Window window{};

GLuint VAO, VBO, PO;

const float vertices[] = {
    // position attribute ~ [-1, 1]
    0.0f, 0.375f, 0.0f, 1.0f,
    0.475f * cos(30 * DEG2RAD), -0.475f * sin(30 * DEG2RAD) - 0.1f, 0.0f, 1.0f,
    -0.475f * cos(30 * DEG2RAD), -0.475f * sin(30 * DEG2RAD) - 0.1f, 0.0f, 1.0f,

    // color attribute
     1.0f, 0.0f, 0.0f, 1.0f,  // red
     0.0f, 1.0f, 0.0f, 1.0f,  // green
     0.0f, 0.0f, 1.0f, 1.0f,  // blue
};

void SetupWindow() {
    window.title = "Fading Rotation";
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind, but is unnecessary

    // create shader program
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateProgram(dir);
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(PO);
    glBindVertexArray(VAO);

    // update the uniform's value
    glUniform1f(glGetUniformLocation(PO, "elapsed_time"), glutGet(GLUT_ELAPSED_TIME) / 1000.0f);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);  // position attribute index
    glEnableVertexAttribArray(1);  // color attribute index
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(vertices) / 2));

    glDrawArrays(GL_TRIANGLES, 0, 3);

    // clean up the context, but this is optional (not desired) since we have only 1 VAO
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
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