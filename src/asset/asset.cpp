#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "asset/asset.h"
#include "utils/ext.h"

namespace asset {

    IAsset::IAsset() : id(0) {
        CORE_ASERT(core::Application::GLContextActive(), "OpenGL context not found: {0}", utils::func_sig());
    }

    IAsset::IAsset(IAsset&& other) noexcept : id { std::exchange(other.id, 0) } {}

    IAsset& IAsset::operator=(IAsset&& other) noexcept {
        if (this != &other) {
            this->id = std::exchange(other.id, 0);
        }
        return *this;
    }

}
