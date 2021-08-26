#pragma once

#include <vector>
#include <GL/glew.h>

namespace scene {

    // uniform buffers (globally shared shader data, 1-1 mapping with GLSL uniform blocks)
    class UBO {
      private:
        GLuint id { 0 };
        GLuint binding_point { 0 };
        size_t buffer_size { 0 };

        std::vector<GLuint> offset;  // aligned offset of each uniform in the buffer
        std::vector<size_t> size;    // size in bytes of each uniform in the buffer

      public:
        UBO() = default;
        UBO(GLuint binding_point, size_t buffer_size, const std::vector<GLuint>& offset,
            const std::vector<size_t>& size, GLenum hint = GL_DYNAMIC_DRAW);
        ~UBO();

        bool Bind() const;
        void Unbind() const;

        void SetData(GLuint uniform_index, const void* data) const;
        void SetData(GLuint fr, GLuint to, const void* data) const;

        GLuint Count() const;
        GLuint Size() const;
    };

}
