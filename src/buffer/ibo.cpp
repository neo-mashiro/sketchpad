#include "pch.h"
#include "buffer/ibo.h"

namespace buffer {

    IBO::IBO() {
        glCreateBuffers(1, &id);
    }

    IBO::~IBO() {
        glDeleteBuffers(1, &id);
    }

    void IBO::Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    }

    void IBO::Unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void IBO::SetData(GLsizeiptr size, const void* data) const {
        glNamedBufferData(id, size, data, GL_STATIC_DRAW);
    }

    void IBO::SetData(GLsizeiptr size, const void* data, GLintptr offset) const {
        glNamedBufferSubData(id, offset, size, data);
    }

}
