#include "pch.h"

#include "core/log.h"
#include "buffer/ssbo.h"

namespace buffer {

    template<typename T>
    SSBO<T>::SSBO(GLuint array_size, GLuint index) : Buffer(), array_size(array_size), index(index) {
        glGenBuffers(1, &id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);

        // SSBO's buffer data is mostly dynamic, by default we use `GL_DYNAMIC_DRAW` as the hint.
        // here we only allocate GPU memory for the buffer, but its data is left uninitialized
        glBufferData(GL_SHADER_STORAGE_BUFFER, array_size * sizeof(T), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    template<typename T>
    SSBO<T>::~SSBO() {
        CORE_WARN("Deleting shader storage buffer object {0}...", id);
        glDeleteBuffers(1, &id);
    }

    template<typename T>
    void SSBO<T>::Bind() const {
        // multiple SSBOs can share the same index (binding point of an SSBO buffer block in GLSL)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id);
    }

    template<typename T>
    void SSBO<T>::Unbind() const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, 0);
    }

    template<typename T>
    void SSBO<T>::Write(const std::vector<T>& data) const {
        CORE_ASERT(data.size() <= array_size, "SSBO does not have enough memory to hold the data!");
        glNamedBufferData(id, array_size * sizeof(T), &data[0], GL_DYNAMIC_DRAW);
    }

    template<typename T>
    void SSBO<T>::Write(const std::vector<T>& data, GLintptr offset, GLsizeiptr size) const {
        CORE_ASERT(data.size() <= array_size, "SSBO does not have enough memory to hold the data!");
        glNamedBufferSubData(id, offset, size, &data[0]);
    }

    template<typename T>
    T* SSBO<T>::Acquire() const {
        return reinterpret_cast<T*>(glMapNamedBuffer(id, GL_READ_WRITE));
    }

    template<typename T>
    T* SSBO<T>::Acquire(GLintptr offset, GLuint n_elements) const {
        return reinterpret_cast<T*>(glMapNamedBufferRange(id, offset, n_elements * sizeof(T), GL_MAP_READ_BIT));
    }

    template<typename T>
    void SSBO<T>::Release() const {
        glUnmapNamedBuffer(id);
    }

    template<typename T>
    void SSBO<T>::Clear() const {
        std::vector<T> zeros(array_size);  // don't make it static, the memory can be huge
        glNamedBufferData(id, array_size * sizeof(T), &zeros[0], GL_DYNAMIC_DRAW);
    }

    // the template type T is explicitly instantiated to `int`, `uint`, `float`, `vec2` and `vec4`
    // users are not allowed to use other generic data types (but feel free to extend it)
    template class SSBO<GLint>;
    template class SSBO<GLuint>;
    template class SSBO<GLfloat>;
    template class SSBO<glm::vec2>;
    template class SSBO<glm::vec4>;

}
