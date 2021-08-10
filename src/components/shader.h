#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace components {

    class Texture;  // forward declaration

    class Shader {
      private:
        std::string source_path;
        GLuint id;
        GLuint n_samplers_max;

        std::vector<GLuint> shaders;
        std::unordered_map<GLuint, Texture> textures;
        mutable std::unordered_map<std::string, GLint> uniform_loc_cache;

        void LoadShader(GLenum type);
        void LinkShaders();
        GLint GetUniformLocation(const std::string& name) const;

      public:
        Shader(const std::string& source_path);
        Shader(const std::string& binary_path, GLenum format);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        void PushTexture(GLuint unit, GLenum target, const std::string& path);
        void PopTexture(GLuint unit);
        bool Bind() const;
        void Unbind() const;
        void Save() const;

        // set shader uniforms
        void SetBool(const std::string& name, bool value) const;
        void SetInt(const std::string& name, int value) const;
        void SetFloat(const std::string& name, float value) const;
        void SetVec2(const std::string& name, const glm::vec2& value) const;
        void SetVec3(const std::string& name, const glm::vec3& value) const;
        void SetVec4(const std::string& name, const glm::vec4& value) const;
        void SetMat2(const std::string& name, const glm::mat2& value) const;
        void SetMat3(const std::string& name, const glm::mat3& value) const;
        void SetMat4(const std::string& name, const glm::mat4& value) const;

      private:
        const char* GlslType(GLenum gl_type);

      public:
        void GetActiveUniformList();
    };
}
