#pragma once

#include "buffer/buffer.h"

namespace buffer {

    class VBO : public Buffer {
      public:
        VBO();
        ~VBO();

        void Bind() const override;
        void Unbind() const override;

        void SetData(GLsizeiptr size, const void* data) const;
        void SetData(GLsizeiptr size, const void* data, GLintptr offset) const;

        GLfloat* MapData(GLintptr offset, GLsizeiptr size) const;
        void UnmapData() const;
    };

}
