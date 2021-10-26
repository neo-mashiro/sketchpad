#include "pch.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "buffer/fbo.h"
#include "buffer/ubo.h"
#include "components/all.h"
#include "scene/entity.h"
#include "scene/factory.h"
#include "scene/renderer.h"
#include "scene/scene.h"
#include "scene/ui.h"
#include "utils/path.h"

using namespace core;
using namespace buffer;
using namespace components;

namespace scene {

    Scene* Renderer::last_scene = nullptr;
    Scene* Renderer::curr_scene = nullptr;

    std::queue<entt::entity> Renderer::render_queue {};

    static bool depth_prepass = false;

    void Renderer::MSAA(bool on) {
        // the built-in MSAA only works on the default framebuffer (without multi-pass)
        static GLint buffers, samples;
        if (on) {
            glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
            glGetIntegerv(GL_SAMPLES, &samples);
            CORE_ASERT(buffers > 0, "MSAA buffers are not available! Check your window context...");
            CORE_ASERT(samples == 4, "Invalid MSAA buffer size! Expected 4 samples per pixel...");
            glEnable(GL_MULTISAMPLE);
        }
        else {
            glDisable(GL_MULTISAMPLE);
        }
    }

    void Renderer::DepthPrepass(bool on) {
        depth_prepass = on;
    }

    void Renderer::DepthTest(bool on) {
        if (on) {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void Renderer::StencilTest(bool on) {
        // todo
        if (on) {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_EQUAL, 1, 0xFF);  // discard fragments whose stencil values != 1
        }
        else {
            glDisable(GL_STENCIL_TEST);
        }
    }

    void Renderer::FaceCulling(bool on) {
        if (on) {
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CCW);
            glCullFace(GL_BACK);
        }
        else {
            glDisable(GL_CULL_FACE);
        }
    }

    void Renderer::SeamlessCubemap(bool on) {
        if (on) {
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
        else {
            glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
    }

    void Renderer::SetFrontFace(bool ccw) {
        glFrontFace(ccw ? GL_CCW : GL_CW);
    }

    void Renderer::SetViewport(GLuint width, GLuint height) {
        glViewport(0, 0, width, height);
    }

    void Renderer::Attach(const std::string& title) {
        CORE_TRACE("Attaching scene \"{0}\" ......", title);

        Input::ResetCursor();
        Input::ShowCursor();
        Window::Rename(title);
        Window::layer = Layer::ImGui;

        Scene* new_scene = factory::LoadScene(title);
        new_scene->Init();
        curr_scene = new_scene;
    }

    void Renderer::Detach() {
        CORE_TRACE("Detaching scene \"{0}\" ......", curr_scene->title);

        last_scene = curr_scene;
        curr_scene = nullptr;

        delete last_scene;  // each object in the scene will be destructed
        last_scene = nullptr;
    }

    void Renderer::Clear(const glm::vec3& color, float depth, int stencil) {
        // for the default framebuffer, do not use black as the clear color, because we want
        // to clearly see what pixels are background, but black makes it hard to debug many
        // buffer textures. However, custom framebuffers should always use a black clear color
        // so that the render buffer is clean, we don't want any dirty values other than 0
        glClearColor(color.r, color.g, color.b, 1.0f);
        glClearDepth(depth);
        glClearStencil(stencil);  // 8-bit integer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void Renderer::Flush() {
        glutSwapBuffers();
        glutPostRedisplay();
    }

    void Renderer::Render() {
        auto& reg = curr_scene->registry;
        auto mesh_group = reg.group<Mesh>(entt::get<Transform, Tag, Material>);
        auto model_group = reg.group<Model>(entt::get<Transform, Tag, Material>);

        while (!render_queue.empty()) {
            auto& e = render_queue.front();

            // skip entities marked as null (a convenient mask to tell if an entity should be drawn)
            if (e == entt::null) {
                render_queue.pop();
                continue;
            }

            // entity is a native mesh
            if (mesh_group.contains(e)) {
                auto& transform = mesh_group.get<Transform>(e);
                auto& mesh      = mesh_group.get<Mesh>(e);
                auto& material  = mesh_group.get<Material>(e);
                auto tag        = mesh_group.get<Tag>(e).tag;

                material.SetUniform(0, depth_prepass);
                material.SetUniform(1, transform.transform);
                material.SetUniform(2, 0);  // primitive mesh does not need a material id

                if (material.Bind()) {
                    if (tag == ETag::Skybox) {
                        SetFrontFace(0);  // skybox has reversed winding order, we only draw the inner faces
                        mesh.Draw();
                        SetFrontFace(1);  // recover the global winding order
                        material.Unbind();
                    }
                    else {
                        mesh.Draw();
                        material.Unbind();
                    }
                }
            }

            // entity is an imported model
            else if (model_group.contains(e)) {
                auto& transform = model_group.get<Transform>(e);
                auto& model     = model_group.get<Model>(e);
                auto& material  = model_group.get<Material>(e);  // one material is reused for every mesh

                material.SetUniform(0, depth_prepass);
                material.SetUniform(1, transform.transform);

                for (auto& mesh : model.meshes) {
                    GLuint material_id = mesh.material_id;
                    auto& textures = model.textures[material_id];
                    auto& properties = model.properties[material_id];

                    // update material id for the current mesh
                    material.SetUniform(2, static_cast<int>(material_id));

                    // update textures for the current mesh if textures are available
                    if (size_t n = textures.size(); n > 0) {
                        for (size_t i = 0; i < n; i++) {
                            material.SetTexture(i, textures[i]);  // use array index as texture unit
                        }
                    }
                    // update properties only if no textures are available
                    else {
                        for (size_t i = 0; i < properties.size(); i++) {
                            auto visitor = [&i, &material](const auto& prop) {
                                // for model-native properties, by default they use uniform locations
                                // from 100 and upwards, so as not to conflict with other user-defined
                                // or internally reserved uniforms. This is safe as modern GPUs have
                                // at least 1024 active uniforms available in each shader stage
                                material.SetUniform(i + 100, prop);
                            };
                            std::visit(visitor, properties[i]);
                        }
                    }

                    // commit and push the updates to the shader (notice that the shader is really
                    // bound only once for the first mesh, there's no context switching afterwards
                    // because the material and shader is reused for every mesh, so this is cheap)
                    material.Bind();
                    mesh.Draw();
                }

                material.Unbind();  // we only need to unbind once since it's shared by all meshes
            }

            // a non-null entity must have either a mesh or a model component to be considered renderable
            else {
                CORE_ERROR("Entity {0} in the render list is non-renderable!", e);
                Clear(color::black);  // in this case just show a black screen (UI stuff is separate)
            }

            render_queue.pop();
        }
    }

    void Renderer::DrawScene() {
        curr_scene->OnSceneRender();
    }

    void Renderer::DrawImGui() {
        bool switch_scene = false;
        std::string next_scene_title;

        ui::NewFrame();
        {
            ui::DrawMenuBar(curr_scene->title, next_scene_title);
            ui::DrawStatusBar();

            if (!next_scene_title.empty()) {
                switch_scene = true;
                Clear(color::black);
                ui::DrawLoadingScreen();
            }
            else {
                if (Window::layer == Layer::ImGui) {
                    curr_scene->OnImGuiRender();
                }
                else {
                    ui::DrawCrosshair();
                }
            }
        }
        ui::EndFrame();

        /* [side note] difficulties on using multiple threads in OpenGL.
           
           [1] at any given time, there should be only one active scene, no two scenes
               can be alive at the same time in the OpenGL context, so, each time we
               switch scenes, we must delete the previous one first to safely clean up
               global OpenGL states, before creating the new one from our factory.
           
           [2] the new scene must be fully loaded and initialized before being assigned
               to `curr_scene`, otherwise, `curr_scene` could be pointing to a scene
               that has dirty states, so a subsequent draw call could possibly throw an
               access violation exception that will crash the program.
           
           [3] cleaning and loading scenes can take quite a while, especially when there
               are complicated meshes. Ideally, this function should be scheduled as an
               asynchronous background task that runs in another thread, so as not to
               block and freeze the window. To do so, we can wrap the task in std::async,
               and then query the result from the std::future object, C++ will handle
               concurrency for us, just make sure that the lifespan of the future extend
               beyond the calling function.
           
           [4] sadly though, multi-threading in OpenGL is a pain, you can't multithread
               OpenGL calls easily without using complex context switching, because lots
               of buffer data cannot be shared between threads, also freeglut and your
               graphics card driver may not be supporting it anyway. Sharing context and
               resources between threads is not worth the effort, if at all possible.
               What we sure can do is to load images and compute textures concurrently,
               but for the sake of multi-threading, D3D11 would be a better option.
        */

        Flush();

        if (switch_scene) {
            Detach();                  // blocking call
            Attach(next_scene_title);  // blocking call (could take 30 minutes if scene is huge)
        }
    }

}
