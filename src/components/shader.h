#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>

namespace components {

    class Shader {
      private:
        std::vector<GLuint> shaders;

        void LoadShader(GLenum type);
        void LinkShaders();

      public:
        GLuint id;
        std::string source_path;

        Shader(const std::string& source_path);
        Shader(const std::string& binary_path, GLenum format);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        void Bind() const;
        void Unbind() const;
        void Save() const;
    };
}
