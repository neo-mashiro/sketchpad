#include "pch.h"

#include "core/app.h"
#include "core/log.h"
#include "buffer/texture.h"
#include "components/shader.h"
#include "components/material.h"
#include "utils/filesystem.h"

// #ifdef _DEBUG
// #define DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// #else
// #define DEBUG_NEW new
// #endif

namespace components {

    // list of currently supported sampler types
    static const std::vector<GLenum> samplers {
        GL_SAMPLER_2D,             // sampler2D
        GL_SAMPLER_3D,             // sampler3D
        GL_SAMPLER_CUBE,           // samplerCube
        GL_SAMPLER_1D_SHADOW,      // sampler1DShadow
        GL_SAMPLER_2D_SHADOW,      // sampler2DShadow
        GL_SAMPLER_CUBE_SHADOW,    // samplerCubeShadow
        GL_SAMPLER_2D_MULTISAMPLE  // sampler2DMS
    };

    Material::Material(asset_ref<Shader> shader_asset) {
        SetShader(shader_asset);
    }

    Material::Material(asset_ref<Material> material_asset) : Material(*material_asset) {}

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

    bool Material::Bind() const {
        // visitor lambda function
        static auto conditional_upload = [](auto& unif) {
            if (unif.pending_upload || unif.binding_upload) {
                unif.Upload();
            }
        };

        CORE_ASERT(shader != nullptr, "Unable to bind the material, please set a valid shader first...");
        shader->Bind();  // the rendering state won't be changed if the shader is already bound

        // upload uniform values to the shader
        for (const auto& pair : uniforms) {
            std::visit(conditional_upload, pair.second);
        }

        // rebind textures to the unit slots
        for (const auto& pair : textures) {
            if (auto& texture = pair.second; texture != nullptr) {
                texture->Bind(pair.first);
            }
        }

        return true;
    }

    void Material::Unbind() const {
        // our shader, texture and uniform class now support smart bindings and smart uploads,
        // there's no need to unbind the shader or clean up the texture units, so we'll simply
        // keep the current rendering state untouched, keep the texture bindings, let the next
        // object's material bind does its optimization work as much as possible.

        // for (const auto& pair : textures) {
        //     if (auto& texture = pair.second; texture != nullptr) {
        //         texture->Unbind(pair.first);
        //     }
        // }

        // shader->Unbind();
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
        size_t max_samplers = core::Application::GetInstance().gl_max_texture_units;
        size_t n_textures = 0;

        for (const auto& pair : textures) {
            if (pair.second != nullptr) {
                n_textures++;
            }
        }

        if (texture_ref != nullptr && n_textures >= max_samplers) {
            CORE_WARN("{0} samplers limit has reached, failed to add texture...", max_samplers);
            return;
        }

        // even if texture_ref is nullptr, we don't really need to erase or clear the unit slot
        // it's fine to set the slot to nullptr, a null slot will be skipped on bind and unbind
        textures[unit] = texture_ref;
    }

    template<typename T>
    void Material::SetUniform(GLuint location, const T& value, bool bind) {
        if (uniforms.count(location) == 0) {
            static bool warned = false;
            if (!warned) {
                CORE_WARN("Uniform location {0} is not active in shader: {1}", location, shader->GetID());
                CORE_WARN("The uniform may have been optimized out by the GLSL compiler");
                warned = true;
            }
            return;
        }

        auto& unif_variant = uniforms[location];

        if (!std::holds_alternative<Uniform<T>>(unif_variant)) {
            CORE_ERROR("Mismatched value type, unable to set uniform in {0}", __FUNCSIG__);
            __debugbreak();
        }

        if (bind) {
            std::get<Uniform<T>>(unif_variant) <<= &value;
        }
        else {
            std::get<Uniform<T>>(unif_variant) << value;
        }
    }

    void Material::LoadActiveUniforms() {
        GLuint id = shader->GetID();
        CORE_INFO("Parsing active uniforms in shader (id = {0}): ...", id);

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

            GLenum gl_type = unif_info[1];
            GLint location = unif_info[2];

            if (std::find(samplers.begin(), samplers.end(), gl_type) != samplers.end()) {
                delete[] name;  // don't forget to free memory if we were to break early
                continue;       // skip sampler uniforms (will be handled by `SetTexture`)
            }

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

    using namespace glm;

    // explicit template functon instantiation
    template void Material::SetUniform<int>  (GLuint location, const int& value,   bool bind);
    template void Material::SetUniform<bool> (GLuint location, const bool& value,  bool bind);
    template void Material::SetUniform<float>(GLuint location, const float& value, bool bind);
    template void Material::SetUniform<vec2> (GLuint location, const vec2& value,  bool bind);
    template void Material::SetUniform<vec3> (GLuint location, const vec3& value,  bool bind);
    template void Material::SetUniform<vec4> (GLuint location, const vec4& value,  bool bind);
    template void Material::SetUniform<mat2> (GLuint location, const mat2& value,  bool bind);
    template void Material::SetUniform<mat3> (GLuint location, const mat3& value,  bool bind);
    template void Material::SetUniform<mat4> (GLuint location, const mat4& value,  bool bind);

}
