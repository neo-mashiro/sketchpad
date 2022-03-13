#pragma once

#include <string>
#include <queue>
#include <ECS/entt.hpp>

namespace scene {

    class Scene;  // forward declaration

    /* the static renderer class provides a set of functions to change the global rendering
       settings, as well as some core event functions used by the application instance. It
       keeps track of the list of entities to draw in a queue. Every frame, the user should
       submit entities to the renderer, and make sure they are in the correct order. Note
       that the renderer is only in charge of operations that are directly related to draw
       calls, it doesn't care which framebuffer is currently bound (controlled by the user).

       # entity-component-system

       the draw calls are issued using the entity-component system, a popular design pattern
       that is widely adopted by many commercial game engines. For every submitted entity in
       the render queue, the renderer will decide how to bind its shaders and textures, how
       to upload the uniforms, etc, all based on the entity's attached components. This is
       done very efficiently using views and groups from the "entt" library.

       in entt, a group is a rearranged owning subset of the registry, it is intended to be
       setup once and used repeatedly, where future access can be made blazingly fast. In
       contrast, views are slower as they do not take ownership, and can be considered more
       temporary than groups. The `Render()` function uses partial-owning groups to filter
       out entities in the render queue, it's primarily in charge of drawing imported models
       and native (primitive) meshes, and takes ownership of both components. The material
       component, on the other hand, is shared by both meshes and models, so it can only be
       partially owned by groups in order to avoid potential conflicts.

       in entt, ownership implies that a group is free to rearrange components in the pool
       as it sees fit, that means, we cannot rely on a Mesh* or Model* pointer in our code,
       because its memory address will be volatile even if our own code doesn't touch it.

       # tips on submission of entities to the renderer

       when you submit a list of entities to the renderer, they are internally stored in a
       queue and will be drawn in the order of submission, this order can not only affect
       alpha blending but can also make a huge difference in performance.

       it is suggested to submit the skybox last to the renderer because that is the farthest
       object in the scene, this can save us quite a lot fragment invocations as the pixels
       that already fail the depth test will be instantly discarded. Moreover, entities that
       share the same shader or textures are suggested to be packed as much as possible, this
       can help reduce the number of context switching, which is quite expensive. Our shader,
       texture and most buffer classes have been optimized by using smart bindings, therefore
       you should make the most out of this feature to avoid unnecessary binds or unbinds, if
       you cannot use batch rendering.

       recall that alpha blending is often order-dependent (except OIT) so is a special case,
       when blending is involved, transparent entities must come last in the render queue.

       # switching scenes and multithreading

       this class is also responsible for loading and unloading scenes while the application
       is up and running, which is done by the two blocking calls `Attach()` and `Detach()`.
       at any given time, there can only be one active scene in the OpenGL context, so every
       time we switch scenes, `Detach()` will first delete the previous one to safely clean
       up global OpenGL states, and `Attach()` will then load the new scene from our factory.

       note that cleaning and loading scenes can take quite a while, especially when there is
       a large number entities in the scene. Ideally, loading and unloading scenes should be
       scheduled as asynchronous background tasks, so as not to block and freeze the window,
       sadly though, multithreading in OpenGL is a pain, there's no way to multithread OpenGL
       calls easily without using complex context switching, and some assets cannot be shared
       between threads, there can be only one context, so overall it's not worth the effort.

       in this regard, Vulkan and D3D12 are a better option, we will leave this support for a
       future project. That said, the resource manager can be used to optimize the loading of
       resources in a concurrent way (TODO: integrate the "taskflow" library), this should be
       more than enough for us to achieve reasonably good performance.
    */

    class Renderer {
      private:
        static Scene* last_scene;
        static Scene* curr_scene;
        static std::queue<entt::entity> render_queue;

      public:
        static const Scene* GetScene();

        // configuration functions
        static void MSAA(bool enable);
        static void DepthPrepass(bool enable);
        static void DepthTest(bool enable);
        static void StencilTest(bool enable);
        static void AlphaBlend(bool enable);
        static void FaceCulling(bool enable);
        static void SeamlessCubemap(bool enable);
        static void PrimitiveRestart(bool enable);
        static void SetFrontFace(bool ccw);
        static void SetViewport(GLuint width, GLuint height);

        // core event functions
        static void Attach(const std::string& title);
        static void Detach();

        static void Reset();
        static void Clear();
        static void Flush();
        static void Render();

        static void DrawScene();
        static void DrawImGui();

        // submit a variable number of entity ids to the render queue (to be processed next frame)
        template<typename... Args>
        static void Submit(Args&&... args) {
            (render_queue.push(args), ...);
        }
    };

}
