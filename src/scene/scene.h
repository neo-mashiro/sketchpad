#pragma once

#include <map>
#include <string>
#include "core/log.h"
#include "buffer/all.h"
#include "components/all.h"
#include "scene/entity.h"

using namespace buffer;
using namespace components;

namespace scene {

    // OpenGL debug message callback is not always triggered when error occurs, how it
    // behaves is up to the driver (may vary across drivers) so it's not 100% reliable
    // when debugging the scene code, use this function alias to manually catch errors

    template<typename... Args>
    auto CheckGLError(Args&&... args) ->
        decltype(core::Log::CheckGLError(std::forward<Args>(args)...)) {
        return core::Log::CheckGLError(std::forward<Args>(args)...);
    }

    class Scene {
      private:
        friend class Renderer;
        entt::registry registry;
        std::map<entt::entity, std::string> directory;
        
      protected:
        std::map<GLuint, UBO> UBOs;  // stores a uniform buffer at each binding point
        std::map<GLuint, FBO> FBOs;  // stores a list of indexed framebuffers

        void AddUBO(GLuint shader_id);
        FBO& AddFBO(GLuint width, GLuint height);

        Entity CreateEntity(const std::string& name, ETag tag = ETag::Untagged);
        void DestroyEntity(Entity entity);

      public:
        std::string title;

        explicit Scene(const std::string& title);
        virtual ~Scene();

        // these virtual functions should be overridden by each derived class
        // here in the base class, they are used to render the welcome screen
        virtual void Init(void);
        virtual void OnSceneRender(void);
        virtual void OnImGuiRender(void);
    };

    namespace world {
        // world space constants (OpenGL adopts a right-handed coordinate system)
        constexpr glm::vec3 origin { 0.0f };
        constexpr glm::vec3 zero { 0.0f };
        constexpr glm::vec3 unit { 1.0f };
        constexpr glm::mat4 eye { 1.0f };
        constexpr glm::vec3 up { 0.0f, 1.0f, 0.0f };
        constexpr glm::vec3 forward { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 right { -1.0f, 0.0f, 0.0f };
    }

    namespace color {
        constexpr glm::vec3 white { 1.0f };
        constexpr glm::vec3 black { 0.0f };
        constexpr glm::vec3 red { 1.0f, 0.0f, 0.0f };
        constexpr glm::vec3 green { 0.0f, 1.0f, 0.0f };
        constexpr glm::vec3 lime { 0.5f, 1.0f, 0.0f };
        constexpr glm::vec3 blue { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 cyan { 0.0f, 1.0f, 1.0f };
        constexpr glm::vec3 yellow { 1.0f, 1.0f, 0.0f };
        constexpr glm::vec3 purple { 0.5f, 0.0f, 1.0f };
    }

}
