#include "utils.h"

/// <summary>
/// Given a shader type and file path, compiles the shader file and returns a shader id.
/// If the path does not exist, deems the shader file optional and returns 0.
/// </summary>
static GLuint LoadShader(GLenum shader_type, const std::string& shader_file_path) {
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
        glDeleteShader(shader_id);  // prevent shader leak
        
        std::cin.get();  // pause the console before exiting so that we can read error messages
        exit(EXIT_FAILURE);
    }

    return shader_id;
}

/// <summary>
/// Links a vector of shaders together to create a program. Negative shader ids will be discarded.
/// </summary>
static GLuint LinkShaders(const std::vector<GLuint>& shaders) {
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

/// <summary>
/// Loads and links all shaders from the path to create a program.
/// </summary>
GLuint CreateProgram(const std::string& shader_path) {
    GLuint VS = LoadShader(GL_VERTEX_SHADER, shader_path + "vertex.glsl");
    GLuint FS = LoadShader(GL_FRAGMENT_SHADER, shader_path + "fragment.glsl");
    GLuint GS = LoadShader(GL_GEOMETRY_SHADER, shader_path + "geometry.glsl");
    GLuint CS = LoadShader(GL_COMPUTE_SHADER, shader_path + "compute.glsl");

    // create PO
    std::vector<GLuint> shaders{ VS, FS, GS, CS };
    GLuint program_id = LinkShaders(shaders);

    // clean up
    std::for_each(shaders.begin(), shaders.end(), glDeleteShader);

    return program_id;
}


void SetupDefaultWindow(Window* window) {
    window->aspect_ratio = (float)window->width / window->height;
    window->pos_x = (glutGet(GLUT_SCREEN_WIDTH) - window->width) / 2;
    window->pos_y = (glutGet(GLUT_SCREEN_HEIGHT) - window->height) / 2;
    window->display_mode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
}

void DefaultReshapeCallback(int width, int height) {
    // keep original aspect ratio
    GLsizei viewport_w = width <= height ? width : height;
    GLsizei viewport_h = viewport_w / window.aspect_ratio;

    // center viewport relative to the window
    GLint pos_x = (width - viewport_w) / 2;
    GLint pos_y = (height - viewport_h) / 2;

    glViewport(pos_x, pos_y, viewport_w, viewport_h);
}

void DefaultKeyboardCallback(unsigned char key, int x, int y) {
    if (key == VK_ESCAPE) {
        int button_id = MessageBox(
            NULL,
            (LPCWSTR)L"Do you want to close the window?",
            (LPCWSTR)L"Sketchpad Canvas",
            MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_SETFOREGROUND
        );

        switch (button_id) {
            case IDOK:
                glutLeaveMainLoop();
                break;
            case IDCANCEL:
                break;
        }
    }
}