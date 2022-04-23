#include "pch.h"
#include "asset/vao.h"
#include "core/debug.h"

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

    void VAO::SetVBO(GLuint vbo, GLuint attr_id, GLint offset, GLint size, GLint stride, GLenum type) const {
        glVertexArrayVertexBuffer(id, attr_id, vbo, offset, stride);
        glEnableVertexArrayAttrib(id, attr_id);
        glVertexArrayAttribBinding(id, attr_id, attr_id);

        switch (type) {
            case GL_HALF_FLOAT:
            case GL_FLOAT: {
                glVertexArrayAttribFormat(id, attr_id, size, type, GL_FALSE, 0); break;
            }
            case GL_UNSIGNED_INT:
            case GL_INT: {
                glVertexArrayAttribIFormat(id, attr_id, size, type, 0); break;  // notice the "I" here
            }
            case GL_DOUBLE: {
                glVertexArrayAttribLFormat(id, attr_id, size, type, 0); break;  // notice the "L" here
            }
            default: {
                throw core::NotImplementedError("Unsupported vertex attribute type!"); break;
            }
        }
    }

    void VAO::SetIBO(GLuint ibo) const {
        glVertexArrayElementBuffer(id, ibo);
    }

    void VAO::Draw(GLenum mode, GLsizei count) {
        Bind();
        glDrawElements(mode, count, GL_UNSIGNED_INT, 0);

        if constexpr (false) {
           Unbind();  // with smart bindings, we never need to unbind after the draw call
        }
    }

}