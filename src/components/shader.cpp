#include "pch.h"

#include <type_traits>
#include "core/app.h"
#include "core/log.h"
#include "components/shader.h"
#include "components/component.h"

using namespace core;

namespace components {

    // optimize context switching by avoiding unnecessary binds and unbinds
    static GLuint prev_bound_shader_id = 0;  // keep track of the current rendering state

    ///////////////////////////////////////////////////////////////////////////////////////////////

    Shader::Shader(const std::string& source_path) : id(0), source_path(source_path) {
        CORE_ASERT(Application::GLContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        CORE_INFO("Compiling shader source: {0}", source_path);

        LoadShader(GL_VERTEX_SHADER);
        LoadShader(GL_GEOMETRY_SHADER);
        LoadShader(GL_FRAGMENT_SHADER);
        LoadShader(GL_COMPUTE_SHADER);
        LoadShader(GL_TESS_CONTROL_SHADER);
        LoadShader(GL_TESS_EVALUATION_SHADER);

        CORE_INFO("Linking compiled shader files...");

        LinkShaders();

        // clean up shader caches
        std::for_each(shaders.begin(), shaders.end(), glDeleteShader);
    }

    Shader::Shader(const std::string& binary_path, GLenum format) : id(0), source_path("") {
        CORE_ASERT(Application::GLContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        CORE_INFO("Loading pre-compiled shader program from {0} ...", binary_path);

        // construct the shader program by loading from a pre-compiled shader binary
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
        CORE_WARN("Deleting shader program (id = {0})...", id);
        glDeleteProgram(id);

        // reset the global static field of tracked shader id on each destructor call.
        // this may introduce a few extra bindings (only if the scene destroys shaders
        // on the fly, which is rare), but helps ensure safety when we switch scenes.
        prev_bound_shader_id = 0;
    }

    Shader::Shader(Shader&& other) noexcept {
        *this = std::move(other);
    }

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            id = prev_bound_shader_id = 0;
            std::swap(id, other.id);
            std::swap(source_path, other.source_path);
            std::swap(shaders, other.shaders);
        }

        return *this;
    }

    void Shader::Bind() const {
        // keep track of the previously bound shader id to avoid unnecessary binds
        if (id != prev_bound_shader_id) {
            glUseProgram(id);
            prev_bound_shader_id = id;
        }
    }

    void Shader::Unbind() const {
        glUseProgram(0);
        prev_bound_shader_id = 0;
    }

    void Shader::Save() const {
        // save the compiled shader binary to the source folder on disk
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

    template<typename T>
    void Shader::SetUniform(GLuint location, const T& val) const {
        using namespace glm;

        /**/ if constexpr (std::is_same_v<T, bool>)   { glProgramUniform1i(id, location, static_cast<int>(val)); }
        else if constexpr (std::is_same_v<T, int>)    { glProgramUniform1i(id, location, val); }
        else if constexpr (std::is_same_v<T, float>)  { glProgramUniform1f(id, location, val); }
        else if constexpr (std::is_same_v<T, GLuint>) { glProgramUniform1ui(id, location, val); }
        else if constexpr (std::is_same_v<T, vec2>)   { glProgramUniform2fv(id, location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec3>)   { glProgramUniform3fv(id, location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec4>)   { glProgramUniform4fv(id, location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, mat2>)   { glProgramUniformMatrix2fv(id, location, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat3>)   { glProgramUniformMatrix3fv(id, location, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat4>)   { glProgramUniformMatrix4fv(id, location, 1, GL_FALSE, &val[0][0]); }
        else {
            static_assert(const_false<T>, "Unspecified template uniform type T ...");
        }
    }

    void Shader::LoadShader(GLenum type) {
        // this line may cause access violation if OpenGL context has not been setup
        GLuint shader_id = glCreateShader(type);

        std::string shader_def;

        switch (type) {
            case GL_VERTEX_SHADER:          shader_def = "vertex_shader";          break;
            case GL_GEOMETRY_SHADER:        shader_def = "geometry_shader";        break;
            case GL_FRAGMENT_SHADER:        shader_def = "fragment_shader";        break;
            case GL_COMPUTE_SHADER:         shader_def = "compute_shader";         break;
            case GL_TESS_CONTROL_SHADER:    shader_def = "tess_control_shader";    break;
            case GL_TESS_EVALUATION_SHADER: shader_def = "tess_evaluation_shader"; break;
            default:
                CORE_ERROR("Unable to load shader, invalid shader type {0} ... ", type);
                break;
        }

        std::ifstream file_stream(source_path, std::ios::in);

        if (!file_stream.is_open()) {
            CORE_ERROR("Unable to read shader file {0} ... ", source_path);
            return;
        }

        std::stringstream buffer;
        std::string line;
        bool defined = false;

        // the file to include must be in the same directory or parent directory of the shader file
        std::string include_dir = source_path.substr(0, source_path.rfind("\\"));
        std::string parent_dir = std::filesystem::path(include_dir).parent_path().string();

        while (getline(file_stream, line)) {
            // replace "#include" with the contents of the include file and copy-paste to the line
            if (size_t pos = line.find("#include "); pos != std::string::npos) {
                std::string include_file = line.substr(pos + 9);
                std::string include_path = include_dir + "\\" + include_file;

                // if cannot find the include file in the source directory, search the parent directory
                if (auto path = std::filesystem::path(include_path); !std::filesystem::exists(path)) {
                    include_path = parent_dir + "\\" + include_file;
                }

                std::ifstream incl_stream(include_path, std::ios::in);

                if (incl_stream.is_open()) {
                    std::stringstream incl_buff;
                    incl_buff << incl_stream.rdbuf();
                    line = incl_buff.str();
                    incl_stream.close();
                }
                else {
                    CORE_ERROR("Unable to include file {0} in shader {1} ... ", include_file, source_path);
                    continue;  // cannot open the include file, discard or skip the line
                }
            }

            // insert "#define" before the first occurrence of "#ifdef"
            else if (!defined && line.find("#ifdef " + shader_def) != std::string::npos) {
                buffer << "#define " << shader_def << '\n';
                defined = true;
            }

            buffer << line << '\n';
        }

        file_stream.close();

        if (!defined) {
            return;  // this type of shader is not defined in the shader, skip
        }

        std::string shader_code = buffer.str();
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

        if (this->id = program_id; this->id <= 0) {
            CORE_WARN("Invalid shader program, results of shader execution will be undefined");
        }
    }

    void Shader::Sync(GLbitfield barriers) {
        glMemoryBarrier(barriers);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ComputeShader::ComputeShader(const std::string& source_path)
        : Shader(source_path) {}

    ComputeShader::ComputeShader(const std::string& binary_path, GLenum format)
        : Shader(binary_path, format) {}

    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
        : Shader(std::move(other)) {}

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
        Shader::operator=(std::move(other));
        return *this;
    }

    void ComputeShader::Bind() const {
        Shader::Bind();
    }

    void ComputeShader::Unbind() const {
        Shader::Unbind();
    }

    void ComputeShader::Save() const {
        Shader::Save();
    }

    void ComputeShader::Dispatch(GLuint nx, GLuint ny, GLuint nz) const {
        // only the number of work groups is defined by the user and is validated here, but the user
        // must also ensure that the total number of invocations within a work group (sx * sy * sz)
        // is <= `app.cs_max_invocations`, and that the local size of each work group (defined in the
        // shader by `layout(sx, sy, sz)`) does not exceed the size limit in every dimension.

        static GLuint cs_nx = Application::GetInstance().cs_nx;
        static GLuint cs_ny = Application::GetInstance().cs_ny;
        static GLuint cs_nz = Application::GetInstance().cs_nz;

        CORE_ASERT(nx >= 1 && nx <= cs_nx, "Invalid compute space size x: {0}", nx);
        CORE_ASERT(ny >= 1 && ny <= cs_ny, "Invalid compute space size y: {0}", ny);
        CORE_ASERT(nz >= 1 && nz <= cs_nz, "Invalid compute space size z: {0}", nz);

        glDispatchCompute(nx, ny, nz);
    }

    void ComputeShader::SyncWait(GLbitfield barriers) const {
        glMemoryBarrier(barriers);  // sync here to ensure all writes are complete
    }

    template<typename T>
    void ComputeShader::SetUniform(GLuint location, const T& val) const {
        Shader::SetUniform<T>(location, val);
    }

    // explicit template functon instantiation
    template void ComputeShader::SetUniform<int>(GLuint location, const int& val) const;
    template void ComputeShader::SetUniform<bool>(GLuint location, const bool& val) const;
    template void ComputeShader::SetUniform<float>(GLuint location, const float& val) const;
    template void ComputeShader::SetUniform<GLuint>(GLuint location, const GLuint& val) const;
    template void ComputeShader::SetUniform<glm::vec2>(GLuint location, const glm::vec2& val) const;
    template void ComputeShader::SetUniform<glm::vec3>(GLuint location, const glm::vec3& val) const;
    template void ComputeShader::SetUniform<glm::vec4>(GLuint location, const glm::vec4& val) const;
    template void ComputeShader::SetUniform<glm::mat2>(GLuint location, const glm::mat2& val) const;
    template void ComputeShader::SetUniform<glm::mat3>(GLuint location, const glm::mat3& val) const;
    template void ComputeShader::SetUniform<glm::mat4>(GLuint location, const glm::mat4& val) const;

}
