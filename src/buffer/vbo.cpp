#include "pch.h"
#include "buffer/vbo.h"

namespace buffer {

    VBO::VBO() {
        glGenBuffers(1, &id);
    }

    VBO::~VBO() {
        glDeleteBuffers(1, &id);
    }

    void VBO::Bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    void VBO::Unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VBO::SetData(GLsizeiptr size, const void* data) const {
        glNamedBufferData(id, size, data, GL_STATIC_DRAW);
    }

    void VBO::SetData(GLsizeiptr size, const void* data, GLintptr offset) const {
        glNamedBufferSubData(id, offset, size, data);
    }

    GLfloat* VBO::MapData(GLintptr offset, GLsizeiptr size) const {
        return reinterpret_cast<GLfloat*>(glMapNamedBufferRange(id, offset, size, GL_MAP_READ_BIT));
    }

    void VBO::UnmapData() const {
        glUnmapNamedBuffer(id);
    }

}
