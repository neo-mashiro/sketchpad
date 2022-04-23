/* 
   scene class is the base class for all other scenes in the inheritance hierarchy.
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
   since framebuffers and uniform buffers are closely tied to almost any scene, we
   also provide functions and containers to add/access FBOs and UBOs conveniently.
   other assets such as SSBO, ATC, PBO and samplers are often needed case by case
   thus users are expected to manage them directly.
*/

#pragma once

#include <map>
#include <string>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include "core/base.h"
#include "asset/all.h"
#include "component/all.h"
#include "scene/entity.h"
#include "scene/resource.h"

using namespace asset;
using namespace component;

namespace scene {

    class Scene {
      private:
        entt::registry registry;
        std::map<entt::entity, std::string> directory;
        friend class Renderer;
        
      protected:
        ResourceManager resource_manager;
        std::map<GLuint, UBO> UBOs;  // indexed by uniform buffer's binding point
        std::map<GLuint, FBO> FBOs;  // indexed by the order of creation

        void AddUBO(GLuint shader_id);
        void AddFBO(GLuint width, GLuint height);

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
        ImTextureID welcome_screen_texture_id;
        asset_tmp<Texture> welcome_screen;
    };

    using glm::vec2, glm::vec3, glm::vec4;
    using glm::mat2, glm::mat3, glm::mat4, glm::quat;
    using glm::ivec2, glm::ivec3, glm::ivec4;
    using glm::uvec2, glm::uvec3, glm::uvec4;
    using uint = unsigned int;

    namespace world {
        // world space constants (OpenGL adopts a right-handed coordinate system)
        inline constexpr vec3 origin   { 0.0f };
        inline constexpr vec3 zero     { 0.0f };
        inline constexpr vec3 unit     { 1.0f };
        inline constexpr mat4 identity { 1.0f };
        inline constexpr quat eye      { 1.0f, 0.0f, 0.0f, 0.0f };
        inline constexpr vec3 up       { 0.0f, 1.0f, 0.0f };
        inline constexpr vec3 down     { 0.0f,-1.0f, 0.0f };
        inline constexpr vec3 forward  { 0.0f, 0.0f,-1.0f };
        inline constexpr vec3 backward { 0.0f, 0.0f, 1.0f };
        inline constexpr vec3 left     {-1.0f, 0.0f, 0.0f };
        inline constexpr vec3 right    { 1.0f, 0.0f, 0.0f };
    }

    namespace color {
        // some commonly used color presets
        inline constexpr vec3 white  { 1.0f };
        inline constexpr vec3 black  { 0.0f };
        inline constexpr vec3 red    { 1.0f, 0.0f, 0.0f };
        inline constexpr vec3 green  { 0.0f, 1.0f, 0.0f };
        inline constexpr vec3 lime   { 0.5f, 1.0f, 0.0f };
        inline constexpr vec3 blue   { 0.0f, 0.0f, 1.0f };
        inline constexpr vec3 cyan   { 0.0f, 1.0f, 1.0f };
        inline constexpr vec3 yellow { 1.0f, 1.0f, 0.0f };
        inline constexpr vec3 orange { 1.0f, 0.5f, 0.0f };
        inline constexpr vec3 purple { 0.5f, 0.0f, 1.0f };
    }

}
