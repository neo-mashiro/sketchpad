#include "core/log.h"

namespace scene {

    template<typename T>
    inline void ResourceManager::Add(int key, const asset_ref<T>& resource) {
        // ignore keys that are already in the registry
        if (registry.find(key) != registry.end()) {
            CORE_ERROR("Duplicate key already exists, cannot add the resource...");
            return;
        }

        registry.try_emplace(key, typeid(T));  // construct type_index in place into the registry
        resources.insert_or_assign(key, std::static_pointer_cast<void>(resource));
    }

    template<typename T>
    inline asset_ref<T> ResourceManager::Get(int key) const {
        // check if the key is in the registry
        if (registry.find(key) == registry.end()) {
            CORE_ERROR("Invalid resource key!");
            return nullptr;
        }
        // check if `T` matches the registered type index
        else if (registry.at(key) != std::type_index(typeid(T))) {  // update to spaceship <=> in C++20
            CORE_ERROR("Mismatched resource type!");
            return nullptr;
        }

        return std::static_pointer_cast<T>(resources.at(key));
    }

    inline void ResourceManager::Del(int key) {
        // invalid keys are silently ignored
        if (registry.find(key) != registry.end()) {
            registry.erase(key);
            resources.erase(key);
        }
    }

    inline void ResourceManager::Clear() {
        registry.clear();
        resources.clear();
    }

}