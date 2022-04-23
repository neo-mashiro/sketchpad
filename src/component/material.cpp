#include "pch.h"

#include <glm/glm.hpp>
#include "core/base.h"
#include "core/app.h"
#include "core/log.h"
#include "component/material.h"
#include "utils/ext.h"

namespace component {

    template<typename T>
    Uniform<T>::Uniform(GLuint owner_id, GLuint location, const char* name)
        : owner_id(owner_id), location(location), name(name), size(1),
          value(0), value_ptr(nullptr), array_ptr(nullptr) {}

    template<typename T>
    void Uniform<T>::operator<<(const T& value) {
        this->value = value;
    }

    template<typename T>
    void Uniform<T>::operator<<=(const T* value_ptr) {
        this->value_ptr = value_ptr;
    }

    template<typename T>
    void Uniform<T>::operator<<=(const std::vector<T>* array_ptr) {
        this->array_ptr = array_ptr;
    }

    template<typename T>
    void Uniform<T>::Upload(T val, GLuint index) const {
        const GLuint& id = owner_id;
        const GLuint& lc = location + index;

        /**/ if constexpr (std::is_same_v<T, bool>)   { glProgramUniform1i(id, lc, static_cast<int>(val)); }
        else if constexpr (std::is_same_v<T, int>)    { glProgramUniform1i(id, lc, val); }
        else if constexpr (std::is_same_v<T, float>)  { glProgramUniform1f(id, lc, val); }
        else if constexpr (std::is_same_v<T, GLuint>) { glProgramUniform1ui(id, lc, val); }
        else if constexpr (std::is_same_v<T, vec2>)   { glProgramUniform2fv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec3>)   { glProgramUniform3fv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, vec4>)   { glProgramUniform4fv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, ivec2>)  { glProgramUniform2iv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, ivec3>)  { glProgramUniform3iv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, ivec4>)  { glProgramUniform4iv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, uvec2>)  { glProgramUniform2uiv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, uvec3>)  { glProgramUniform3uiv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, uvec4>)  { glProgramUniform4uiv(id, lc, 1, &val[0]); }
        else if constexpr (std::is_same_v<T, mat2>)   { glProgramUniformMatrix2fv(id, lc, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat3>)   { glProgramUniformMatrix3fv(id, lc, 1, GL_FALSE, &val[0][0]); }
        else if constexpr (std::is_same_v<T, mat4>)   { glProgramUniformMatrix4fv(id, lc, 1, GL_FALSE, &val[0][0]); }
        else {
            static_assert(const_false<T>, "Unspecified template uniform type T ...");
        }
    }

    template<typename T>
    void Uniform<T>::Upload() const {
        if (size == 1) {
            T val = this->value_ptr ? *(value_ptr) : value;
            Upload(val);
            return;
        }

        for (GLuint i = 0; i < size; ++i) {
            T val = (*array_ptr)[i];
            Upload(val, i);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    Material::Material(const asset_ref<Shader>& shader_asset) : Component() {
        SetShader(shader_asset);

        // initialize built-in PBR uniform default values, see "pbr_metallic.glsl"
        // uniform locations not active in the shader will be silently ignored
        if (this->shader != nullptr) {
            // sampling booleans
            SetUniform(900U, false);                   // sample_albedo
            SetUniform(901U, false);                   // sample_normal
            SetUniform(902U, false);                   // sample_metallic
            SetUniform(903U, false);                   // sample_roughness
            SetUniform(904U, false);                   // sample_ao
            SetUniform(905U, false);                   // sample_emission
            SetUniform(906U, false);                   // sample_displace
            SetUniform(907U, false);                   // sample_opacity
            SetUniform(908U, false);                   // sample_lightmap
            SetUniform(909U, false);                   // sample_anisotan

            // shared properties
            SetUniform(912U, vec4(1.0f));              // albedo with alpha (not pre-multiplied)
            SetUniform(913U, 1.0f);                    // roughness
            SetUniform(914U, 1.0f);                    // ambient occlussion
            SetUniform(915U, vec4(vec3(0.0f), 1.0f));  // emission
            SetUniform(916U, vec2(1.0f));              // texture coordinates tiling factor
            SetUniform(928U, 0.0f);                    // alpha threshold

            // standard model
            SetUniform(917U, 0.0f);                    // metalness
            SetUniform(918U, 0.5f);                    // specular reflectance ~ [0.35, 1]
            SetUniform(919U, 0.0f);                    // anisotropy ~ [-1, 1]
            SetUniform(920U, vec3(1.0f, 0.0f, 0.0f));  // anisotropy direction

            // refraction model
            SetUniform(921U, 0.0f);                    // transmission
            SetUniform(922U, 2.0f);                    // thickness
            SetUniform(923U, 1.5f);                    // index of refraction (IOR)
            SetUniform(924U, vec3(1.0f));              // transmittance color
            SetUniform(925U, 4.0f);                    // transmission distance
            SetUniform(931U, 0U);                      // volume type, 0 = uniform sphere, 1 = cube/box/glass

            // cloth model
            SetUniform(926U, vec3(1.0f));              // sheen color
            SetUniform(927U, vec3(0.0f));              // subsurface color

            // additive clear coat layer
            SetUniform(929U, 0.0f);                    // clearcoat
            SetUniform(930U, 0.0f);                    // clearcoat roughness

            // shading model switch
            SetUniform(999U, uvec2(1, 0));             // uvec2 model
        }
    }

    Material::Material(const asset_ref<Material>& material_asset) : Material(*material_asset) {}  // calls copy ctor

    void Material::Bind() const {
        // visitor lambda function
        static auto upload = [](auto& unif) { unif.Upload(); };

        CORE_ASERT(shader, "Unable to bind the material, please set a valid shader first...");
        shader->Bind();  // smart bind the attached shader

        // upload uniform values to the shader
        for (const auto& [location, unif_variant] : uniforms) {
            std::visit(upload, unif_variant);
        }

        // smart bind textures to the slots
        for (const auto& [unit, texture] : textures) {
            if (texture != nullptr) {
                texture->Bind(unit);
            }
        }
    }

    void Material::Unbind() const {
        // thanks to smart shader and texture bindings, there's no need to unbind or cleanup
        // just keep the current rendering state and let the next material's bind does its work
        if constexpr (false) {
            for (const auto& [unit, texture] : textures) {
                if (texture != nullptr) {
                    texture->Unbind(unit);
                }
            }
            shader->Unbind();
        }
    }

    void Material::SetShader(asset_ref<Shader> shader_ref) {
        glUseProgram(0);
        uniforms.clear();
        textures.clear();
        this->shader = shader_ref;  // share ownership

        // if nullptr is passed in, the intention is to reset the material to a clean empty state
        if (shader == nullptr) {
            return;
        }

        // load active uniforms from the shader and cache them into the `uniforms` std::map
        GLuint id = shader->ID();
        CORE_INFO("Parsing active uniforms in shader (id = {0}): ...", id);

        GLint n_uniforms;
        glGetProgramInterfaceiv(id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &n_uniforms);

        GLenum meta_data[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX };

        for (int i = 0; i < n_uniforms; ++i) {
            GLint unif_info[4] {};
            glGetProgramResourceiv(id, GL_UNIFORM, i, 4, meta_data, 4, NULL, unif_info);

            if (unif_info[3] != -1) {
                continue;  // skip uniforms in blocks (will be handled by UBO)
            }

            GLint name_length = unif_info[0];
            char* name = new char[name_length];
            glGetProgramResourceName(id, GL_UNIFORM, i, name_length, NULL, name);

            GLenum type = unif_info[1];
            GLuint loc = unif_info[2];

            // note that OpenGL will return us all resources that have the keyword "uniform", which
            // not only include uniforms in blocks, but also all types of samplers, images and even
            // atomic counters, which are not really the uniforms we want (basic, non-opaque types)

            static const std::vector<GLenum> valid_types {
                GL_INT, GL_INT_VEC2, GL_INT_VEC3, GL_INT_VEC4, GL_BOOL, GL_BOOL_VEC2, GL_BOOL_VEC3, GL_BOOL_VEC4,
                GL_UNSIGNED_INT, GL_UNSIGNED_INT_VEC2, GL_UNSIGNED_INT_VEC3, GL_UNSIGNED_INT_VEC4,
                GL_FLOAT, GL_FLOAT_VEC2, GL_FLOAT_VEC3, GL_FLOAT_VEC4, GL_FLOAT_MAT2, GL_FLOAT_MAT3, GL_FLOAT_MAT4,
                GL_DOUBLE, GL_DOUBLE_VEC2, GL_DOUBLE_VEC3, GL_DOUBLE_VEC4, GL_DOUBLE_MAT2, GL_DOUBLE_MAT3, GL_DOUBLE_MAT4,
                GL_FLOAT_MAT2x3, GL_FLOAT_MAT2x4, GL_FLOAT_MAT3x2, GL_FLOAT_MAT3x4, GL_FLOAT_MAT4x2, GL_FLOAT_MAT4x3,
                GL_DOUBLE_MAT2x3, GL_DOUBLE_MAT2x4, GL_DOUBLE_MAT3x2, GL_DOUBLE_MAT3x4, GL_DOUBLE_MAT4x2, GL_DOUBLE_MAT4x3
            };

            if (utils::ranges::find(valid_types, type) == valid_types.end()) {
                delete[] name;  // don't forget to free memory if we were to break early
                continue;       // skip fake uniforms (opaque type samplers and images)
            }

            #define IN_PLACE_CONSTRUCT_UNI_VARIANT(i) \
                uniforms.try_emplace(loc, std::in_place_index<i>, id, loc, name);

            switch (type) {
                case GL_INT:                IN_PLACE_CONSTRUCT_UNI_VARIANT(0);   break;
                case GL_UNSIGNED_INT:       IN_PLACE_CONSTRUCT_UNI_VARIANT(1);   break;
                case GL_BOOL:               IN_PLACE_CONSTRUCT_UNI_VARIANT(2);   break;
                case GL_FLOAT:              IN_PLACE_CONSTRUCT_UNI_VARIANT(3);   break;
                case GL_FLOAT_VEC2:         IN_PLACE_CONSTRUCT_UNI_VARIANT(4);   break;
                case GL_FLOAT_VEC3:         IN_PLACE_CONSTRUCT_UNI_VARIANT(5);   break;
                case GL_FLOAT_VEC4:         IN_PLACE_CONSTRUCT_UNI_VARIANT(6);   break;
                case GL_UNSIGNED_INT_VEC2:  IN_PLACE_CONSTRUCT_UNI_VARIANT(7);   break;
                case GL_UNSIGNED_INT_VEC3:  IN_PLACE_CONSTRUCT_UNI_VARIANT(8);   break;
                case GL_UNSIGNED_INT_VEC4:  IN_PLACE_CONSTRUCT_UNI_VARIANT(9);   break;
                case GL_FLOAT_MAT2:         IN_PLACE_CONSTRUCT_UNI_VARIANT(10);  break;
                case GL_FLOAT_MAT3:         IN_PLACE_CONSTRUCT_UNI_VARIANT(11);  break;
                case GL_FLOAT_MAT4:         IN_PLACE_CONSTRUCT_UNI_VARIANT(12);  break;
                case GL_INT_VEC2:           IN_PLACE_CONSTRUCT_UNI_VARIANT(13);  break;
                case GL_INT_VEC3:           IN_PLACE_CONSTRUCT_UNI_VARIANT(14);  break;
                case GL_INT_VEC4:           IN_PLACE_CONSTRUCT_UNI_VARIANT(15);  break;
                default: {
                    CORE_ERROR("Uniform \"{0}\" is using an unsupported type!", name);
                    SP_DBG_BREAK();
                }
            }

            #undef IN_PLACE_CONSTRUCT_UNI_VARIANT

            delete[] name;
        }
    }

    void Material::SetTexture(GLuint unit, asset_ref<Texture> texture_ref) {
        size_t n_textures = 0;
        for (const auto& [_, texture] : textures) {
            if (texture != nullptr) {
                n_textures++;
            }
        }

        static size_t max_samplers = core::Application::GetInstance().gl_max_texture_units;
        if (texture_ref != nullptr && n_textures >= max_samplers) {
            CORE_ERROR("{0} samplers limit has reached, failed to add texture...", max_samplers);
            return;
        }

        // if nullptr is passed in, the intention is to clear this texture unit
        textures[unit] = texture_ref;  // nullptr is ok, a null slot will be skipped on `Bind()`
    }

    void Material::SetTexture(pbr_t attribute, asset_ref<Texture> texture_ref) {
        GLuint texture_unit = static_cast<GLuint>(attribute);
        SetTexture(texture_unit, texture_ref);

        if (texture_unit >= 20) {
            GLuint sample_loc = texture_unit + 880U;  // "sample_xxx" uniform locations
            SetUniform(sample_loc, texture_ref ? true : false);
        }
    }

    template<typename T, typename>
    void Material::SetUniform(GLuint location, const T& value) {
        if (uniforms.count(location) > 0) {  // ignore inactive uniforms
            auto& unif_variant = uniforms[location];
            CORE_ASERT(std::holds_alternative<Uniform<T>>(unif_variant), "Mismatched uniform type!");
            std::get<Uniform<T>>(unif_variant) << value;
        }
    }

    template<typename T, typename>
    void Material::SetUniform(pbr_u attribute, const T& value) {
        SetUniform<T>(static_cast<GLuint>(attribute), value);
    }

    template<typename T, typename>
    void Material::BindUniform(GLuint location, const T* value_ptr) {
        if (uniforms.count(location) > 0) {
            auto& unif_variant = uniforms[location];
            CORE_ASERT(std::holds_alternative<Uniform<T>>(unif_variant), "Mismatched uniform type!");
            std::get<Uniform<T>>(unif_variant) <<= value_ptr;
        }
    }

    template<typename T, typename>
    void Material::BindUniform(pbr_u attribute, const T* value_ptr) {
        BindUniform<T>(static_cast<GLuint>(attribute), value_ptr);
    }

    template<typename T, typename>
    void Material::SetUniformArray(GLuint location, GLuint size, const std::vector<T>* array_ptr) {
        if (uniforms.count(location) > 0) {
            auto& unif_variant = uniforms[location];
            CORE_ASERT(std::holds_alternative<Uniform<T>>(unif_variant), "Mismatched uniform type!");
            auto& uniform = std::get<Uniform<T>>(unif_variant);
            uniform.size = size;
            uniform <<= array_ptr;
        }
    }

    // explicit template instantiation using X macro
    #define INSTANTIATE_TEMPLATE(T) \
        template class Uniform<T>; \
        template void Material::SetUniform<T>(GLuint location, const T& value); \
        template void Material::SetUniform<T>(pbr_u attribute, const T& value); \
        template void Material::BindUniform<T>(GLuint location, const T* value_ptr); \
        template void Material::BindUniform<T>(pbr_u attribute, const T* value_ptr); \
        template void Material::SetUniformArray(GLuint location, GLuint size, const std::vector<T>* array_ptr);

    INSTANTIATE_TEMPLATE(int)
    INSTANTIATE_TEMPLATE(GLuint)
    INSTANTIATE_TEMPLATE(bool)
    INSTANTIATE_TEMPLATE(float)
    INSTANTIATE_TEMPLATE(vec2)
    INSTANTIATE_TEMPLATE(vec3)
    INSTANTIATE_TEMPLATE(vec4)
    INSTANTIATE_TEMPLATE(mat2)
    INSTANTIATE_TEMPLATE(mat3)
    INSTANTIATE_TEMPLATE(mat4)
    INSTANTIATE_TEMPLATE(ivec2)
    INSTANTIATE_TEMPLATE(ivec3)
    INSTANTIATE_TEMPLATE(ivec4)
    INSTANTIATE_TEMPLATE(uvec2)
    INSTANTIATE_TEMPLATE(uvec3)
    INSTANTIATE_TEMPLATE(uvec4)

    #undef INSTANTIATE_TEMPLATE

}
