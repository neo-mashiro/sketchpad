#include "pch.h"

#include "scene/entity.h"

using namespace components;

namespace scene {

    Entity::Entity(const std::string& name, entt::entity id, entt::registry* registry)
        : name(name), id(id), registry(registry)
    {
        // add a default (empty) material component whenever a mesh component is added
        registry->on_construct<Mesh>().connect<&entt::registry::emplace_or_replace<Material>>();
    }

}
