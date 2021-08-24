#pragma once

#include <vector>
#include <GL/glew.h>

namespace scene {

    // uniform buffers (globally shared shader data, 1-1 mapping with GLSL uniform blocks)
    class UBO {
      private:
        GLuint id;
        GLuint binding_point;
        size_t buffer_size;

        std::vector<GLuint> offset;  // aligned offset of each uniform in the buffer
        std::vector<GLuint> size;    // size in bytes of each uniform in the buffer

      public:
        UBO() = default;
        UBO(GLuint binding_point, size_t buffer_size, const std::vector<GLuint>& offset,
            const std::vector<GLuint>& size, GLenum hint = GL_DYNAMIC_DRAW);
        ~UBO();

        bool Bind() const;
        void Unbind() const;
        void SetData(GLuint uniform_index, const void* data) const;

        GLuint Count() const;
        GLuint Size() const;
    };

}
