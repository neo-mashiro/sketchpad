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
    */

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

}
