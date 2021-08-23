#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "components/shader.h"
#include "components/texture.h"
#include "components/material.h"

namespace components {

    Material::Material() {
        if (max_samplers == 0) {
            max_samplers = core::Application::GetInstance().gl_max_texture_units;
        }
    }

    Material::Material(Material&& other) noexcept {
        *this = std::move(other);
    }

    Material& Material::operator=(Material&& other) noexcept {
        if (this != &other) {
            std::swap(shader, other.shader);
            std::swap(uniforms, other.uniforms);
            std::swap(textures, other.textures);
        }

        return *this;
    }

    void Material::SetShader(asset_ref<Shader> shader_ref) {
        glUseProgram(0);
        uniforms.clear();
        textures.clear();
        this->shader = shader_ref;

        if (shader) {
            LoadActiveUniforms();
        }
    }

    void Material::SetTexture(GLuint unit, asset_ref<Texture> texture_ref) {
        // remove texture from the unit slot
        if (texture_ref == nullptr) {
            if (textures.count(unit) == 0) {
                CORE_WARN("Unit {0} is already empty, operation ignored...", unit);
                return;
            }

            textures.erase(unit);
        }

        // add texture to the unit slot
        else {
            if (textures.size() >= max_samplers) {
                CORE_WARN("{0} samplers limit has reached, failed to add texture...", max_samplers);
                return;
            }

            if (textures.count(unit) > 0) {
                CORE_WARN("Unit {0} is not empty, previous texture will be replaced...", unit);
            }

            textures[unit] = texture_ref;
        }
    }

    bool Material::Bind() const {
        CORE_ASERT(shader, "Unable to bind the material, please set a valid shader first...");
        shader->Bind();

        // rebind textures to the unit slots
        for (const auto& pair : textures) {
            auto& texture = pair.second;
            glActiveTexture(GL_TEXTURE0 + pair.first);
            glBindTexture(texture->target, texture->id);
        }

        auto conditional_upload = [](auto& unif) {
            if (unif.pending_upload || unif.binding_upload) {
                unif.Upload();
            }
        };

        // upload uniform values to the shader
        for (const auto& pair : uniforms) {
            std::visit(conditional_upload, pair.second);
        }

        return (shader->id) > 0;
    }

    void Material::Unbind() const {
        // clean up texture units
        for (const auto& pair : textures) {
            auto& texture = pair.second;
            glActiveTexture(GL_TEXTURE0 + pair.first);
            glBindTexture(texture->target, 0);
        }

        CORE_ASERT(shader, "Unable to unbind the material, please set a valid shader first...");
        shader->Unbind();
    }

    void Material::LoadActiveUniforms() {
        GLuint id = shader->id;
        CORE_INFO("Parsing active uniforms in shader: (id = {0})...", id);

        GLint n_uniforms;
        glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &n_uniforms);

        GLenum meta_data[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };

        for (int i = 0; i < n_uniforms; ++i) {
            GLint unif_info[4] {};
            glGetProgramResourceiv(id, GL_UNIFORM, i, 4, meta_data, 4, NULL, unif_info);

            if (unif_info[3] != -1) {
                continue;  // skip uniforms in blocks
            }

            GLint name_length = unif_info[0];
            char* name = new char[name_length];
            glGetProgramResourceName(id, GL_UNIFORM, i, name_length, NULL, name);

            GLint gl_type = unif_info[1];
            GLint location = unif_info[2];

            // we can't wrap this into a function because in_place_index must be compile-time constants
            switch (gl_type) {
                case GL_INT:
                    uniforms.try_emplace(location, std::in_place_index<0>, id, location, name); break;
                case GL_BOOL:
                    uniforms.try_emplace(location, std::in_place_index<1>, id, location, name); break;
                case GL_FLOAT:
                    uniforms.try_emplace(location, std::in_place_index<2>, id, location, name); break;
                case GL_FLOAT_VEC2:
                    uniforms.try_emplace(location, std::in_place_index<3>, id, location, name); break;
                case GL_FLOAT_VEC3:
                    uniforms.try_emplace(location, std::in_place_index<4>, id, location, name); break;
                case GL_FLOAT_VEC4:
                    uniforms.try_emplace(location, std::in_place_index<5>, id, location, name); break;
                case GL_FLOAT_MAT2:
                    uniforms.try_emplace(location, std::in_place_index<6>, id, location, name); break;
                case GL_FLOAT_MAT3:
                    uniforms.try_emplace(location, std::in_place_index<7>, id, location, name); break;
                case GL_FLOAT_MAT4:
                    uniforms.try_emplace(location, std::in_place_index<8>, id, location, name); break;
                default:
                    CORE_ERROR("Uniform \"{0}\" is using an unsupported type!", name);
                    CORE_ERROR("Please extend uniform variants in the Material API...");
                    __debugbreak();
            }

            delete[] name;
        }
    }

}
