#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "buffer/buffer.h"

namespace buffer {

    Buffer::Buffer() : id(0) {
        CORE_ASERT(core::Application::IsContextActive(), "OpenGL context not found: {0}", __FUNCSIG__);
    }

}
