#include "pch.h"

#include "buffer/vao.h"
#include "core/log.h"

namespace buffer {

    VAO::VAO() {
        glGenVertexArrays(1, &id);
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

    void VAO::SetLayout() const {
        GLint vbo = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);

        if (vbo <= 0) {
            CORE_ERROR("No active VBO is currently bound, unable to set the vertex layout...");
            return;
        }

        glEnableVertexAttribArray(0);  // position
        glEnableVertexAttribArray(1);  // normal
        glEnableVertexAttribArray(2);  // uv
        glEnableVertexAttribArray(3);  // uv2
        glEnableVertexAttribArray(4);  // tangent
        glEnableVertexAttribArray(5);  // bitangent

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv2));
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
    }

}
