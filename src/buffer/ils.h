#pragma once

#include "buffer/buffer.h"

namespace buffer {

    /* despite the name ILS, image load store is just an "image", a single level of image from a
       texture. Image here refers to a layer in the texture, not the image object that is loaded
       from local image files. While textures can have multiple mipmap levels or array layers,
       images can only represent one of them. An image is merely a layered reference to the host
       texture, which does not hold extra data and cannot exist by itself (like texture views).
       as soon as the texture is destroyed, the ILS becomes invalid, we don't need a destructor.

       images have their own set of binding points called image units, which are independent of
       texture units and are counted separately. Images are essentially a large 2D array, so the
       pixels can only be accessed by (signed) integer indices, as such, floating-point indices
       are not allowed, and you must make sure no index is out of bound. This also means that no
       filtering will be applied, as a result, images do not work with samplers.
       
       for image load store, reads and writes are accessed via a list of GLSL built-in functions.
       ILS is often used in pair with a compute shader, the images play the role of data buffers.
       an image can be bound to multiple image units at the same time, in this case, coherency
       of memory accesses must be taken care of. If it's only bound to one image unit, we should
       always use the `restrict` memory qualifier so that reads/writes can be optimized.
       see also: https://www.khronos.org/opengl/wiki/Image_Load_Store#Image_operations

       the advantage of using image load/store over regular textures (whether storage is mutable
       or not) lies in its flexibility in terms of both read and write operations from within
       the shader. These operations are cheap, mostly atomic, but you need to manage incoherent
       memory access with proper barrier calls. Equipped with the powerful features of ILS, the
       user is able to manipulate the data store in a number of new ways.

       # example use cases

       ILS can be used to implement relatively cheap order-independent transparency (OIT).
       ILS is the best tool to implement temporal anti-aliasing (TAA), both the past and current
       frame can be represented by ILS so that sampling and blending pixels are made much easier.
    */

    class Texture;

    class ILS : public Buffer {
      private:
        void Bind() const override {}
        void Unbind() const override {}

      public:
        GLuint width, height, level;
        GLenum target, internal_format;

        ILS(const Texture& texture, GLuint level);
        ~ILS() {}
        
        void Bind(GLuint unit) const;
        void Unbind(GLuint unit) const;

        static void Transfer(const ILS& fr, const ILS& to);
    };

}
