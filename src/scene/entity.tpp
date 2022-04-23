#include <type_traits>
#include "core/base.h"
#include "core/log.h"
#include "component/all.h"
#include "utils/ext.h"

namespace scene {

    template<typename T, typename... Args>
    inline T& Entity::AddComponent(Args&&... args) {
        CORE_ASERT(!registry->all_of<T>(id), "{0} already has component {1}!", name, utils::type_name<T>());
        using namespace component;

        if constexpr (std::is_same_v<T, Camera>) {
            auto& transform = registry->get<Transform>(id);
            return registry->emplace<T>(id, &transform, std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<T, Animator>) {
            auto& model = registry->get<Model>(id);
            return registry->emplace<T>(id, &model, std::forward<Args>(args)...);
        }
        else {
            return registry->emplace<T>(id, std::forward<Args>(args)...);
        }
    }

    template<typename T>
    inline T& Entity::GetComponent() {
        CORE_ASERT(registry->all_of<T>(id), "Component {0} not found in {1}!", utils::type_name<T>(), name);
        return registry->get<T>(id);
    }

    template<typename T, typename... Args>
    inline T& Entity::SetComponent(Args&&... args) {
        return registry->emplace_or_replace<T>(id, std::forward<Args>(args)...);
    }

    template<typename T>
    inline void Entity::RemoveComponent() {
        registry->remove<T>(id);
    }

}
