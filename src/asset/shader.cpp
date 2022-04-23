#include "pch.h"

#include <fstream>
#include <sstream>
#include <type_traits>
#include "core/app.h"
#include "core/log.h"
#include "asset/shader.h"
#include "utils/ext.h"

namespace asset {

    static GLuint curr_bound_shader = 0;  // keep track of the current rendering state

    ///////////////////////////////////////////////////////////////////////////////////////////////

    Shader::Shader(const std::string& source_path) : IAsset(), source_path(source_path) {
        CORE_INFO("Compiling and linking shader source: {0}", source_path);
        LoadShader(GL_VERTEX_SHADER);
        LoadShader(GL_TESS_CONTROL_SHADER);
        LoadShader(GL_TESS_EVALUATION_SHADER);
        LoadShader(GL_GEOMETRY_SHADER);
        LoadShader(GL_FRAGMENT_SHADER);

        LinkShaders();
    }

    Shader::Shader(const std::string& binary_path, GLenum format) : IAsset(), source_path() {
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

            CORE_ERROR("Are you sure the shader binary's format number is correct?");
            CORE_ERROR("Are you loading a binary compiled from a different driver?");
            throw std::runtime_error("Corrupted shader binary: " + binary_path);
        }

        this->id = program_id;
    }

    Shader::~Shader() {
        Unbind();
        glDeleteProgram(id);
    }

    void Shader::Bind() const {
        if (id != curr_bound_shader) {
            glUseProgram(id);
            curr_bound_shader = id;
        }
    }

    void Shader::Unbind() const {
        if (curr_bound_shader == id) {
            curr_bound_shader = 0;
            glUseProgram(0);
        }
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

    void Shader::Inspect() const {
        if (source_code.empty()) {
            if (source_path.empty()) {
                CORE_WARN("Shader loaded from binary, source code not available");
            }
            else {
                CORE_ERROR("Shader compilation has errors, source code not found...");
            }
            return;
        }

        CORE_TRACE("Inspecting source code for shader {0}: (example shader stage)", id);
        core::Log::GetLogger()->set_pattern("         > %^%v%$");

        std::istringstream isstream(source_code);
        std::string line;
        int line_number = 1;

        while (std::getline(isstream, line)) {
            CORE_DEBUG("{0:03d} | {1}", line_number, line);
            line_number++;
        }

        core::Log::GetLogger()->set_pattern("%^%T > [%L] %v%$");
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

    template<typename T>
    void Shader::SetUniformArray(GLuint location, GLuint size, const std::vector<T>& vec) const {
        for (GLuint i = 0; i < size; ++i) {
            const T& val = vec[i];
            SetUniform(location + i, val);
        }
    }

    void Shader::ReadShader(const std::string& path, const std::string& macro, std::string& output) {
        std::ifstream file_stream = std::ifstream(path, std::ios::in);
        if (!file_stream.is_open()) {
            CORE_ERROR("Unable to read shader file {0} ... ", path);
            return;
        }

        bool defined = false;
        std::string line;

        while (std::getline(file_stream, line)) {
            // lines w/o "#ifdef" and "#include", just append to the string buffer and continue
            if (line.find("#include") == std::string::npos && line.find("#ifdef") == std::string::npos) {
                output.append(line + '\n');
                continue;
            }

            // "#define" the macro only once before the first occurrence of "#ifdef" or "#include"
            if (!defined && !macro.empty()) {
                output += ("#ifndef " + macro + '\n');
                output += ("#define " + macro + '\n');
                output += ("#endif\n\n");
                defined = true;
            }

            if (line.find("#include") != std::string::npos) {
                // recursively resolve "#include", replace the line with the contents of the header file
                auto directory = std::filesystem::path(path).parent_path();
                std::string relative_path = line.substr(10);
                relative_path.pop_back();  // remove the last double quote
                std::string full_include_path = (directory / relative_path).string();

                ReadShader(full_include_path, "", output);  // recursive call doesn't define the macro
            }
            else {
                output.append(line + '\n');  // macro is already defined, "#ifdef" proceeds as normal
            }
        }

        output.append("\n");
        file_stream.close();
    }

    void Shader::LoadShader(GLenum type) {
        std::string macro;
        std::string outbuff;
        outbuff.reserve(8192);

        switch (type) {
            case GL_COMPUTE_SHADER:          macro = "compute_shader";    break;
            case GL_VERTEX_SHADER:           macro = "vertex_shader";     break;
            case GL_TESS_CONTROL_SHADER:     macro = "tess_ctrl_shader";  break;
            case GL_TESS_EVALUATION_SHADER:  macro = "tess_eval_shader";  break;
            case GL_GEOMETRY_SHADER:         macro = "geometry_shader";   break;
            case GL_FRAGMENT_SHADER:         macro = "fragment_shader";   break;
            default: {
                CORE_ERROR("Invalid shader type: {0}", type);
                return;
            }
        }

        ReadShader(source_path, macro, outbuff);

        if (outbuff.find("#ifdef " + macro) == std::string::npos) {
            return;  // this shader type is not defined in the GLSL file, skip
        }

        if (this->source_code.empty()) {
            this->source_code = outbuff;
        }

        const char* c_source_code = outbuff.c_str();
        GLuint shader_id = glCreateShader(type);
        glShaderSource(shader_id, 1, &c_source_code, nullptr);
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
        GLuint pid = glCreateProgram();
        CORE_ASERT(pid > 0, "Cannot create the program object...");

        for (auto&& shader : shaders) {
            if (shader > 0) {  // invalid shader ids are discarded
                glAttachShader(pid, shader);
            }
        }

        glLinkProgram(pid);

        GLint status;
        glGetProgramiv(pid, GL_LINK_STATUS, &status);

        if (status == GL_FALSE) {
            GLint info_log_length;
            glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &info_log_length);

            GLchar* info_log = new GLchar[info_log_length + 1];
            glGetProgramInfoLog(pid, info_log_length, NULL, info_log);

            CORE_ERROR("Failed to link shaders: {0}", info_log);
            delete[] info_log;

            std::cin.get();  // pause the console before exiting so that we can read error messages
            exit(EXIT_FAILURE);
        }

        auto clear_cache = [&pid](GLuint& shader) {
            if (shader > 0) {
                glDetachShader(pid, shader);
                glDeleteShader(shader);
            }
        };
        
        utils::ranges::for_each(shaders, clear_cache);
        this->shaders.clear();
        this->id = pid;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    CShader::CShader(const std::string& source_path) : Shader() {
        this->source_path = source_path;
        this->source_code = "";

        CORE_INFO("Compiling and linking compute shader: {0}", source_path);
        LoadShader(GL_COMPUTE_SHADER);
        LinkShaders();

        GLint local_size[3];
        glGetProgramiv(id, GL_COMPUTE_WORK_GROUP_SIZE, local_size);
        this->local_size_x = local_size[0];
        this->local_size_y = local_size[1];
        this->local_size_z = local_size[2];
    }

    CShader::CShader(const std::string& binary_path, GLenum format)
    try : Shader(binary_path, format) {
        // this is the ctor body, we won't reach here if `try` failed in the initializer list
        GLint local_size[3];
        glGetProgramiv(id, GL_COMPUTE_WORK_GROUP_SIZE, local_size);
        this->local_size_x = local_size[0];
        this->local_size_y = local_size[1];
        this->local_size_z = local_size[2];
    }
    // this is the `catch` block in the initializer list, not the ctor body
    catch (const std::runtime_error& e) {
        CORE_ERROR("Cannot load compute shader: {0}", e.what());
        throw std::runtime_error("Compute shader compilation failed...");
    }

    void CShader::Dispatch(GLuint nx, GLuint ny, GLuint nz) const {
        static GLuint max_invocations = core::Application::GetInstance().cs_max_invocations;
        CORE_ASERT(local_size_x * local_size_y * local_size_z <= max_invocations, "Compute size overflow!");

        static GLuint cs_sx = core::Application::GetInstance().cs_sx;
        static GLuint cs_sy = core::Application::GetInstance().cs_sy;
        static GLuint cs_sz = core::Application::GetInstance().cs_sz;

        CORE_ASERT(local_size_x <= cs_sx, "Maxed out local work group size x: {0}", local_size_x);
        CORE_ASERT(local_size_y <= cs_sy, "Maxed out local work group size y: {0}", local_size_y);
        CORE_ASERT(local_size_z <= cs_sz, "Maxed out local work group size z: {0}", local_size_z);

        static GLuint cs_nx = core::Application::GetInstance().cs_nx;
        static GLuint cs_ny = core::Application::GetInstance().cs_ny;
        static GLuint cs_nz = core::Application::GetInstance().cs_nz;

        CORE_ASERT(nx >= 1 && nx <= cs_nx, "Invalid number of work groups x: {0}", nx);
        CORE_ASERT(ny >= 1 && ny <= cs_ny, "Invalid number of work groups y: {0}", ny);
        CORE_ASERT(nz >= 1 && nz <= cs_nz, "Invalid number of work groups z: {0}", nz);

        glDispatchCompute(nx, ny, nz);
    }

    void CShader::SyncWait(GLbitfield barriers) const {
        glMemoryBarrier(barriers);  // sync to ensure all writes are complete
    }
    
    // explicit template function instantiation using X macro
    #define INSTANTIATE_TEMPLATE(T) \
        template void Shader::SetUniform<T>(GLuint location, const T& val) const; \
        template void Shader::SetUniformArray<T>(GLuint location, GLuint size, const std::vector<T>& vec) const;

    using namespace glm;

    INSTANTIATE_TEMPLATE(int)
    INSTANTIATE_TEMPLATE(bool)
    INSTANTIATE_TEMPLATE(float)
    INSTANTIATE_TEMPLATE(GLuint)
    INSTANTIATE_TEMPLATE(vec2)
    INSTANTIATE_TEMPLATE(vec3)
    INSTANTIATE_TEMPLATE(vec4)
    INSTANTIATE_TEMPLATE(mat2)
    INSTANTIATE_TEMPLATE(mat3)
    INSTANTIATE_TEMPLATE(mat4)

    #undef INSTANTIATE_TEMPLATE

}
