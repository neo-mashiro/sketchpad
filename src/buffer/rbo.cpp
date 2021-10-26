#include "pch.h"

#include "core/log.h"
#include "buffer/rbo.h"

namespace buffer {

    RBO::RBO(GLuint width, GLuint height) : Buffer(), width(width), height(height) {
        glCreateRenderbuffers(1, &id);
        glNamedRenderbufferStorage(id, GL_DEPTH24_STENCIL8, width, height);
    }

    RBO::~RBO() {
        glDeleteRenderbuffers(1, &id);
    }

    void RBO::Bind() const {
        glBindRenderbuffer(GL_RENDERBUFFER, id);
    }

    void RBO::Unbind() const {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

}
