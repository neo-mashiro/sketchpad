#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "components/shader.h"
#include "components/texture.h"

using namespace core;

namespace components {

    // construct the shader program by compiling from shader sources
    Shader::Shader(const std::string& source_path) : id(0), source_path(source_path) {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        n_samplers_max = Application::GetInstance().gl_max_texture_units;

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
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
        n_samplers_max = Application::GetInstance().gl_max_texture_units;

        CORE_TRACE("Loading pre-compiled shader program from {0} ...", binary_path);

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

            std::cin.get();  // pause the console before exiting so that we can read error messages
            exit(EXIT_FAILURE);
        }

        id = program_id;
    }

    Shader::~Shader() {
        CORE_ASERT(Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);

        // log message to the console so that we are aware of the *hidden* destructor calls
        // this can be super useful in case our data accidentally goes out of scope
        if (id > 0) {
            CORE_WARN("Deleting shader program (id = {0})!", id);
        }

        for (const auto& pair : textures) {
            auto& texture = pair.second;
            glDeleteTextures(1, &texture.id);  // optional, textures will clean up themselves
        }

        shaders.clear();
        shaders.shrink_to_fit();

        textures.clear();
        uniform_loc_cache.clear();

        glDeleteProgram(id);
    }

    Shader::Shader(Shader&& other) noexcept {
        *this = std::move(other);
    }

    Shader& Shader::operator=(Shader&& other) noexcept {
        if (this != &other) {
            for (const auto& pair : textures) {
                auto& texture = pair.second;
                glDeleteTextures(1, &texture.id);
            }

            glDeleteProgram(id);
            source_path = "";
            id = n_samplers_max = 0;

            shaders.clear();
            shaders.shrink_to_fit();

            textures.clear();
            uniform_loc_cache.clear();

            std::swap(id, other.id);
            std::swap(source_path, other.source_path);
            std::swap(n_samplers_max, other.n_samplers_max);
            std::swap(shaders, other.shaders);
            std::swap(textures, other.textures);
            std::swap(uniform_loc_cache, other.uniform_loc_cache);
        }

        return *this;
    }

    void Shader::PushTexture(GLuint unit, GLenum target, const std::string& path) {
        if (textures.size() >= n_samplers_max) {
            CORE_WARN("{0} samplers limit has been reached, failed to push texture...", n_samplers_max);
            return;
        }
        else if (textures.count(unit) > 0) {
            CORE_WARN("Unit {0} already has a texture, operation ignored...", unit);
            return;
        }

        textures.insert(std::make_pair(unit, Texture(target, path)));  // move insertion
    }

    void Shader::PopTexture(GLuint unit) {
        if (textures.count(unit) == 0) {
            CORE_WARN("Unit {0} is already empty, operation ignored...", unit);
            return;
        }

        textures.erase(unit);
    }

    // [side note] in terms of textures binding, our code is making a few implicit assumptions.
    // -----------------------------------------------------------------------------------------
    // [1] assume that we only have one texture of each specific type, or none at all
    //     i.e. we will not allow two specular maps, two displacement maps, etc.
    // -----------------------------------------------------------------------------------------
    // [2] assume that samplers are directly bound to texture units in GLSL, so we don't need to
    //     set sampler uniforms from C++ code, this is easier to work with because we know what
    //     the textures are and in which texture unit they appear. By using an `unordered_map`
    //     to store the textures rather than a `vector`, we are basically enforcing the users to
    //     specify which unit binds to which texture, which is less error-prone as opposed to
    //     using unit 0 by default or setting sampler uniforms later from C++ code.
    //
    //     e.g.: shader.PushTexture(0, GL_TEXTURE_2D, "ambient.jpg");
    //           shader.PushTexture(1, GL_TEXTURE_2D, "diffuse.jpg");
    //           shader.PushTexture(3, GL_TEXTURE_2D, "emission.jpg");
    //
    //     => we know the ambient map goes to texture unit 0, diffuse map goes to 1, etc.
    //     => so in the fragment shader we can specify the binding points in the same order:
    //           ...
    //           layout(binding = 0) uniform sampler2D ambient;
    //           layout(binding = 1) uniform sampler2D diffuse;
    //           layout(binding = 3) uniform sampler2D emission;
    //           ...
    // -----------------------------------------------------------------------------------------
    // [3] when loading external models, textures are often embedded inside and hidden from the
    //     user, so we have no direct access to their details, but in most cases, the model
    //     itself is responsible for handling everything for us, so we assume that the mesh is
    //     either out-of-the-box for immediate use, or plainly coming without textures.
    // -----------------------------------------------------------------------------------------
    // [4] the reason we choose to handle textures in the shader class is that, conceptually,
    //     textures are tied to samplers in GLSL, so we can think of them as being part of the
    //     shader, even though they are not really owned by shaders in the OpenGL global state
    //     machine. It goes without saying that, no matter how we are going to bind textures to
    //     uniform samplers, this will introduce a lot of CPU overhead before each draw call,
    //     which is performance-critical. Recently we also have bindless textures, deferred
    //     shading to work around this, but that's yet another topic for future work...
    // -----------------------------------------------------------------------------------------

    bool Shader::Bind() const {
        if (id <= 0) {
            CORE_ERROR("Attempting to use an invalid shader (id <= 0)!");
            return false;
        }

        glUseProgram(id);

        // rebind textures before each draw call
        for (const auto& pair : textures) {
            auto& texture = pair.second;
            glActiveTexture(GL_TEXTURE0 + pair.first);  // activate texture unit `unit`
            glBindTexture(texture.target, texture.id);  // bind texture in this unit
        }

        // todo: set interface block

        return true;
    }

    void Shader::Unbind() const {
        // clean up texture units after each draw call (optional but recommended)
        for (const auto& pair : textures) {
            auto& texture = pair.second;
            glActiveTexture(GL_TEXTURE0 + pair.first);
            glBindTexture(texture.target, 0);
        }

        glUseProgram(0);
    }

    // save the compiled shader binary to the source folder on disk
    void Shader::Save() const {
        if (source_path.empty()) {
            CORE_ERROR("Shader binary already exists, failed to save shader ...");
            return;
        }

        GLint formats;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
        CORE_TRACE("Number of binary formats supported by the driver = {0}", formats);

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

        std::string filepath = source_path + std::to_string(binary_format) + ".bin";
        CORE_TRACE("Saving compiled shader program to {0} ...", filepath);

        std::ofstream out_stream(filepath.c_str(), std::ios::binary);
        out_stream.write(reinterpret_cast<char*>(buffer.data()), binary_length);
        out_stream.close();
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

    void Shader::LoadShader(GLenum type) {
        // this line may cause access violation if OpenGL context has not been setup
        GLuint shader_id = glCreateShader(type);

        std::string file("");

        switch (type) {
            case GL_VERTEX_SHADER:          file = source_path + "vertex.glsl";   break;
            case GL_GEOMETRY_SHADER:        file = source_path + "geometry.glsl"; break;
            case GL_FRAGMENT_SHADER:        file = source_path + "fragment.glsl"; break;
            case GL_COMPUTE_SHADER:         file = source_path + "compute.glsl";  break;
            case GL_TESS_CONTROL_SHADER:    file = source_path + "tess-ct.glsl";  break;
            case GL_TESS_EVALUATION_SHADER: file = source_path + "tess-ev.glsl";  break;

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
}
