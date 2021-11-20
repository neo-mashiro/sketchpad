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

    /* scene class is the base class for all other scenes in the inheritance hierarchy.
       to make a new scene, simply inherit this class and override these three virtual
       functions listed below in your derived scene class. Here in the base class they
       are only used to display the welcome screen.

       > virtual void Init(void);
       > virtual void OnSceneRender(void);
       > virtual void OnImGuiRender(void);

       the renderer class is in charge of this class as well as all the derived scenes,
       the three event functions above will be called in sequence every frame.

       the main duty of this class is to manage the creation and destruction of entity
       objects and register them in the entity-component system. With the help of ECS
       pool, we can keep track of the hierarchy info of each entity in the scene graph.
       the scene will also help users manage the framebuffers and uniform buffers as
       they are closely tied to almost any scene, however, other buffers such as SSBO,
       ILS and samplers should be directly managed by the user.

       # tips on debugging

       modern OpenGL applications use the debug message callback for catching errors,
       this callback is a convenient way for debugging, very much like the GLFW error
       callback. In order to register this callback successfully, a valid OpenGL debug
       context must be present first. However, such context may not be available when
       using freeglut on some drivers, so the callback is not always triggered (GLFW
       seems to be working on all drivers all the time). In case it's not 100 percent
       reliable, we also provide a `CheckGLError()` function alias that allows users
       to manually catch errors when debugging the scene code.

       the `CheckGLError()` function can be used in conjunction with the debug message
       callback to ensure 100% bug-free. You don't need to call it after every line of
       code, it's intended to be placed uniformly across the scene code, each call may
       have a unique "checkpoint" number that will be printed to the console when error
       occurs. This way, we can quicky find out which call is closest to the error.

       if we want to create an error on purpose, or know about a persistent error that
       is irrelevant to our application (e.g. caused by a third-party library), we can
       catch it manually using `CheckGLError(-1)`. By passing a checkpoint of -1, the
       function will silently ignore the error and suppress the console message.
    */

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

        virtual void Init(void);
        virtual void OnSceneRender(void);
        virtual void OnImGuiRender(void);

      private:
        asset_ref<Texture> welcome_screen;
        ImTextureID welcome_screen_texture_id;
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
        // some commonly used color presets
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
