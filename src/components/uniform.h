#pragma once

#include <string>
#include <type_traits>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "components/component.h"

namespace components {

    // loose uniforms of dynamic type (locally scoped shader data)
    template<typename T>
    class Uniform {
      public:
        GLuint owner_id;  // id of the shader that owns this uniform
        GLuint location;
        std::string name;

        T value;
        const T* value_ptr;
        mutable bool pending_upload = false;
        mutable bool binding_upload = false;

        Uniform() = default;
        Uniform(GLuint owner_id, GLuint location, char* name)
            : owner_id(owner_id), location(location), name(name), value(0), value_ptr(nullptr) {}

        Uniform(const Uniform&) = default;
        Uniform& operator=(const Uniform&) = default;

        void operator<<(const T& value) {
            this->value = value;
            pending_upload = true;
        }

        void operator<<=(const T* value_ptr) {
            this->value_ptr = value_ptr;
            binding_upload = true;
        }

        void Upload() const {
            T val = binding_upload ? (*value_ptr) : value;

            /**/ if constexpr (std::is_same_v<T, bool>)      { glUniform1i(location, static_cast<int>(val)); }
            else if constexpr (std::is_same_v<T, int>)       { glUniform1i(location, val); }
            else if constexpr (std::is_same_v<T, float>)     { glUniform1f(location, val); }
            else if constexpr (std::is_same_v<T, glm::vec2>) { glUniform2fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::vec3>) { glUniform3fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::vec4>) { glUniform4fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::mat2>) { glUniformMatrix2fv(location, 1, GL_FALSE, &val[0][0]); }
            else if constexpr (std::is_same_v<T, glm::mat3>) { glUniformMatrix3fv(location, 1, GL_FALSE, &val[0][0]); }
            else if constexpr (std::is_same_v<T, glm::mat4>) { glUniformMatrix4fv(location, 1, GL_FALSE, &val[0][0]); }
            else {
                // throw a compiler error for any other type of T
                static_assert(const_false<T>, "Unspecified template uniform type T ...");
            }

            pending_upload = false;
        }
    };

}
