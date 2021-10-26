#pragma once

#include "buffer/buffer.h"

namespace buffer {

    /* for image load store, reads and writes are accessed via a list of GLSL built-in functions.
       ILS is often used in pair with a compute shader, the images play the role of data buffers.
       see also: https://www.khronos.org/opengl/wiki/Image_Load_Store#Image_operations

       the advantage of using image load/store over regular textures (whether storage is mutable
       or not) lies in its flexibility in terms of both read and write operations from within
       the shader. These operations are cheap, mostly atomic, but you need to manage incoherent
       memory access with proper barrier calls. Equipped with the powerful features of ILS, the
       user is able to manipulate the data store in a number of new ways.

       # example use cases

       ILS can be used to implement relatively cheap order-independent transparency.
       ILS is the best tool to implement temporal anti-aliasing (TAA), both the past and current
       frame can be represented by ILS so that sampling and blending pixels are made much easier.
    */

    class ILS : public Buffer {
      private:
        void Bind() const override {}
        void Unbind() const override {}

      public:
        GLuint width, height;
        GLenum internal_format;

        ILS() = default;
        ILS(GLuint width, GLuint height, GLenum internal_format);
        ~ILS();
        
        void Bind(GLuint unit) const;
        void Unbind(GLuint unit) const;

        static void Transfer(const ILS& fr, const ILS& to);
    };

}
