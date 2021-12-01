#include "pch.h"

#include "core/debug.h"
#include "core/log.h"
#include "buffer/ils.h"
#include "buffer/texture.h"

namespace buffer {

    ILS::ILS(const Texture& texture, GLuint level) : Buffer(), level(level) {
        this->id = texture.id;
        this->target = texture.target;
        this->internal_format = texture.internal_format;

        CORE_ASERT(level < texture.n_levels, "Input texture does not have level {0}...", level);
        GLuint scale = static_cast<GLuint>(std::pow(2, level));

        this->width = texture.width / scale;
        this->height = texture.height / scale;
    }

    void ILS::Bind(GLuint unit) const {
        glBindImageTexture(unit, id, level, GL_TRUE, 0, GL_READ_WRITE, internal_format);
    }

    void ILS::Unbind(GLuint unit) const {
        glBindImageTexture(unit, 0, level, GL_TRUE, 0, GL_READ_WRITE, internal_format);
    }

    void ILS::Transfer(const ILS& fr, const ILS& to) {
        GLuint fw = fr.width;
        GLuint fh = fr.height;
        GLenum target = fr.target;

        if (fw != to.width || fh != to.height) {
            CORE_ERROR("Unable to transfer ILS data store, width or height mismatch!");
            return;
        }

        if (target != to.target) {
            CORE_ERROR("Unable to transfer ILS data store, incompatible target!");
            return;
        }

        if (target != GL_TEXTURE_2D) {
            throw core::NotImplementedError("ILS target not yet supported....");
        }

        glCopyImageSubData(fr.id, target, 0, 0, 0, 0, to.id, target, 0, 0, 0, 0, fw, fh, 1);
    }

}
