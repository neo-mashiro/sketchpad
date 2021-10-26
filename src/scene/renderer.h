#pragma once

#include <string>
#include <queue>
#include <ECS/entt.hpp>
#include "buffer/fbo.h"

namespace scene {

    class Scene;  // forward declaration

    /* the static renderer class provides a set of functions to change the global rendering
       settings, as well as some core event functions used by the application instance. It
       keeps track of the list of entities to draw in a queue. Every frame, the user should
       submit entities to the renderer, and make sure they are in the correct order. Note
       that the renderer is only in charge of operations that are directly related to draw
       calls, it doesn't care which framebuffer is currently bound (controlled by the user).

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
    */

    class Renderer {
      private:
        static Scene* last_scene;
        static Scene* curr_scene;
        static std::queue<entt::entity> render_queue;

      public:
        // configuration functions
        static void MSAA(bool on);
        static void DepthPrepass(bool on);
        static void DepthTest(bool on);
        static void StencilTest(bool on);
        static void FaceCulling(bool on);
        static void SeamlessCubemap(bool on);
        static void SetFrontFace(bool ccw);
        static void SetViewport(GLuint width, GLuint height);

        // core event functions
        static void Attach(const std::string& title);
        static void Detach();

        static void Clear(const glm::vec3& color, float depth = 1.0f, int stencil = 0);
        static void Flush();
        static void Render();

        static void DrawScene();
        static void DrawImGui();

        // submit a variable number of entities to the render queue (to be processed next frame)
        template<typename... Args>
        static void Submit(Args&&... args) {
            (render_queue.push(args), ...);
        }
    };

}
