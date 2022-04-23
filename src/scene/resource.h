/* 
   in practice, a resource manager or asset manager is responsible for handling all the
   resources needed for rendering or gameplay. This is an essential role for most large
   applications, whose primary goal here is to isolate the management of resources from
   other modules so as to not pollute their code. It is supposed to manage the lifetime
   of various resources and make sure they will be available on demand. Ideally, such a
   manager class should load resources fast (multithreading), reliably (independent of
   others) and efficiently (dynamic data streaming, data packing, compression, etc.) so
   that there's a balanced tradeoff between memory usage and CPU workload.

   for rendering engines specifically, even though there isn't a concept of "levels" as
   in most game engines, it's often very useful if assets can be carried over from scene
   to scene. For example, skybox and large meshes can be made persistent across multiple
   scenes, by sharing these assets, loading time of subsequent scenes or models could be
   drastically reduced. Instead of having the scene takes full ownership of its entities
   and resources being used, a manager can steal some of these ownership so that not all
   resources are destroyed once the scene is unloaded. Managers are often created at the
   application level in a global scope, but can also be grouped by types, such as audio
   managers, level managers, model managers, etc, ultimately it's up to the design.

   # simple role

   despite the merits of a powerful manager, of course implementation does have a price.
   for apps with a small scope, it's also less beneficial so perhaps not worth the cost.
   for our demo, the main focus is on static rendering where we only care about in-scene
   framerates and the number of draw calls, thus to make it easy, our scene and renderer
   are not harnessing the power of parallelism by running on multiple threads, and most
   resources will simply be loaded upfront, so there's really nothing to play with here.

   for now, this class solely serves as a container to manage the lifetime of resources
   per scene level, each scene will have a `resource_manager` member whose scope is tied
   to the scene. Later on, we will upgrade it to a singleton that controls all scenes.

   # terminology

   unless otherwise stated, "asset" is defined as any object that holds OpenGL states or
   data to be consumed by the renderer, this includes buffer objects, textures, shaders
   and vertex arrays, as well as container components that are not registered in the ECS
   pool (e.g. a base material component that holds strong refs to a shader and multiple
   textures). If A is a valid container asset, all of its child assets must be valid as
   well. As a distinguishing feature, assets are always tied to the current valid OpenGL
   context, without such an active context, they cannot be alive. (dangling assets)

   by constrast, "resource" is a much broader term that refers to any object related to
   rendering, it could be an asset as we defined above, or a persistent entity like the
   skybox, or even intermediate in-memory data that has not yet been converted to living
   assets in the OpenGL context (e.g. loaded audio clips or raw image pixels). Note that
   resources in their raw data format have certain advantages over assets: since they're
   not tied to the OpenGL context, they have longer lifespan, and since data hasn't been
   touched yet, they are highly adaptable, thus also have higher chances of being reused

   despite the definition of "resource" being fairly broad, it is not supposed to handle
   everything falling under this term. While it is common to manage textures, materials,
   shaders, meshes and raw image data, it doesn't make sense to handle UBOs or FBOs in a
   manager, which are conceptually part of a scene itself.

   # RAII

   all resources handled by the manager must be RAII-compliant, which means they must be
   encapsulated in properly scoped objects that manage their life cycles, such as smart
   pointers, locks and STL containers (std::map, std::shared_ptr, std::scoped_lock...).
   the RAII principle is already observed by our asset classes (rule of 5 in base + rule
   of 0 in derived) so it's likely not an issue, but for other memory data we still need
   to take care, as many of them require a custom deleter.

   for convenience, we are using `shared_ptr<void>` to allow any type of resources, thus
   there's no need to write a template wrapper class just for the resources. Yet another
   option is to use the safer `std::any`, sadly though it doesn't allow move-only types.
   note that the `Get` function must return shared pointers by value, so that every call
   will return a copy of the pointer to the user, this is to protect the original sample
   resource from being invalidated inadvertently, however, maintaining the coherency and
   integrity of data is completely up to the user.
*/

#pragma once

#include <map>
#include <typeinfo>
#include <typeindex>
#include "core/base.h"

namespace scene {

    class ResourceManager {
      private:
        std::map<int, std::type_index> registry;
        std::map<int, asset_ref<void>> resources;
        
      public:
        ResourceManager() {}
        ~ResourceManager() {}

        template<typename T>
        void Add(int key, const asset_ref<T>& resource);

        template<typename T>
        asset_ref<T> Get(int key) const;

        void Del(int key);
        void Clear();
    };

}

#include "scene/resource.tpp"