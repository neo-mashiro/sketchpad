#pragma once

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader {
  private:
    GLuint id;
    std::vector<GLuint> shaders;
    mutable std::unordered_map<std::string, GLint> uniform_loc_cache;

    void LoadShader(GLenum type, const std::string& filepath);
    void LinkShaders();

    GLint GetUniformLocation(const std::string& name) const;

  public:
    Shader(const std::string& filepath);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Set shader uniforms
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec2(const std::string& name, const glm::vec2& value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetMat2(const std::string& name, const glm::mat2& value) const;
    void SetMat3(const std::string& name, const glm::mat3& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;
};
