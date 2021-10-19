#pragma once

#include <map>
#include <variant>
#include <glm/glm.hpp>

#include "core/log.h"
#include "components/component.h"
#include "components/uniform.h"

namespace components {

    // forward declaration
    class Shader;
    class Texture;

    // supported uniform variants
    using uniform_variant = std::variant<
        uni_int, uni_bool, uni_float, uni_vec2, uni_vec3, uni_vec4, uni_mat2, uni_mat3, uni_mat4
    >;

    static_assert(std::variant_size_v<uniform_variant> == 9);

    /* every renderable entity has a material component attached to it (FBO is a special case
       that has a virtual material attached to it), you can think of material as a container
       that glues together a shader ref and its associated texture refs or uniforms, which is
       used to automate some tedious tasks such as uploading uniforms or binding textures. If
       you are familiar with Unity, you'll probably notice that this design is very much the
       same idea, but comes with a much more simplified version of implementation.

       this class is very important as the renderer depends on it to decide what preparation
       work should be done before and after each draw call.

       the constructor does nothing by itself until the user assigns a shader ref to it. The
       function `SetShader()` takes in a valid shader program and parses it to automatically
       decide the structure of all active uniforms and textures in that shader. Once the data
       of these values have been correctly setup, they will be automatically uploaded to the
       shader every time when `Bind()` is called, similarly, the `Unbind()` function is used
       to reset the state (unbind the textures and the shader) after the draw call.

       to use this class correctly, users must clearly understand what these 3 set functions
       do: `SetShader()` often only needs to be setup once in the scene initialization code,
       the same also applies to `SetTexture()`, since the class will remember your choices.
       however, users need to pay attention to `SetUniform()` so as not to misunderstand it.
       this function does not update uniforms immediately, but only remembers their values in
       the uniform variants and set the `pending_upload` flag to true, the uniform class will
       then upload the values to the GPU when the material is bound. Note that the flag will
       be reset to false after each upload, so it happens only once, because we would like to
       avoid unnecessary uploads.
       
       unless a uniform never changes its value between frames, you should always set uniform
       every frame. This is true even if a uniform is constant-value but the shader is shared
       across multiple entities. For example, we'll often have a universal PBR shader that is
       referenced by multiple materials, which is the purpose of making the shader an asset
       ref so that you don't create 100 shaders with the same code for 100 different entities
       and then bind/unbind 100 times, or create 100 GLSL files that only differ by a uniform
       value. In this case, since the PBR shader is reused by the next entity's material, a
       "constant" uniform is very likely to be overwritten by the next draw call of the other
       entity. Just remember that setting uniforms is cheap, it doesn't hurt to do so every
       frame, in constrast, binding operations for buffers, shaders and textures is expensive.
    */

    class Material : public Component {
      private:
        std::map<GLuint, uniform_variant> uniforms;
        std::map<GLuint, asset_ref<Texture>> textures;

        void LoadActiveUniforms();

      public:
        asset_ref<Shader> shader;

        Material();
        ~Material() {}

        Material(const Material&) = default;
        Material& operator=(const Material&) = default;

        Material(Material&& other) noexcept;
        Material& operator=(Material&& other) noexcept;

        bool Bind() const;
        void Unbind() const;

        template<typename T>
        void SetUniform(GLuint location, const T& value, bool bind = false);
        void SetShader(asset_ref<Shader> shader_ref);
        void SetTexture(GLuint unit, asset_ref<Texture> texture_ref);
    };

}
