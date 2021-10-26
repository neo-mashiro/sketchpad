#include "pch.h"

#include "core/log.h"
#include "buffer/ils.h"

namespace buffer {

    ILS::ILS(GLuint width, GLuint height, GLenum internal_format)
        : Buffer(), width(width), height(height), internal_format(internal_format) {

        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, 1, internal_format, width, height);
    }

    ILS::~ILS() {
        CORE_WARN("Deleting image load store {0}...", id);
        glDeleteTextures(1, &id);        
    }

    void ILS::Bind(GLuint unit) const {
        glBindImageTexture(unit, id, 0, GL_FALSE, 0, GL_READ_WRITE, internal_format);
    }

    void ILS::Unbind(GLuint unit) const {
        glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_WRITE, internal_format);
    }

    void ILS::Transfer(const ILS& fr, const ILS& to) {
        GLuint fw = fr.width;
        GLuint fh = fr.height;

        if (fw != to.width || fh != to.height) {
            CORE_ERROR("Unable to transfer ILS data store, width or height mismatch!");
            return;
        }

        glCopyImageSubData(fr.id, GL_TEXTURE_2D, 0, 0, 0, 0, to.id, GL_TEXTURE_2D, 0, 0, 0, 0, fw, fh, 1);
    }

}
