#pragma once

#include "buffer/buffer.h"

namespace buffer {

    class IBO : public Buffer {
      public:
        IBO();
        ~IBO();

        void Bind() const override;
        void Unbind() const override;

        void SetData(GLsizeiptr size, const void* data) const;
        void SetData(GLsizeiptr size, const void* data, GLintptr offset) const;
    };

}
