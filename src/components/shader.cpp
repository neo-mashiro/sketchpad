#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "components/shader.h"

namespace components {

    // construct the shader program by compiling from shader sources
    Shader::Shader(const std::string& source_path) : id(0), source_path(source_path) {
        CORE_ASERT(core::Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // load all shaders from the source path
        LoadShader(GL_VERTEX_SHADER);
        LoadShader(GL_GEOMETRY_SHADER);
        LoadShader(GL_FRAGMENT_SHADER);
        LoadShader(GL_COMPUTE_SHADER);
        LoadShader(GL_TESS_CONTROL_SHADER);
        LoadShader(GL_TESS_EVALUATION_SHADER);

        // link all shaders to create the shader program
        LinkShaders();

        // clean up shader caches
        std::for_each(shaders.begin(), shaders.end(), glDeleteShader);
    }

    // construct the shader program by loading from a pre-compiled shader binary
    Shader::Shader(const std::string& binary_path, GLenum format) : id(0), source_path("") {
        CORE_ASERT(core::Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        CORE_INFO("Loading pre-compiled shader program from {0} ...", binary_path);

        std::ifstream in_stream(binary_path.c_str(), std::ios::binary);
        std::istreambuf_iterator<char> iter_start(in_stream), iter_end;
        std::vector<char> buffer(iter_start, iter_end);
        in_stream.close();

        GLuint program_id = glCreateProgram();
        glProgramBinary(program_id, format, buffer.data(), buffer.size());

        GLint status;
        glGetProgramiv(program_id, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            GLint info_log_length;
            glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);

            std::string info_log;
            info_log.resize(info_log_length, ' ');
            glGetProgramInfoLog(program_id, info_log_length, NULL, &info_log[0]);

            CORE_ERROR("Failed to load shader binary, failure reason: {0}", info_log);
            glDeleteProgram(program_id);

            CORE_ERROR("Are you sure the shader binary format number is correct?");
            CORE_ERROR("Are you loading a binary saved by a different platform or driver version?");

            std::cin.get();  // pause the console before exiting so that we can read error messages
            exit(EXIT_FAILURE);
        }

        id = program_id;
    }

    Shader::~Shader() {
        CORE_ASERT(core::Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // log message to the console so that we are aware of the *hidden* destructor calls
        // this can be super useful in case our data accidentally goes out of scope
        if (id > 0) {
            CORE_WARN("Deleting shader program (id = {0})!", id);
        }

        glDeleteProgram(id);
    }

    Shader::Shader(Shader&& other) noexcept {
        *this = std::move(other);
    }

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            glDeleteProgram(id);
            id = 0;

            std::swap(id, other.id);
            std::swap(source_path, other.source_path);
            std::swap(shaders, other.shaders);
        }

        return *this;
    }

    void Shader::Bind() const {
        CORE_ASERT(id > 0, "Attempting to use an invalid shader (id <= 0)!");
        glUseProgram(id);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    // save the compiled shader binary to the source folder on disk
    void Shader::Save() const {
        if (source_path.empty()) {
            CORE_ERROR("Shader binary already exists, please delete it before saving ...");
            return;
        }

        GLint formats;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
        CORE_TRACE("Number of shader binary formats supported: {0}", formats);

        if (formats <= 0) {
            CORE_WARN("No binary formats supported, failed to save shader binary.");
            return;
        }

        GLint binary_length;
        glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &binary_length);
        CORE_TRACE("Retrieving shader binary length ... : {0}", binary_length);

        GLenum binary_format;
        std::vector<GLubyte> buffer(binary_length);
        glGetProgramBinary(id, binary_length, NULL, &binary_format, buffer.data());

        std::string filepath = source_path + "\\" + std::to_string(binary_format) + ".bin";
        CORE_TRACE("Saving compiled shader program to {0} ...", filepath);

        std::ofstream out_stream(filepath.c_str(), std::ios::binary);
        out_stream.write(reinterpret_cast<char*>(buffer.data()), binary_length);
        out_stream.close();
    }

    void Shader::LoadShader(GLenum type) {
        // this line may cause access violation if OpenGL context has not been setup
        GLuint shader_id = glCreateShader(type);

        std::string file("");

        switch (type) {
            case GL_VERTEX_SHADER:          file = source_path + "\\vertex.glsl";   break;
            case GL_GEOMETRY_SHADER:        file = source_path + "\\geometry.glsl"; break;
            case GL_FRAGMENT_SHADER:        file = source_path + "\\fragment.glsl"; break;
            case GL_COMPUTE_SHADER:         file = source_path + "\\compute.glsl";  break;
            case GL_TESS_CONTROL_SHADER:    file = source_path + "\\tess-ct.glsl";  break;
            case GL_TESS_EVALUATION_SHADER: file = source_path + "\\tess-ev.glsl";  break;

            default:
                CORE_ERROR("Unable to load shader, invalid shader type {0} ... ", type);
                break;
        }

        std::string shader_code;
        std::ifstream file_stream(file, std::ios::in);

        if (file_stream.is_open()) {
            std::stringstream buffer;
            buffer << file_stream.rdbuf();
            shader_code = buffer.str();
            file_stream.close();
        }
        else {
            return;  // shader file does not exist, skip this type of shader
        }

        CORE_INFO("Compiling shader source: {0}", file.c_str());

        const char* shader = shader_code.c_str();
        glShaderSource(shader_id, 1, &shader, nullptr);
        glCompileShader(shader_id);

        GLint status;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE) {
            GLint info_log_length;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);

            GLchar* info_log = new GLchar[info_log_length + 1];
            glGetShaderInfoLog(shader_id, info_log_length, NULL, info_log);

            CORE_ERROR("Failed to compile shader: {0}", info_log);
            delete[] info_log;
            glDeleteShader(shader_id);  // prevent shader leak

            std::cin.get();  // pause the console before exiting so that we can read error messages
            exit(EXIT_FAILURE);
        }

        shaders.push_back(shader_id);
    }

    void Shader::LinkShaders() {
        CORE_INFO("Linking shader files...");

        GLuint program_id = glCreateProgram();

        for (auto&& shader : shaders) {
            if (shader > 0) {  // invalid shader ids are discarded
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

            CORE_ERROR("Failed to link shaders: {0}", info_log);
            delete[] info_log;

            std::cin.get();  // pause the console before exiting so that we can read error messages
            exit(EXIT_FAILURE);
        }

        for (auto&& shader : shaders) {
            if (shader > 0) {
                glDetachShader(program_id, shader);
            }
        }

        id = program_id;
    }

}
