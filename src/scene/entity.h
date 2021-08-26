#pragma once

#include <bitset>
#include <string>
#include <type_traits>

#include "ECS/entt.hpp"
#include "core/log.h"
#include "components/all.h"

namespace scene {

    // context of the scene, primarily used by the renderer
    struct Context {
        std::bitset<8> lightmask { 0 };

        Context() = default;
        Context(Context&& other) = default;
        Context& operator=(Context&& other) = default;
    };

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
            using namespace components;

            CORE_ASERT(!registry->all_of<T>(id), "Component {0} already exists in {1}!", type_name<T>(), name);
            auto& context = registry->ctx<Context>();

            if constexpr (std::is_same_v<T, Camera>) {
                // the camera component should be tied to the entity's transform component
                auto& this_transform = registry->get<Transform>(id);
                return registry->emplace<T>(id, &this_transform, std::forward<Args>(args)...);
            }
            else if constexpr (std::is_same_v<T, DirectionLight>) {
                context.lightmask.set(0);
                return registry->emplace<T>(id, std::forward<Args>(args)...);
            }
            else if constexpr (std::is_same_v<T, PointLight>) {
                context.lightmask.set(1);
                return registry->emplace<T>(id, std::forward<Args>(args)...);
            }
            else if constexpr (std::is_same_v<T, Spotlight>) {
                context.lightmask.set(2);
                return registry->emplace<T>(id, std::forward<Args>(args)...);
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
