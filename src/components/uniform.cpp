#include "pch.h"

#include <type_traits>
#include <glm/glm.hpp>
#include "components/uniform.h"
#include "components/component.h"

using namespace glm;

namespace components {

    template<typename T>
    void Uniform<T>::operator<<(const T& value) {
        this->value = value;
        pending_upload = true;
    }

    template<typename T>
    void Uniform<T>::operator<<=(const T* value_ptr) {
        this->value_ptr = value_ptr;
        binding_upload = true;
    }

    template<typename T>
    void Uniform<T>::Upload() const {
        T val = binding_upload ? (*value_ptr) : value;

        /**/ if constexpr (std::is_same_v<T, bool>)  { glUniform1i(location, static_cast<int>(val)); }
        else if constexpr (std::is_same_v<T, int>)   { glUniform1i(location, val); }
        else if constexpr (std::is_same_v<T, float>) { glUniform1f(location, val); }
        else if constexpr (std::is_same_v<T, vec2>)  { glUniform2fv(location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec3>)  { glUniform3fv(location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec4>)  { glUniform4fv(location, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, mat2>)  { glUniformMatrix2fv(location, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat3>)  { glUniformMatrix3fv(location, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat4>)  { glUniformMatrix4fv(location, 1, GL_FALSE, &val[0][0]); }
        else {
            // throw a compiler error for any other type of T
            static_assert(const_false<T>, "Unspecified template uniform type T ...");
        }

        pending_upload = false;
    }

    // explicit template class instantiation
    template class Uniform<int>;
    template class Uniform<bool>;
    template class Uniform<float>;
    template class Uniform<vec2>;
    template class Uniform<vec3>;
    template class Uniform<vec4>;
    template class Uniform<mat2>;
    template class Uniform<mat3>;
    template class Uniform<mat4>;

}
