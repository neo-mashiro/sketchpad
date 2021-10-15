#include "pch.h"
#include "buffer/vao.h"

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
        glEnableVertexArrayAttrib(id, 0);  // position
        glEnableVertexArrayAttrib(id, 1);  // normal
        glEnableVertexArrayAttrib(id, 2);  // uv
        glEnableVertexArrayAttrib(id, 3);  // uv2
        glEnableVertexArrayAttrib(id, 4);  // tangent
        glEnableVertexArrayAttrib(id, 5);  // bitangent

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, uv2));
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tangent));
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, bitangent));
    }

}
