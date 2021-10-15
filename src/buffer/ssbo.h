#pragma once

#include <vector>
#include "buffer/buffer.h"

namespace buffer {

    /*
       SSBOs are mostly used as data buffers in the compute shader, the most typical use
       cases are: particle systems, water or cloth simulation and forward plus rendering.
       for computations that naturally fit onto a 2D Grid, you can also use ILS (image load
       store), but the advantage of SSBO is that it can store much larger data (>= 128 MB).

       from an abstract point of view, you can think of SSBOs as being tightly packed one-
       dimensional arrays, or the flattened array of a higher dimensional compute space.
       every element in the array directly maps to an invocation in the compute space, this
       relation can be derived from the number/size of work groups and local invocation id.
       see also: https://www.khronos.org/opengl/wiki/Compute_Shader#Inputs

       when working with SSBOs, it is recommended to use a multiple of 4 component data so
       as to avoid issues with memory layouts and achieve maximum efficiency, for instance,
       if the SSBO is used to hold 1000 world positions, we'd better use an `array_size`
       of 4000 instead of 3000 because `vec4` is much faster than a `vec3` on the hardware.

       our SSBOs are stored as an array of type T, where T is usually a `float` since it is
       sufficient for most purposes (`double` is overkill). Some users like SSBO because it
       also allows T to be a user-defined struct, but in such cases, paddings must be taken
       into account as per the `std430` layout rules, and one has to map the buffer to the
       C++ address space in order to fill it with mixed-type data, rather than write to the
       buffer directly. All of this is BAD and error-prone so we won't allow it, let alone
       the waste of memory space when the struct is complex or the SSBO buffer is large.
       
       The best way of handling struct is to store each struct element into a separate SSBO,
       so that every SSBO has a tightly-packed homogeneous buffer array. We will simply
       enforce users to follow this rule, which not only makes access faster but also our
       code much cleaner. This class is made as a template only to allow for more common
       data types, not any struct type, all supported types are explicitly instantiated in
       the cpp file, currently we have:

       int, uint, float, vec2, vec4 (do not use a vec3, it's slow)

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

    template<typename T>
    class SSBO : public Buffer {
      public:
        GLuint index, array_size;

        SSBO() = default;
        SSBO(GLuint array_size, GLuint index);
        ~SSBO();
        
        void Bind() const override;
        void Unbind() const override;

        void Write(const std::vector<T>& data) const;
        void Write(const std::vector<T>& data, GLintptr offset, GLsizeiptr size) const;
        void Clear() const;

        T* Acquire() const;
        T* Acquire(GLintptr offset, GLuint n_elements) const;
        void Release() const;
    };

}
