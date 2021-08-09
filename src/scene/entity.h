#pragma once

#include <string>
#include "ecs/entt.hpp"
#include "core/log.h"

namespace scene {

    class Entity {
      private:
        entt::entity id = entt::null;
        entt::registry* registry = nullptr;

        friend class Scene;  // entity internals should be exposed only to the scene

      public:
        std::string name;

      public:
        Entity() = default;
        Entity(const std::string& name, entt::entity id, entt::registry* registry);
        ~Entity() {};

        Entity(const Entity&) = default;
        Entity& operator=(const Entity&) = default;

        operator bool() const { return id != entt::null; }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            CORE_ASERT(!registry->all_of<T>(id), "Component {0} already exists in {1}!", type_name<T>(), name);
            return registry->emplace<T>(id, std::forward<Args>(args)...);
        }

        template<typename T>
        T& GetComponent(void) {
            CORE_ASERT(registry->all_of<T>(id), "Component {0} not found in {1}!", type_name<T>(), name);
            return registry->get<T>(id);
        }

        template<typename T, typename... Args>
        T& ReplaceComponent(Args&&... args) {
            return registry->emplace_or_replace<T>(id, std::forward<Args>(args)...);
        }

        template<typename T>
        void RemoveComponent(void) {
            registry->remove<T>(id);
        }

    };

}
