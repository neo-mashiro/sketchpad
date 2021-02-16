#include <cstdio>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "define.h"

Window window{};

GLuint VAO;  // vertex array object
GLuint VBO;  // vertex buffer object
GLuint PO;   // program object


const GLfloat vertices[] = {
    0.75f, 0.75f, 0.0f, 1.0f,
    0.75f, -0.75f, 0.0f, 1.0f,
    -0.75f, -0.75f, 0.0f, 1.0f,
};


/// <summary>
/// Given a shader type and file path, compiles the shader file and returns a shader id.
/// If the path does not exist, deems the shader file optional and returns 0.
/// </summary>
GLuint LoadShader(GLenum shader_type, const std::string& shader_file_path) {
    GLuint shader_id = glCreateShader(shader_type);
    std::string shader_code;

    std::ifstream file_stream(shader_file_path, std::ios::in);
    if (file_stream.is_open()) {
        std::stringstream buffer;
        buffer << file_stream.rdbuf();
        shader_code = buffer.str();
        file_stream.close();
    }
    else {
        return 0;
    }

    printf("Compiling shader file : %s\n", shader_file_path.c_str());

    const char* shader = shader_code.c_str();
    glShaderSource(shader_id, 1, &shader, NULL);
    glCompileShader(shader_id);

    GLint status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint info_log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

        GLchar* info_log = new GLchar[info_log_length + 1];
        glGetShaderInfoLog(shader_id, info_log_length, NULL, info_log);

        fprintf(stderr, "Failed to compile shader : %s\n", info_log);
        delete[] info_log;
    }

    return shader_id;
}

/// <summary>
/// Links a vector of shaders together to create a program. Negative shader ids will be discarded.
/// </summary>
GLuint LinkShaders(const std::vector<GLuint>& shaders) {
    printf("Linking shader files ...\n");

    GLuint program_id = glCreateProgram();

    for (auto&& shader : shaders) {
        if (shader > 0) {
            glAttachShader(program_id, shader);
        }
    }

    glLinkProgram(program_id);

    GLint status;
    glGetProgramiv(program_id, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint info_log_length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

        GLchar* info_log = new GLchar[info_log_length + 1];
        glGetProgramInfoLog(program_id, info_log_length, NULL, info_log);

        fprintf(stderr, "Failed to link shaders : %s\n", info_log);
        delete[] info_log;
    }

    for (auto&& shader : shaders) {
        if (shader > 0) {
            glDetachShader(program_id, shader);
        }
    }

    return program_id;
}

void SetupWindow() {
    window.title = "Triangle";
    window.pos_x = glutGet(GLUT_SCREEN_WIDTH) / 2 - 256;
    window.pos_y = glutGet(GLUT_SCREEN_HEIGHT) / 2 - 256;
    window.display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
}

void Init() {
    // create VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // create VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind

    // compile shaders
    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";

    GLuint VS = LoadShader(GL_VERTEX_SHADER, dir + "vertex.glsl");
    GLuint FS = LoadShader(GL_FRAGMENT_SHADER, dir + "fragment.glsl");
    GLuint GS = LoadShader(GL_GEOMETRY_SHADER, dir + "geometry.glsl");
    GLuint CS = LoadShader(GL_COMPUTE_SHADER, dir + "compute.glsl");

    // create PO
    std::vector<GLuint> shaders { VS, FS, GS, CS };
    PO = LinkShaders(shaders);

    // clean up
    std::for_each(shaders.begin(), shaders.end(), glDeleteShader);
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(PO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glUseProgram(0);

    glutSwapBuffers();
    //glutPostRedisplay();  // call this at the end if you need continuous updates of the screen
}

void Reshape(int width, int height) {
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    //TODO: keep aspect ratio
}

void Keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case VK_ESCAPE:
            // add messagebox https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messageboxa
            glutLeaveMainLoop();
            return;
    }
}

void Mouse(int button, int state, int x, int y) {};
void Idle(void) {};
void Motion(int x, int y) {};
void PassiveMotion(int x, int y) {};
