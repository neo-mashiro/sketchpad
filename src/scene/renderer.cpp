#include "pch.h"

#include <GL/freeglut.h>
#include <GLFW/glfw3.h>

#include "core/base.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/log.h"
#include "core/sync.h"
#include "core/window.h"
#include "asset/buffer.h"
#include "asset/fbo.h"
#include "asset/shader.h"
#include "component/all.h"
#include "scene/entity.h"
#include "scene/factory.h"
#include "scene/renderer.h"
#include "scene/scene.h"
#include "scene/ui.h"
#include "utils/ext.h"
#include "utils/path.h"

using namespace core;
using namespace asset;
using namespace component;

namespace scene {

    Scene* Renderer::last_scene = nullptr;
    Scene* Renderer::curr_scene = nullptr;
    std::queue<entt::entity> Renderer::render_queue {};

    static bool depth_prepass = false;
    static uint shadow_index = 0U;
    static asset_tmp<UBO> renderer_input = nullptr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    const Scene* Renderer::GetScene() {
        return curr_scene;
    }

    void Renderer::MSAA(bool enable) {
        // the built-in MSAA only works on the default framebuffer (without multi-pass)
        static GLint buffers = 0, samples = 0, max_samples = 0;
        if (samples == 0) {
            glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
            glGetIntegerv(GL_SAMPLES, &samples);
            glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
            CORE_ASERT(buffers > 0, "MSAA buffers are not available! Check your window context...");
            CORE_ASERT(samples == 4, "Invalid MSAA buffer size! 4 samples per pixel is not available...");
        }

        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_MULTISAMPLE);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_MULTISAMPLE);
            is_enabled = false;
        }
    }

    void Renderer::DepthPrepass(bool enable) {
        depth_prepass = enable;
    }

    void Renderer::DepthTest(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_DEPTH_TEST);
            is_enabled = false;
        }
    }

    void Renderer::StencilTest(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_EQUAL, 1, 0xFF);  // discard fragments whose stencil values != 1
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_STENCIL_TEST);
            is_enabled = false;
        }
    }

    void Renderer::AlphaBlend(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
            glBlendEquation(GL_FUNC_ADD);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_BLEND);
            is_enabled = false;
        }
    }

    void Renderer::FaceCulling(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CCW);
            glCullFace(GL_BACK);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_CULL_FACE);
            is_enabled = false;
        }
    }

    void Renderer::SeamlessCubemap(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
            is_enabled = false;
        }
    }

    void Renderer::PrimitiveRestart(bool enable) {
        static bool is_enabled = false;

        if (enable && !is_enabled) {
            glEnable(GL_PRIMITIVE_RESTART);
            glPrimitiveRestartIndex(0xFFFFFF);
            is_enabled = true;
        }
        else if (!enable && is_enabled) {
            glDisable(GL_PRIMITIVE_RESTART);
            is_enabled = false;
        }
    }

    void Renderer::SetFrontFace(bool ccw) {
        glFrontFace(ccw ? GL_CCW : GL_CW);
    }

    void Renderer::SetViewport(GLuint width, GLuint height) {
        glViewport(0, 0, width, height);
    }

    void Renderer::SetShadowPass(unsigned int index) {
        // to cast shadows from multiple lights, we need multiple passes, once per light source
        shadow_index = index;  // use this to identify a specific shadow pass and light source
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Renderer::Attach(const std::string& title) {
        CORE_TRACE("Attaching scene \"{0}\" ......", title);

        // create the renderer input UBO on the first run (internal UBO)
        if (renderer_input == nullptr) {
            const std::vector<GLuint> offset { 0U, 8U, 16U, 20U, 24U, 28U, 32U, 36U };
            const std::vector<GLuint> length { 8U, 8U, 4U, 4U, 4U, 4U, 4U, 4U };
            const std::vector<GLuint> stride { 8U, 8U, 4U, 4U, 4U, 4U, 4U, 4U };

            renderer_input = WrapAsset<UBO>(10, offset, length, stride);
        }

        Input::Clear();
        Input::ShowCursor();
        Window::Rename(title);
        Window::layer = Layer::ImGui;

        // the new scene must be fully loaded and initialized before this function returns, o/w
        // `curr_scene` could be pointing to a scene that has dirty states and subsequent calls
        // could possibly throw an access violation that will crash the application. if `Init()`
        // involves precomputation that requires heavy rendering commands, we need to block the
        // main thread until all tasks in the GPU's command queue are fully executed.

        Scene* new_scene = factory::LoadScene(title);
        new_scene->Init();  // asynchronous call
        curr_scene = new_scene;

        Sync::WaitFinish();  // block until the CPU and GPU are in sync
    }

    void Renderer::Detach() {
        CORE_TRACE("Detaching scene \"{0}\" ......", curr_scene->title);

        last_scene = curr_scene;
        curr_scene = nullptr;

        delete last_scene;  // every object in the scene will be destructed
        last_scene = nullptr;

        Sync::WaitFinish();  // block until the scene is fully unloaded
        Renderer::Reset();   // reset renderer to a clean default state
    }

    void Renderer::Reset() {
        // reset the rasterizer or raytracer to the default factory state
        MSAA(0);
        DepthPrepass(0);
        DepthTest(0);
        StencilTest(0);
        AlphaBlend(0);
        FaceCulling(0);
        SeamlessCubemap(0);
        PrimitiveRestart(0);
        SetFrontFace(1);
        SetViewport(Window::width, Window::height);
        SetShadowPass(0);
    }

    void Renderer::Clear() {
        // for the default framebuffer, do not use black as the clear color, because we want
        // to clearly see what pixels are background, but black makes it hard to debug many
        // buffer textures. Deep blue is a nice color, think of it as Microsoft blue screen.
        // note: custom framebuffers should always use a black clear color to make sure the
        // render buffer is clean, we don't want any dirty values other than 0. However, you
        // should call the `Clear()` method on a framebuffer instead, this function is only
        // intended for clearing the default framebuffer.

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClearDepth(1.0f);
        glClearStencil(0);  // 8-bit integer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void Renderer::Flush() {
        if constexpr (_freeglut) {
            glutSwapBuffers();
            glutPostRedisplay();
        }
        else {
            glfwSwapBuffers(Window::window_ptr);
            glfwPollEvents();
        }
    }

    void Renderer::Render(const asset_ref<asset::Shader> custom_shader) {
        auto& reg = curr_scene->registry;
        auto mesh_group = reg.group<Mesh>(entt::get<Transform, Tag, Material>);
        auto model_group = reg.group<Model>(entt::get<Transform, Tag>);  // materials are managed by the model

        if (!render_queue.empty()) {
            constexpr float near_clip = 0.1f;
            constexpr float far_clip = 100.0f;

            glm::ivec2 resolution = glm::ivec2(Window::width, Window::height);
            glm::ivec2 cursor_pos = ui::GetCursorPosition();

            float total_time = Clock::time;
            float delta_time = Clock::delta_time;

            renderer_input->SetUniform(0U, utils::val_ptr(resolution));
            renderer_input->SetUniform(1U, utils::val_ptr(cursor_pos));
            renderer_input->SetUniform(2U, utils::val_ptr(near_clip));
            renderer_input->SetUniform(3U, utils::val_ptr(far_clip));
            renderer_input->SetUniform(4U, utils::val_ptr(total_time));
            renderer_input->SetUniform(5U, utils::val_ptr(delta_time));
            renderer_input->SetUniform(6U, utils::val_ptr(static_cast<int>(depth_prepass)));
            renderer_input->SetUniform(7U, utils::val_ptr(shadow_index));
        }

        while (!render_queue.empty()) {
            auto& e = render_queue.front();

            // skip null entities
            if (e == entt::null) {
                render_queue.pop();
                continue;
            }

            // entity is a native mesh
            if (mesh_group.contains(e)) {
                auto& transform = mesh_group.get<Transform>(e);
                auto& mesh      = mesh_group.get<Mesh>(e);
                auto& material  = mesh_group.get<Material>(e);
                auto& tag       = mesh_group.get<Tag>(e);

                if (custom_shader) {
                    custom_shader->SetUniform(1000U, transform.transform);
                    custom_shader->SetUniform(1001U, 0U);
                    custom_shader->Bind();
                }
                else {
                    material.SetUniform(1000U, transform.transform);
                    material.SetUniform(1001U, 0U);  // primitive mesh does not have a material id
                    material.SetUniform(1002U, 0U);  // ext_1002
                    material.SetUniform(1003U, 0U);  // ext_1003
                    material.SetUniform(1004U, 0U);  // ext_1004
                    material.SetUniform(1005U, 0U);  // ext_1005
                    material.SetUniform(1006U, 0U);  // ext_1006
                    material.SetUniform(1007U, 0U);  // ext_1007
                    material.Bind();  // smart binding, no need to unbind
                }

                if (tag.Contains(ETag::Skybox)) {
                    SetFrontFace(0);  // skybox has reversed winding order, we only draw the inner faces
                    mesh.Draw();
                    SetFrontFace(1);  // recover the global winding order
                }
                else {
                    mesh.Draw();
                }
            }

            // entity is an imported model
            else if (model_group.contains(e)) {
                auto& transform = model_group.get<Transform>(e);
                auto& model = model_group.get<Model>(e);

                for (auto& mesh : model.meshes) {
                    GLuint material_id = mesh.material_id;
                    auto& material = model.materials.at(material_id);

                    if (custom_shader) {
                        custom_shader->SetUniform(1000U, transform.transform);
                        custom_shader->SetUniform(1001U, material_id);
                        custom_shader->Bind();
                    }
                    else {
                        material.SetUniform(1000U, transform.transform);
                        material.SetUniform(1001U, material_id);
                        material.SetUniform(1002U, 0U);  // ext_1002
                        material.SetUniform(1003U, 0U);  // ext_1003
                        material.SetUniform(1004U, 0U);  // ext_1004
                        material.SetUniform(1005U, 0U);  // ext_1005
                        material.SetUniform(1006U, 0U);  // ext_1006
                        material.SetUniform(1007U, 0U);  // ext_1007
                        material.Bind();  // smart binding, no need to unbind
                    }

                    mesh.Draw();
                }
            }

            // a non-null entity must have either a mesh or a model component to be considered renderable
            else {
                CORE_ERROR("Entity {0} in the render list is non-renderable!", e);
                Clear();  // in this case just show a deep blue screen (UI stuff is separate)
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

        if (ui::NewFrame(); true) {
            ui::DrawMenuBar(next_scene_title);
            ui::DrawStatusBar();

            if (!next_scene_title.empty()) {
                switch_scene = true;
                Clear();
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

            ui::EndFrame();
        }

        Flush();

        if (switch_scene) {
            Detach();                  // blocking call
            Attach(next_scene_title);  // blocking call
        }
    }

}
