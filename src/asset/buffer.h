/* in this demo, we use the abstract term "buffer" to represent any certain type of data store
   or data buffer that is used to hold data or move data around. A buffer can be intermediate
   data containers for communication between the CPU and GPU, or live on the GPU exclusively.
   in most cases, we would want to first allocate GPU memory for the buffer, next feed data to
   the buffer, then upload or bind it in the OpenGL global state machine, and finally consume
   it within the GLSL shaders. It is also possible to share one buffer's data across multiple
   uses, or read data back from the GPU. For instance, a VBO can be constructed from the data
   of an existing SSBO, an SSBO can be mapped to the CPU memory space and returns a pointer...

   specifically, we are going to abstract these OpenGL objects into buffers:

   - VAO  (Vertex Array Object)
   - VBO  (Vertex Buffer Object)
   - IBO  (Index Buffer Object)
   - UBO  (Uniform Buffer Object)
   - FBO  (Framebuffer Object)
   - RBO  (Renderbuffer Object)
   - SSBO (Shader Storage Buffer Object)
   - PBO  (Pixel Buffer Object)
   - ILS  (Image Load Store)
   - TFB  (Transform Feedback)
   - TBO  (Texture Buffer Object)

   similarly, textures, texture views and samplers are basically just data stores so they also
   fall into this category. In addition, shaders and compute shaders, seeing as containers of
   shading instructions grouped in the form a GLSL program, can be treated as "buffers" (kind
   of) as well, although this seems like a far-fetched analogy, it helps keep our code clean.

   since OpenGL 4.5, DSA (direct state access) has been introduced into the core profile, with
   DSA at out disposal, we can now easily modify, read/write or setup data for buffers without
   having to bind them. Whenever possible, we will use the DSA version of OpenGL calls instead
   of the old ones. Previously without DSA, we have to bind a buffer before changing its state,
   then unbind after we're done, possibly need to restore all the read/write buffers as well.

   note that the presence of DSA also makes some `glBindBuffer()` calls entirely unnecessary,
   for instance, our UBO and SSBO class would never need a `Bind()` or `Unbind()` call, we can
   simply override the pure virtual functions with empty bodies and make them private to hide
   visibility from the user.

   we will use immutable data store everywhere
*/

/* the UBO class assumes that every uniform block in GLSL uses the `std140` layout, based
   on this assumption, we can simplify the APIs so that the user only needs to provide the
   a uniform's index in the block when updating uniform data.

   # memory layout


   # supported data types



   # no bind

   for indexed buffers like UBO and SSBO, with DSA features we don't really need the `Bind()`
   and `Unbind()` functions because the buffer object is always bound to a unique binding
   point in GLSL.

// this function parses the shader to determine the structure of uniform blocks and then
// stores them in `UBOs`, each uniform buffer in the `UBOs` map is uniquely keyed by its
// binding point. As to the uniform data types, note that we will only allow scalars and
// 2-element or 4-element vectors and matrices, and arrays of them, and we won't support
// any double precision types such as double or dvec's, or any user-defined struct types.
// for this demo, these principles are applied to SSBOs as well (using either `std140` or
// `std430` layout), this makes it much easier to work with either of the memory layouts.

// the reason for this is that 3-element data types are evil, implementation of 3-element
// paddings tends to be buggy on many drivers, and that they are less performant than the
// 4-element equivalents. Meanwhile, double precision data types is an overkill for most
// applications so can be safely ignored anyway, doing so has the additional benefit that
// the paddings for elements in an array will then reduce to a very simple rule:

// for `std140`, an array element is always padded to the size of a vec4 (4N = 16 bytes)
// for `std430`, an array element is always padded to the base alignment of its data type

// in terms of structs, nested non-basic types will only add complexity to our code base,
// there's no reason to use them as they can always break into multiple basic types.

// for efficient use of memory, it is best to pack pieces of data into 4-element vectors
// as much as possible, then access each piece of data separately with the xyzw component
// swizzling. (e.g. pack a vec3 + a float into a vec4). For elements in arrays, this is a
// mandatory rather than a suggestion because the element will be padded to a vec4 anyway
// so we have no choice but to handle paddings manually in C++, for instance, float[] or
// ivec2[] cannot be directly passed to the UBO as they are not padded in C++, we have to
// either pad them to a vec4 (in which case more than half of the memory space is wasted)
// or pack 4 floats into one vec4 (no memory space is wasted). Note that we always prefer
// vectors over raw C-style arrays when it comes to padding and packing, STL vectors are
// not only guaranteed to be tightly packed in contiguous memory, but they also give us a
// lot more robustness and control over raw arrays.

no bind or unbind

*/

/* SSBOs are mostly used as data buffers in the compute shader, their most typical use
   cases are: particle systems, water and cloth simulation and forward plus rendering.
   for computations that naturally fit onto a 2D Grid, you can also use ILS (image load
   store), but the advantage of SSBO is that it can store much larger data (>= 128 MB).

   from an abstract point of view, you can think of SSBOs as being tightly packed one-
   dimensional arrays, or the flattened array of a higher dimensional compute space.
   every element in the array directly maps to an invocation in the compute space, this
   relation can be derived from the number/size of work groups and local invocation id.
   see also: https://www.khronos.org/opengl/wiki/Compute_Shader#Inputs

   # support data types

   our SSBOs are stored as an array of type T, where T is usually a `float` or a `vec4`
   that are sufficient for most use cases of SSBOs. For the time being, we only need to
   support a few types listed below: (explicitly instantiated in the cpp file)

   int, uint, float, vec2, vec4 (do not use a vec3, it's buggy and slow)

   note that matrices are not supported as they are rarely used in the context of SSBO,
   and it is recommended to split them up into vector components. Besides, given that
   we expect data to come from vectors, boolean types are also not allowed, use an int
   or uint instead. This is because `std::vector<bool>` is not a standard STL container
   like other vectors, for space-optimization reasons, C++ standard requires each bool
   to take up only one byte instead of 4. So if we want to pass a bool from C++ to GLSL
   we would have to cast it to int in all cases.

   # memory layout

   as with UBOs, SSBOs are indexed buffers that specify the `std140` layout, on top of
   that, they also support layout `std430`, which is more widely used for SSBOs because
   the base alignment and stride of an array element is not rounded up to the size of a
   vec4, this means that even `float[]` will be tightly packed.

   our use of SSBOs follows the same principles applied on UBOs, these are detailed in
   the comment section in the UBO header file. In brief, although restrictions have not
   been explicitly imposed on SSBOs, we should never use 3-component types like `vec3`,
   or user-defined structs (BAD and error-prone). In the context of SSBOs, this is even
   more important as SSBOs are intended to hold much more data, so memory space becomes
   the major concern for efficiency. If you have to use struct, store each element in a
   separate SSBO instead, thus making SSBO a tightly-packed homogeneous buffer array.

   since UBOs are mostly identical and reused by different scenes, they're auto managed
   by the scene class which parses the shader to determine the UBO structure. For SSBOs
   the data structure is quite simple, often a single array of scalars or vec4s, so we
   will not bother with the automation of parsing the `std430` buffer block, users are
   responsible for handling data structures themselves with correct aligned offsets and
   data size, e.g. put `float[]` or `int[]` after `vec4[]`s to aovid paddings.

   # SSBO vs UBO

   SSBOs are often used to store much larger data, UBOs are limited by 16KB.
   SSBOs save more memory space using `std430`, no need to pack array data manually.
   SSBOs are slower to access, while UBOs are blazingly fast.
   SSBOs allow read/write in both C++ and GLSL, UBOs are read-only in GLSL.

   # data uploads

   the `Write()` function allows users to overwrite the entire or part of the buffer if
   data is stored in a vector, the `Clear()` function reset the buffer to all zeros so
   that it can be reused. It's not always convenient to store data in a vector, we may
   want a pointer to the buffer for direct reads and writes, this is possible by using
   the `Acquire()` function, but before the call, you have to make sure that the proper
   memory barrier bit has been reached so that reads and writes are visible, otherwise
   data will be corrupted. While the data buffer is mapped to the client address space
   via such a pointer, the SSBO will be in a lock state (kind of) and cannot be used by
   OpenGL, so please remember to `Release()` it once you are done.
*/

// the base class needs to follow the rule of five, with a virtual destructor
// a derived class does not need to explicitly declare any copy/move constructor or the destructor, the
// compiler will generate all of them for us to match the rule of five in the base class.
// but if we explicitly declare one of them, then we have to explicitly declare all others as well

#pragma once

#include <vector>
#include <glad/glad.h>

namespace asset {

    class IBuffer {
      protected:
        GLuint id;
        GLsizeiptr size;
        mutable void* data_ptr;

        IBuffer();
        IBuffer(GLsizeiptr size, const void* data, GLbitfield access);
        virtual ~IBuffer();

        IBuffer(const IBuffer&) = delete;
        IBuffer& operator=(const IBuffer&) = delete;
        IBuffer(IBuffer&& other) noexcept;
        IBuffer& operator=(IBuffer&& other) noexcept;

      public:
        GLuint ID() const { return this->id; }
        GLsizeiptr Size() const { return this->size; }
        void* const Data() const { return this->data_ptr; }

        static void Copy(GLuint fr, GLuint to, GLintptr fr_offset, GLintptr to_offset, GLsizeiptr size);

        void GetData(void* data) const;
        void GetData(GLintptr offset, GLsizeiptr size, void* data) const;
        void SetData(const void* data) const;
        void SetData(GLintptr offset, GLsizeiptr size, const void* data) const;

        void Acquire(GLbitfield access) const;
        void Release() const;
        void Clear() const;
        void Clear(GLintptr offset, GLsizeiptr size) const;
        void Flush() const;
        void Flush(GLintptr offset, GLsizeiptr size) const;
        void Invalidate() const;
        void Invalidate(GLintptr offset, GLsizeiptr size) const;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    class VBO : public IBuffer {
      public:
        VBO(GLsizeiptr size, const void* data, GLbitfield access = 0) : IBuffer(size, data, access) {}
    };

    class IBO : public IBuffer {
      public:
        IBO(GLsizeiptr size, const void* data, GLbitfield access = 0) : IBuffer(size, data, access) {}
    };

    class PBO : public IBuffer {
      public:
        PBO(GLsizeiptr size, const void* data, GLbitfield access = 0) : IBuffer(size, data, access) {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    class IIndexedBuffer : public IBuffer {
      protected:
        GLuint index;
        GLenum target;
        IIndexedBuffer() : IBuffer(), index(0), target(0) {}
        IIndexedBuffer(GLuint index, GLsizeiptr size, GLbitfield access) : IBuffer(size, NULL, access), index(index) {}

      public:
        void Reset(GLuint index);
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    class ATC : public IIndexedBuffer {
      public:
        ATC() = default;
        ATC(GLuint index, GLsizeiptr size, GLbitfield access = GL_DYNAMIC_STORAGE_BIT);
    };

    class SSBO : public IIndexedBuffer {
      public:
        SSBO() = default;
        SSBO(GLuint index, GLsizeiptr size, GLbitfield access = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT);
    };

    class UBO : public IIndexedBuffer {
      private:
        using u_vec = std::vector<GLuint>;
        u_vec offset_vec;  // each uniform's aligned byte offset
        u_vec stride_vec;  // each uniform's byte stride (with padding)
        u_vec length_vec;  // each uniform's byte length (w/o. padding)

      public:
        UBO() = default;
        UBO(GLuint index, const u_vec& offset, const u_vec& length, const u_vec& stride);
        UBO(GLuint shader, GLuint block_id, GLbitfield access = GL_DYNAMIC_STORAGE_BIT);
        void SetUniform(GLuint uid, const void* data) const;
        void SetUniform(GLuint fr, GLuint to, const void* data) const;
    };

}