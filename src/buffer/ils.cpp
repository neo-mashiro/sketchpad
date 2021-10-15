#include "pch.h"

#include "core/log.h"
#include "buffer/ils.h"

namespace buffer {

    ILS::ILS(GLsizei width, GLsizei height, GLuint unit, GLenum format)
        : Buffer(), width(width), height(height), unit(unit), format(format) {

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ILS::~ILS() {
        CORE_WARN("Deleting image load store {0}...", id);
        glDeleteTextures(1, &id);        
    }

    void ILS::Bind() const {
        glBindImageTexture(unit, id, 0, GL_FALSE, 0, GL_READ_WRITE, format);
    }

    void ILS::Unbind() const {
        glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, format);
    }

}
