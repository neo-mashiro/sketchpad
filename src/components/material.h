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
        Uniform<int>,        // index 0
        Uniform<bool>,       // index 1
        Uniform<float>,      // index 2
        Uniform<glm::vec2>,  // index 3
        Uniform<glm::vec3>,  // index 4
        Uniform<glm::vec4>,  // index 5
        Uniform<glm::mat2>,  // index 6
        Uniform<glm::mat3>,  // index 7
        Uniform<glm::mat4>   // index 8
    >;

    static_assert(std::variant_size_v<uniform_variant> == 9);

    class Material : public Component {
      private:
        static inline GLuint max_samplers = 0;
        std::map<GLuint, uniform_variant> uniforms;
        std::map<GLuint, asset_ref<Texture>> textures;

        void LoadActiveUniforms();

      public:
        asset_ref<Shader> shader;
        static inline bool depth_prepass = false;

        Material();
        ~Material() {}

        Material(const Material&) = default;
        Material& operator=(const Material&) = default;

        Material(Material&& other) noexcept;
        Material& operator=(Material&& other) noexcept;

        void SetShader(asset_ref<Shader> shader_ref);
        void SetTexture(GLuint unit, asset_ref<Texture> texture_ref);

        template<typename T>
        void SetUniform(GLuint location, const T& value, bool bind = false) {
            // early z-test in forward+ rendering only accepts the model matrix (uniform location 0)
            if (depth_prepass && location != 0) {
                return;
            }

            if (uniforms.count(location) == 0) {
                static bool warned = false;
                if (!warned) {
                    CORE_WARN("Uniform location {0} is not active in shader: {1}", location, shader->id);
                    CORE_WARN("The uniform may have been optimized out by the GLSL compiler");
                    warned = true;
                }
                return;
            }

            auto& unif_variant = uniforms[location];

            if (!std::holds_alternative<Uniform<T>>(unif_variant)) {
                CORE_ERROR("Mismatched value type, unable to set uniform in {0}", __FUNCSIG__);
                return;
            }

            if (bind) {
                std::get<Uniform<T>>(unif_variant) <<= &value;
            }
            else {
                std::get<Uniform<T>>(unif_variant) << value;
            }
        }

        bool Bind() const;
        void Unbind() const;
    };

}
