#pragma once

#include "buffer/buffer.h"

namespace buffer {

    class RBO : public Buffer {
      private:
        GLuint width, height;

      public:
        RBO(GLuint width, GLuint height);
        ~RBO();

        void Bind() const override;
        void Unbind() const override;
    };

}
