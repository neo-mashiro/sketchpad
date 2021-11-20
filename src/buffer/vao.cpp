#include "pch.h"

#include "core/log.h"
#include "buffer/vao.h"

namespace buffer {

    // optimize context switching by avoiding unnecessary binds and unbinds
    static GLuint curr_bound_buffer = 0;

    VAO::VAO() {
        glCreateVertexArrays(1, &id);
    }

    VAO::~VAO() {
        glDeleteVertexArrays(1, &id);

        if (curr_bound_buffer == id) {
            curr_bound_buffer = 0;
            glBindVertexArray(0);
        }
    }

    void VAO::Bind() const {
        if (id != curr_bound_buffer) {
            glBindVertexArray(id);
            curr_bound_buffer = id;
        }
    }

    void VAO::Unbind() const {
        if (curr_bound_buffer != 0) {
            curr_bound_buffer = 0;
            glBindVertexArray(0);
        }
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
