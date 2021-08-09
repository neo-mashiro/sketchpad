#include "pch.h"

#include "scene/entity.h"

namespace scene {

    Entity::Entity(const std::string& name, entt::entity id, entt::registry* registry)
        : name(name), id(id), registry(registry) {}

}
