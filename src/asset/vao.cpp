#include "pch.h"
#include "asset/vao.h"

namespace asset {

    static GLuint curr_bound_vertex_array = 0;  // smart binding

    VAO::VAO() : IAsset() {
        glCreateVertexArrays(1, &id);
    }

    VAO::~VAO() {
        Unbind();
        glDeleteVertexArrays(1, &id);
    }

    void VAO::Bind() const {
        if (id != curr_bound_vertex_array) {
            glBindVertexArray(id);
            curr_bound_vertex_array = id;
        }
    }

    void VAO::Unbind() const {
        if (curr_bound_vertex_array == id) {
            curr_bound_vertex_array = 0;
            glBindVertexArray(0);
        }
    }

    void VAO::SetVBO(GLuint vbo, GLuint attr_id, GLint offset, GLint size, GLint stride) const {
        glVertexArrayVertexBuffer(id, attr_id, vbo, offset, stride);
        glEnableVertexArrayAttrib(id, attr_id);
        glVertexArrayAttribFormat(id, attr_id, size, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(id, attr_id, attr_id);
    }

    void VAO::SetIBO(GLuint ibo) const {
        glVertexArrayElementBuffer(id, ibo);
    }

}
