#pragma once

#include <map>
#include <string>
#include "components/tag.h"
#include "scene/entity.h"

namespace scene {

    // this struct maps to the config uniform block in GLSL (additional control for shading)
    struct Config {
        bool enabled { false };  // this field will be ruled out when sending data to the shader
        glm::vec3 v1; int i1;    // we intentionally pad int after vec3 to make up a 16-bytes alignment
        glm::vec3 v2; int i2;    // so that the struct's memory layout agrees with the std140 GLSL block
        glm::vec3 v3; int i3;
        int i4, i5, i6; float f1, f2, f3;
    };

    class Scene {
      private:
        friend class Renderer;

        entt::registry registry;
        std::map<entt::entity, std::string> directory;
        
      protected:
        Config config {};    // reserved for user-defined flags (control GLSL code branching)

        Entity CreateEntity(const std::string& name, components::ETag tag = components::ETag::Untagged);
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
        constexpr glm::vec3 blue { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 cyan { 0.0f, 1.0f, 1.0f };
        constexpr glm::vec3 yellow { 1.0f, 1.0f, 0.0f };
        constexpr glm::vec3 purple { 0.5f, 0.0f, 1.0f };
    }

}
