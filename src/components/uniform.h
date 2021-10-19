#pragma once

#include <string>
#include <GL/glew.h>

namespace components {

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

        void operator<<(const T& value);
        void operator<<=(const T* value_ptr);
        void Upload() const;
    };

    using uni_int   = Uniform<int>;
    using uni_bool  = Uniform<bool>;
    using uni_float = Uniform<float>;
    using uni_vec2  = Uniform<glm::vec2>;
    using uni_vec3  = Uniform<glm::vec3>;
    using uni_vec4  = Uniform<glm::vec4>;
    using uni_mat2  = Uniform<glm::mat2>;
    using uni_mat3  = Uniform<glm::mat3>;
    using uni_mat4  = Uniform<glm::mat4>;

}
