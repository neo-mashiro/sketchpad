#pragma once

#include <vector>
#include "buffer/buffer.h"

namespace buffer {

    // in this demo, it is required that every uniform block uses the `std140` layout,
    // based on this assumption, we can simplify the APIs so that the user only needs
    // to provide the a uniform's index in the block when updating uniform data.

    class UBO : public Buffer {
      private:
        GLuint unit;  // uniform block's binding point specified in `layout(std140)`
        
        std::vector<GLuint> offset;  // each uniform's aligned offset in the block
        std::vector<size_t> size;    // each uniform's size in bytes in the block

      public:
        UBO() = default;
        UBO(GLuint unit, GLsizeiptr block_size);
        ~UBO();

        void Bind() const override;
        void Unbind() const override;

        void SetOffset(const std::vector<GLuint>& offset);
        void SetSize(const std::vector<size_t>& size);

        void SetData(GLuint uniform_index, const void* data) const;
        void SetData(GLuint fr, GLuint to, const void* data) const;
    };

}
