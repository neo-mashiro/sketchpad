#pragma once

#include <string>
#include "scene/entity.h"

namespace scene {

    class Scene {
      private:
        entt::registry registry;

      protected:
        Entity CreateEntity(const std::string& name);
        void DestroyEntity(Entity entity);

      public:
        std::string title;

        explicit Scene(const std::string& title) : title(title) {}
        virtual ~Scene() { registry.clear(); }

        // these virtual functions should be overridden by each derived class
        // here in the base class, they are used to render the welcome screen
        virtual void Init(void);
        virtual void OnSceneRender(void);
        virtual void OnImGuiRender(void);
    };

}
