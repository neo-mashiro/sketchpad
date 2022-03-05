/* a material component glues together a shader and its associated textures and uniforms,
   it's designed to ease the preparation setup for rendering, which internally automates
   the tasks of uniform uploads, smart shader binding and smart texture bindings.

   the usage of this class is very similar to Unity's material system, despite that our
   implementation is much simplified. In our physically-based rendering pipeline, a PBR
   shader is often shared by multiple entities in the scene, so that we don't create 100
   shader programs with the exact same code for 100 different meshes. It's then the duty
   of the material component to identify a particular entity's shading inputs, it will
   remember all the uniform values and textures of every individual entity.
*/

#pragma once

#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <glad/glad.h>

#include "asset/shader.h"
#include "asset/texture.h"
#include "component/component.h"

namespace component {

    using vec2 = glm::vec2;
    using vec3 = glm::vec3;
    using vec4 = glm::vec4;
    using mat2 = glm::mat2;
    using mat3 = glm::mat3;
    using mat4 = glm::mat4;

    using uvec2 = glm::uvec2;
    using uvec3 = glm::uvec3;
    using uvec4 = glm::uvec4;
    using ivec2 = glm::ivec2;
    using ivec3 = glm::ivec3;
    using ivec4 = glm::ivec4;

    // allowed uniform types: int, uint, bool, float + 32-bit float vector/matrix
    template<typename T>
    struct is_glsl_t {
        using type = typename std::enable_if_t<
            std::is_same_v<T, int>    ||
            std::is_same_v<T, bool>   ||
            std::is_same_v<T, float>  ||
            std::is_same_v<T, GLuint> ||
            std::is_same_v<T, vec2>   || std::is_same_v<T, uvec2> || std::is_same_v<T, ivec2> ||
            std::is_same_v<T, vec3>   || std::is_same_v<T, uvec3> || std::is_same_v<T, ivec3> ||
            std::is_same_v<T, vec4>   || std::is_same_v<T, uvec4> || std::is_same_v<T, ivec4> ||
            std::is_same_v<T, mat2>   ||
            std::is_same_v<T, mat3>   ||
            std::is_same_v<T, mat4>
        >;
    };

    template<typename T>
    class Uniform {
      public:
        std::string name;
        GLuint location;
        GLuint owner_id;  // which shader owns this uniform
        T value;
        const T* value_ptr;

      public:
        Uniform() = default;
        Uniform(GLuint owner_id, GLuint location, const char* name);

        void operator<<(const T& value);
        void operator<<=(const T* value_ptr);
        void Upload() const;
    };

    enum class pbr_u : uint16_t {
        albedo        = 912U,
        roughness     = 913U,
        ao            = 914U,
        emission      = 915U,
        uv_scale      = 916U,
        alpha_mask    = 928U,
        metalness     = 917U,
        specular      = 918U,
        anisotropy    = 919U,
        aniso_dir     = 920U,
        transmission  = 921U,
        thickness     = 922U,
        ior           = 923U,
        transmittance = 924U,
        tr_distance   = 925U,
        volume_type   = 931U,
        sheen_color   = 926U,
        subsurf_color = 927U,
        clearcoat     = 929U,
        cc_roughness  = 930U,
        shading_model = 999U
    };

    enum class pbr_t : uint8_t {
        irradiance_map  = 17U,
        prefiltered_map = 18U,
        BRDF_LUT        = 19U,
        albedo          = 20U,
        normal          = 21U,
        metallic        = 22U,
        roughness       = 23U,
        ao              = 24U,
        emission        = 25U,
        displace        = 26U,
        opacity         = 27U,
        lightmap        = 28U,
        anisotropic     = 29U
    };

    class Material : public Component {
      private:
        using uni_int   = Uniform<int>;
        using uni_uint  = Uniform<GLuint>;
        using uni_bool  = Uniform<bool>;
        using uni_float = Uniform<float>;
        using uni_vec2  = Uniform<vec2>;
        using uni_vec3  = Uniform<vec3>;
        using uni_vec4  = Uniform<vec4>;
        using uni_mat2  = Uniform<mat2>;
        using uni_mat3  = Uniform<mat3>;
        using uni_mat4  = Uniform<mat4>;
        using uni_uvec2 = Uniform<uvec2>;
        using uni_uvec3 = Uniform<uvec3>;
        using uni_uvec4 = Uniform<uvec4>;
        using uni_ivec2 = Uniform<ivec2>;
        using uni_ivec3 = Uniform<ivec3>;
        using uni_ivec4 = Uniform<ivec4>;

        using uniform_variant = std::variant<
            uni_int, uni_uint, uni_bool, uni_float, uni_vec2, uni_vec3, uni_vec4, uni_mat2, uni_mat3, uni_mat4,
            uni_uvec2, uni_uvec3, uni_uvec4, uni_ivec2, uni_ivec3, uni_ivec4
        >;

        static_assert(std::variant_size_v<uniform_variant> == 16);

      private:
        using Shader  = asset::Shader;
        using Texture = asset::Texture;

        asset_ref<Shader> shader;
        std::map<GLuint, uniform_variant> uniforms;
        std::map<GLuint, asset_ref<Texture>> textures;

      public:
        Material(const asset_ref<Shader>& shader_asset);
        Material(const asset_ref<Material>& material_asset);

        void Bind() const;
        void Unbind() const;

        void SetShader(asset_ref<Shader> shader_ref);
        void SetTexture(GLuint unit, asset_ref<Texture> texture_ref);
        void SetTexture(pbr_t attribute, asset_ref<Texture> texture_ref);

        template<typename T, typename = is_glsl_t<T>>
        void SetUniform(GLuint location, const T& value);

        template<typename T, typename = is_glsl_t<T>>
        void SetUniform(pbr_u attribute, const T& value);

        template<typename T, typename = is_glsl_t<T>>
        void BindUniform(GLuint location, const T* value_ptr);

        template<typename T, typename = is_glsl_t<T>>
        void BindUniform(pbr_u attribute, const T* value_ptr);
    };

}
