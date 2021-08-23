#pragma once

#include <string>
#include <type_traits>
#include "ECS/entt.hpp"

#include "core/log.h"
#include "components/all.h"

namespace scene {

    class Entity {
      private:
        entt::entity id = entt::null;
        entt::registry* registry = nullptr;

      public:
        std::string name;
        friend class Scene;  // entity internals should be exposed only to the scene

      public:
        Entity() = default;
        Entity(const std::string& name, entt::entity id, entt::registry* registry);
        ~Entity() {};

        Entity(const Entity&) = default;
        Entity& operator=(const Entity&) = default;

        operator bool() const { return id != entt::null; }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            using namespace components;
            CORE_ASERT(!registry->all_of<T>(id), "Component {0} already exists in {1}!", type_name<T>(), name);

            // be cautious that both branches of the if-else statement must be compilable
            // unless we use the `if constexpr` statement for compile time branching, when
            // if constexpr evaluates to true, the else branch is truncated and vice versa

            // the camera component should be tied to the entity's transform component
            if constexpr (std::is_same_v<T, Camera>) {
                auto& this_transform = registry->get<Transform>(id);
                return registry->emplace<T>(id, &this_transform, std::forward<Args>(args)...);
            }
            else {
                return registry->emplace<T>(id, std::forward<Args>(args)...);
            }
        }

        template<typename T>
        T& GetComponent() {
            CORE_ASERT(registry->all_of<T>(id), "Component {0} not found in {1}!", type_name<T>(), name);
            return registry->get<T>(id);
        }

        template<typename T, typename... Args>
        T& ReplaceComponent(Args&&... args) {
            return registry->emplace_or_replace<T>(id, std::forward<Args>(args)...);
        }

        template<typename T>
        void RemoveComponent() {
            registry->remove<T>(id);
        }
    };

}
