#include "pch.h"

#include "core/log.h"
#include "buffer/vao.h"

namespace buffer {

    VAO::VAO() {
        glCreateVertexArrays(1, &id);
    }

    VAO::~VAO() {
        glDeleteVertexArrays(1, &id);
    }

    void VAO::Bind() const {
        glBindVertexArray(id);
    }

    void VAO::Unbind() const {
        glBindVertexArray(0);
    }

    void VAO::SetVBO(GLuint vbo, GLuint attribute_id, GLint offset, GLint size, GLint stride) const {
        glVertexArrayVertexBuffer(id, attribute_id, vbo, offset, stride);
        glEnableVertexArrayAttrib(id, attribute_id);
        glVertexArrayAttribFormat(id, attribute_id, size, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(id, attribute_id, attribute_id);
    }

    void VAO::SetIBO(GLuint ibo) const {
        glVertexArrayElementBuffer(id, ibo);
    }

}
