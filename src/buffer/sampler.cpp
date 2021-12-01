#include "pch.h"

#include <type_traits>
#include "core/debug.h"
#include "buffer/sampler.h"
#include "buffer/texture.h"

namespace buffer {

    Sampler::Sampler() : Buffer() {
        glCreateSamplers(1, &id);
    }

    Sampler::~Sampler() {
        glDeleteSamplers(1, &id);
    }

    void Sampler::Bind(GLuint unit) const {
        glBindSampler(unit, id);
    }

    void Sampler::Unbind(GLuint unit) const {
        glBindSampler(unit, 0);
    }

    template<typename T>
    void Sampler::SetParam(GLenum name, T value) const {
        if constexpr (std::is_same_v<T, int>) {
            glSamplerParameteri(id, name, value);
        }
        else if constexpr (std::is_same_v<T, float>) {
            glSamplerParameterf(id, name, value);
        }
    }

    template<typename T>
    void Sampler::SetParam(GLenum name, const T* value) const {
        if constexpr (std::is_same_v<T, int>) {
            glSamplerParameteriv(id, name, value);
        }
        else if constexpr (std::is_same_v<T, float>) {
            glSamplerParameterfv(id, name, value);
        }
    }

    // explicit template functon instantiation
    template void Sampler::SetParam<int>(GLenum name, int value) const;
    template void Sampler::SetParam<int>(GLenum name, const int* value) const;
    template void Sampler::SetParam<float>(GLenum name, float value) const;
    template void Sampler::SetParam<float>(GLenum name, const float* value) const;

    void Sampler::SetDefaultState(const Texture& texture) {
        GLuint tid = texture.GetID();
        GLenum target = texture.target;
        GLuint n_levels = texture.n_levels;

        // for magnification, bilinear filtering is more than enough, for minification,
        // trilinear filtering is only necessary when we need to sample across mipmaps
        GLint mag_filter = GL_LINEAR;
        GLint min_filter = n_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;

        // anisotropic filtering requires OpenGL 4.6, where maximum anisotropy is implementation-defined
        GLfloat max_anisotropy = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        GLfloat anisotropy = std::clamp(max_anisotropy, 1.0f, 8.0f);  // limit anisotropy to 8

        if (target == GL_TEXTURE_2D) {
            glTextureParameteri(tid, GL_TEXTURE_WRAP_S, GL_REPEAT);  // 2D textures are mostly seamless
            glTextureParameteri(tid, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTextureParameteri(tid, GL_TEXTURE_MIN_FILTER, min_filter);
            glTextureParameteri(tid, GL_TEXTURE_MAG_FILTER, mag_filter);
            glTextureParameterf(tid, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
        }
        else if (target == GL_TEXTURE_2D_MULTISAMPLE) {
            // multisampled textures are not filtered at all, there's nothing we need to do here because
            // we'll never sample them, the hardware takes care of all the multisample operations for us.
            // in fact, if we were to set any of the sampler states, we'll get a `GL_INVALID_ENUM` error.
            return;
        }
        else if (target == GL_TEXTURE_CUBE_MAP) {
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
            static const float border[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            glTextureParameteri(tid, GL_TEXTURE_MIN_FILTER, min_filter);
            glTextureParameteri(tid, GL_TEXTURE_MAG_FILTER, mag_filter);
            glTextureParameteri(tid, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(tid, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTextureParameteri(tid, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
            glTextureParameterfv(tid, GL_TEXTURE_BORDER_COLOR, border);
        }
        else {
            throw core::NotImplementedError("Texture target not yet implemented...");
        }
    }

}
