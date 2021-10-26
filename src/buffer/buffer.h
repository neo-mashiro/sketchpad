#pragma once

#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace buffer {

    /* in this demo, we use the abstract term "buffer" to represent any certain type of data store
       or data buffer that is used to hold data or move data around. A buffer can be intermediate
       data containers for communication between the CPU and GPU, or live on the GPU exclusively.
       in most cases, we would want to first allocate GPU memory for the buffer, next feed data to
       the buffer, then upload or bind it in the OpenGL global state machine, and finally consume
       it within the GLSL shaders. It is also possible to share one buffer's data across multiple
       uses, or read data back from the GPU. For instance, a VBO can be constructed from the data
       of an existing SSBO, an SSBO can be mapped to the CPU memory space and returns a pointer...

       in this demo, we are mostly going to use these types of buffers:

       - VAO (Vertex Array Object)
       - VBO (Vertex Buffer Object)
       - IBO (Index Buffer Object)
       - UBO (Uniform Buffer Object)
       - FBO (Framebuffer Object)
       - RBO (Renderbuffer Object)
       - SSBO (Shader Storage Buffer Object)
       - PBO (Pixel Buffer Object)
       - ILS (Image Load Store)

       since OpenGL 4.5, DSA (direct state access) has been introduced into the core profile, with
       DSA at out disposal, we can now easily modify, read/write or setup data for buffers without
       having to bind them. Whenever possible, we will use the DSA version of OpenGL calls instead
       of the old ones. Previously without DSA, we have to bind a buffer before changing its state,
       then unbind, for complex operations such as blitting FBOs, we even have to assign all read
       write buffers and later restore them (read/write buffers are part of the framebuffer state),
       but now that is completely redundant. In short, DSA makes our code clean and life easier.
    */

    template<typename T>
    using buffer_ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr buffer_ref<T> LoadBuffer(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    class Buffer {
      private:
        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

      protected:
        GLuint id;  // read-only

      public:
        Buffer();
        GLuint GetID() const;
    };

}
