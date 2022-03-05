#pragma once

#include <string>
#include <ECS/entt.hpp>

namespace scene {

    class Entity {
      private:
        entt::registry* registry = nullptr;

      public:
        entt::entity id = entt::null;
        std::string name;

      public:
        Entity() = default;
        Entity(const std::string& name, entt::entity id, entt::registry* reg) : name(name), id(id), registry(reg) {}
        ~Entity() {};

        Entity(const Entity&) = default;
        Entity& operator=(const Entity&) = default;

        explicit operator bool() const { return id != entt::null; }

        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T, typename... Args>
        T& SetComponent(Args&&... args);

        template<typename T>
        void RemoveComponent();
    };

}

#include "scene/entity.tpp"