#pragma once

#include "buffer/buffer.h"

namespace buffer {

    // for image load store, reads and writes are accessed via a list of GLSL built-in functions.
    // ILS is often used in pair with a compute shader, the images play the role of data buffers.
    // see also: https://www.khronos.org/opengl/wiki/Image_Load_Store#Image_operations

    class ILS : public Buffer {
      public:
        GLsizei width, height;
        GLuint unit;
        GLenum format;

        ILS() = default;
        ILS(GLsizei width, GLsizei height, GLuint unit, GLenum format = GL_RGBA16F);
        ~ILS();
        
        void Bind() const override;
        void Unbind() const override;
    };

}
