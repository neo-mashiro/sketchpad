#pragma once

#include "buffer/buffer.h"

namespace buffer {

    class VAO : public Buffer {
      public:
        VAO();
        ~VAO();

        void Bind() const override;
        void Unbind() const override;

        void SetVBO(GLuint vbo, GLuint attribute_id, GLint offset, GLint size, GLint stride) const;
        void SetIBO(GLuint ibo) const;
    };

}
