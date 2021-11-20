#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "buffer/buffer.h"

namespace buffer {

    Buffer::Buffer() : id(0) {
        CORE_ASERT(core::Application::GLContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
    }

    GLuint Buffer::GetID() const {
        return this->id;
    }

}
