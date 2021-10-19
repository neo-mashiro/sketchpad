#pragma once

#include <bitset>
#include <string>
#include <type_traits>
#include <ECS/entt.hpp>

#include "core/log.h"
#include "components/all.h"

using namespace components;

namespace scene {

    class Entity {
      private:
        entt::registry* registry = nullptr;

      public:
        entt::entity id = entt::null;
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
            if constexpr (std::is_same_v<T, Camera>) {
                // the camera component is tied to the camera's transform component
                auto& this_transform = registry->get<Transform>(id);
                return registry->emplace<T>(id, &this_transform, std::forward<Args>(args)...);
            }
            else {
                return registry->emplace<T>(id, std::forward<Args>(args)...);
            }
        }

        template<typename T>
        [[nodiscard]] T& GetComponent() {
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
