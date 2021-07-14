#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "scene/shader.h"

using namespace core;

namespace scene {

    void Shader::LoadShader(GLenum type, const std::string& filepath) {
        // this line may cause access violation if OpenGL context is not set up
        // machine-level exceptions like this or segmentation fault cannot be caught
        GLuint shader_id = glCreateShader(type);

        std::string shader_code;
        std::ifstream file_stream(filepath, std::ios::in);

        if (file_stream.is_open()) {
            std::stringstream buffer;
            buffer << file_stream.rdbuf();
            shader_code = buffer.str();
            file_stream.close();
        }
        else {
            return;  // path does not exist, skip this shader type
        }

        CORE_INFO("Compiling shader file: {0}", filepath.c_str());

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
        }

        for (auto&& shader : shaders) {
            if (shader > 0) {
                glDetachShader(program_id, shader);
            }
        }

        id = program_id;
    }

    GLint Shader::GetUniformLocation(const std::string& name) const {
        // look up the cache table first
        if (uniform_loc_cache.find(name) != uniform_loc_cache.end()) {
            return uniform_loc_cache[name];
        }

        // if not found in cache, query from GPU (only the first time)
        GLint location = glGetUniformLocation(id, name.c_str());
        if (location == -1) {
            CORE_WARN("Uniform location not found: {0}, the GLSL compiler may have optimized it out", name.c_str());
        }

        uniform_loc_cache[name] = location;  // cache location in memory (including -1)

        // it's ok to pass a location of -1 to the shader, the data passed in will
        // be silently ignored and the specified uniform variable won't be changed
        return location;
    }

    Shader::Shader(const std::string& filepath) {
        Application::GetInstance().CheckOpenGLContext("Shader");

        // load all shaders from the given path
        LoadShader(GL_VERTEX_SHADER, filepath + "vertex.glsl");
        LoadShader(GL_FRAGMENT_SHADER, filepath + "fragment.glsl");
        LoadShader(GL_GEOMETRY_SHADER, filepath + "geometry.glsl");
        LoadShader(GL_COMPUTE_SHADER, filepath + "compute.glsl");

        // link all shaders to create shader program
        LinkShaders();

        // clean up
        std::for_each(shaders.begin(), shaders.end(), glDeleteShader);
    }

    Shader::~Shader() {
        Application::GetInstance().CheckOpenGLContext("~Shader");

        // log friendly message to the console, so that we are aware of the *hidden* destructor calls
        // this is super useful in case our data accidentally goes out of scope, debugging made easier!
        if (id > 0) {
            CORE_WARN("Deleting shader program (id = {0})!", id);
        }

        shaders.clear();
        shaders.shrink_to_fit();
        uniform_loc_cache.clear();

        glDeleteProgram(id);
    }

    Shader::Shader(Shader&& other) noexcept {
        *this = std::move(other);
    }

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            // free this shader program, reset it to a clean null state
            glDeleteProgram(id);
            id = 0;
            shaders.clear();
            shaders.shrink_to_fit();
            uniform_loc_cache.clear();

            // transfer ownership from other to this
            std::swap(id, other.id);
            std::swap(shaders, other.shaders);
            std::swap(uniform_loc_cache, other.uniform_loc_cache);
        }

        return *this;
    }

    void Shader::Bind() const {
        if (id == 0) {
            CORE_WARN("Attempting to use an invalid shader (id = 0)!");
        }
        glUseProgram(id);
    }

    void Shader::Unbind() const {
        glUseProgram(0);
    }

    void Shader::SetBool(const std::string& name, bool value) const {
        glUniform1i(GetUniformLocation(name), static_cast<int>(value));
    }

    void Shader::SetInt(const std::string& name, int value) const {
        glUniform1i(GetUniformLocation(name), value);
    }

    void Shader::SetFloat(const std::string& name, float value) const {
        glUniform1f(GetUniformLocation(name), value);
    }

    void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2fv(GetUniformLocation(name), 1, &value[0]);
    }

    void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(GetUniformLocation(name), 1, &value[0]);
    }

    void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(GetUniformLocation(name), 1, &value[0]);
    }

    void Shader::SetMat2(const std::string& name, const glm::mat2& value) const {
        glUniformMatrix2fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void Shader::SetMat3(const std::string& name, const glm::mat3& value) const {
        glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }

    void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
    }
}
