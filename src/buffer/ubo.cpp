#include "pch.h"

#include "core/log.h"
#include "buffer/ubo.h"

namespace buffer {

    UBO::UBO(GLuint unit, GLsizeiptr block_size) : Buffer(), unit(unit) {
        // uniform data changes quite often so we always use dynamic as the hint
        glCreateBuffers(1, &id);
        glNamedBufferStorage(id, block_size, NULL, GL_DYNAMIC_STORAGE_BIT);
        glBindBufferBase(GL_UNIFORM_BUFFER, unit, id);
    }

    UBO::~UBO() {
        CORE_WARN("Deleting uniform buffer (id = {0})!", id);
        glDeleteBuffers(1, &id);
    }

    UBO::UBO(UBO&& other) noexcept {
        *this = std::move(other);
    }

    UBO& UBO::operator=(UBO&& other) noexcept {
        if (this != &other) {
            offset.clear();
            size.clear();

            std::swap(id, other.id);
            std::swap(unit, other.unit);
            std::swap(offset, other.offset);
            std::swap(size, other.size);
        }

        return *this;
    }

    void UBO::Bind() const {
        glBindBuffer(GL_UNIFORM_BUFFER, id);
    }

    void UBO::Unbind() const {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UBO::SetOffset(const std::vector<GLuint>& offset) {
        this->offset = offset;
    }

    void UBO::SetSize(const std::vector<size_t>& size) {
        this->size = size;
    }
    
    void UBO::SetData(GLuint uniform_index, const void* data) const {
        // set data for a single uniform in the uniform block
        glNamedBufferSubData(id, offset[uniform_index], size[uniform_index], data);
    }

    void UBO::SetData(GLuint fr, GLuint to, const void* data) const {
        // set data for a range of uniforms in the block
        auto it = size.begin();
        size_t n_bytes = std::accumulate(it + fr, it + to + 1, decltype(size)::value_type(0));
        glNamedBufferSubData(id, offset[fr], n_bytes, data);
    }

}
