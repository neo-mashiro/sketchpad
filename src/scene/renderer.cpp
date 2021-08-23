#include "pch.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/input.h"
#include "core/log.h"
#include "core/window.h"
#include "components/all.h"
#include "scene/entity.h"
#include "scene/factory.h"
#include "scene/renderer.h"
#include "scene/scene.h"
#include "scene/ui.h"
#include "scene/fbo.h"
#include "scene/ubo.h"
#include "utils/path.h"

using namespace core;
using namespace components;

namespace scene {

    Scene* Renderer::last_scene = nullptr;
    Scene* Renderer::curr_scene = nullptr;

    template <typename... Args>
    auto val_ptr(Args&&... args) -> decltype(glm::value_ptr(std::forward<Args>(args)...)) {
        return glm::value_ptr(std::forward<Args>(args)...);
    }

    // uniform type's base alignment in bytes specified by std140 layout
    static const std::map<GLenum, GLuint> align140 {
        { GL_INT, 4 },
        { GL_BOOL, 4 },
        { GL_FLOAT, 4 },
        { GL_FLOAT_VEC2, 8 },
        { GL_FLOAT_VEC3, 16 },
        { GL_FLOAT_VEC4, 16 },
        { GL_FLOAT_MAT2, 16 },
        { GL_FLOAT_MAT3, 16 },
        { GL_FLOAT_MAT4, 16 }
    };

    // uniform type's size in bytes specified by std140 layout
    static const std::map<GLenum, GLuint> size140 {
        { GL_INT, 4 },
        { GL_BOOL, 4 },
        { GL_FLOAT, 4 },
        { GL_FLOAT_VEC2, 8 },
        { GL_FLOAT_VEC3, 12 },
        { GL_FLOAT_VEC4, 16 },
        { GL_FLOAT_MAT2, 16 },  // matrix is stored as an array of column vectors
        { GL_FLOAT_MAT3, 48 },  // element in an array is always padded to a multiple of the size of a vec4
        { GL_FLOAT_MAT4, 64 }
    };

    // -------------------------------------------------------------------------
    // configuration functions
    // -------------------------------------------------------------------------
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

    void Renderer::SetFrontFace(bool ccw) {
        glFrontFace(ccw ? GL_CCW : GL_CW);
    }

    // -------------------------------------------------------------------------
    // core event functions
    // -------------------------------------------------------------------------
    // where do I call this init function? modify app.cpp, also the Free() function
    void Renderer::Init() {
        // on initialization, we parse the sample shader to determine the structure of
        // uniform blocks and create global uniform buffers stored in the UBOs map.
        asset_ref<Shader> sample = LoadAsset<Shader>(SHADER + "sample");
        GLuint id = sample->id;

        GLint n_blocks = 0;
        glGetProgramInterfaceiv(id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &n_blocks);

        GLenum block_props1[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH, GL_BUFFER_BINDING };
        GLenum block_props2[] = { GL_ACTIVE_VARIABLES };

        // iterate over active uniform blocks (iterations are returned in the order of block indices,
        // not in the order of binding points, so binding points may jump between iterations)
        for (int i = 0; i < n_blocks; ++i) {
            GLint block_info[3];
            glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, 3, block_props1, 3, NULL, block_info);
            GLuint binding_point = block_info[2];  // now we have the uniform buffer binding point

            char* name = new char[block_info[1]];
            glGetProgramResourceName(id, GL_UNIFORM_BLOCK, i, block_info[1], NULL, name);
            std::string block_name(name);  // now we have the uniform block name
            delete[] name;

            GLint n_uniforms = block_info[0];
            GLint* indices = new GLint[n_uniforms];
            glGetProgramResourceiv(id, GL_UNIFORM_BLOCK, i, 1, block_props2, n_uniforms, NULL, indices);

            // it's unclear if OpenGL would return the indices in order, this isn't mentioned in the
            // documentation, for safety, we must sort the indices to make sure that they appear in
            // the same order as which uniforms are declared in the GLSL uniform block (vital for std140)
            std::vector<GLint> uniform_indices(indices, indices + n_uniforms);
            std::sort(uniform_indices.begin(), uniform_indices.end(), [](int a, int b) { return a < b; });
            delete[] indices;

            CORE_INFO("Computing std140 aligned offset for the uniform block \"{0}\"", block_name);
            GLenum uniform_props[] = { GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX };

            std::vector<GLuint> offset;
            std::vector<GLuint> size;
            GLuint curr_offset = 0;

            for (auto& index : uniform_indices) {
                GLint unif_info[3];
                glGetProgramResourceiv(id, GL_UNIFORM, index, 3, uniform_props, 3, NULL, unif_info);
                GLenum data_type = unif_info[1];  // now we have the uniform data type

                GLint name_length = unif_info[0];
                char* name = new char[name_length];
                glGetProgramResourceName(id, GL_UNIFORM, index, name_length, NULL, name);
                std::string unif_name(name);  // now we have the uniform name
                delete[] name;

                // find the base alignment and byte size of the uniform
                GLuint base_align, byte_size;
                try {
                    base_align = align140.at(data_type);
                    byte_size = size140.at(data_type);
                }
                catch (const std::out_of_range& oor) {
                    CORE_ERROR("Uniform \"{0}\" is using an unsupported data type...", unif_name);
                    CORE_ASERT(false, "Unable to find the uniform base alignment: {0}", oor.what());
                }

                // the aligned byte offset of a uniform must be equal to a multiple of its base alignment
                GLuint padded_offset = static_cast<GLuint>(ceil(curr_offset / base_align) * base_align);
                offset.push_back(padded_offset);
                size.push_back(byte_size);
                curr_offset = padded_offset + byte_size;
            }

            // after the last iteration, curr_offset = total buffer size (padding included)
            UBOs.try_emplace(binding_point, binding_point, curr_offset, offset, size);
        }
    }

    void Renderer::Free() {
        UBOs.clear();
    }

    void Renderer::Attach(const std::string& title) {
        CORE_TRACE("Attaching scene \"{0}\" ......", title);

        Input::ResetCursor();
        Input::HideCursor();
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

    void Renderer::Clear() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::Flush() {
        glutSwapBuffers();
        glutPostRedisplay();
    }

    void Renderer::Submit(std::initializer_list<entt::entity> entities) {
        for (auto e : entities) {
            render_list.push_back(e);
        }
    }

    // this is called by the glut mainloop event (the display callback)
    void Renderer::DrawScene() {
        Clear();
        
        // update scene data and the render list
        curr_scene->OnSceneRender();
        auto& reg = curr_scene->registry;

        // update camera uniform buffer 0
        if (auto group = reg.group<Camera>(); !group.empty()) {
            auto& camera = group.get<Camera>(group.front());

            if (auto& ubo = UBOs[0]; ubo.Bind()) {
                ubo.SetData(0, val_ptr(camera.T->position));
                ubo.SetData(1, val_ptr(camera.T->forward));
                ubo.SetData(2, val_ptr(camera.GetViewMatrix()));
                ubo.SetData(3, val_ptr(camera.GetProjectionMatrix()));
                ubo.Unbind();
            }
        }
        else {
            CORE_ERROR("Each scene must have one and only one main camera!");
        }

        // update light uniform buffers 1-4
        if (auto group = reg.group<DirectionLight>(); !group.empty()) {
            group.sort([](const entt::entity a, const entt::entity b) { return a < b; });
            GLuint binding_point = 1;

            for (auto e : group) {
                auto& light = group.get<DirectionLight>(e);
                auto& transform = group.get<Transform>(e);

                if (auto& ubo = UBOs[binding_point]; ubo.Bind()) {
                    ubo.SetData(0, light.intensity);
                    ubo.SetData(1, val_ptr(light.color));
                    ubo.SetData(2, val_ptr(transform.forward));
                    ubo.Unbind();
                }

                binding_point++;
            }
        }

        // update light uniform buffers 5-8
        if (auto group = reg.group<PointLight>(); !group.empty()) {
            group.sort([](const entt::entity a, const entt::entity b) { return a < b; });
            GLuint binding_point = 5;

            for (auto e : group) {
                auto& light = group.get<PointLight>(e);
                auto& transform = group.get<Transform>(e);

                if (auto& ubo = UBOs[binding_point]; ubo.Bind()) {
                    ubo.SetData(0, light.intensity);
                    ubo.SetData(1, val_ptr(light.color));
                    ubo.SetData(2, val_ptr(transform.position));
                    ubo.SetData(3, light.linear);
                    ubo.SetData(4, light.quadratic);
                    ubo.SetData(5, light.range);
                    ubo.Unbind();
                }

                binding_point++;
            }
        }

        // update light uniform buffers 9-12
        if (auto group = reg.group<Spotlight>(); !group.empty()) {
            group.sort([](const entt::entity a, const entt::entity b) { return a < b; });
            GLuint binding_point = 9;

            for (auto e : group) {
                auto& light = group.get<Spotlight>(e);
                auto& transform = group.get<Transform>(e);

                if (auto& ubo = UBOs[binding_point]; ubo.Bind()) {
                    ubo.SetData(0, light.intensity);
                    ubo.SetData(1, val_ptr(light.color));
                    ubo.SetData(2, val_ptr(transform.position));
                    ubo.SetData(3, val_ptr(transform.forward));
                    ubo.SetData(4, light.GetInnerCosine());
                    ubo.SetData(5, light.GetOuterCosine());
                    ubo.SetData(6, light.range);
                    ubo.Unbind();
                }

                binding_point++;
            }
        }

        // loop through the render list, draw entities one by one in order
        // (the order of entities in the list is determined by the scene)

        auto group = reg.group<Mesh, Material>();

        for (auto& e : render_list) {
            // skip entities marked as null (a convenient mask to tell if an entity should be drawn)
            if (e == entt::null) continue;

            // a non-null entity must have a mesh and material component attached to be renderable
            CORE_ASERT(group.contains(e), "Entity {0} in the render list is non-renderable!", e);
            
            auto& transform = group.get<Transform>(e);
            auto& tag       = group.get<Tag>(e);
            auto& mesh      = group.get<Mesh>(e);
            auto& material  = group.get<Material>(e);

            material.SetUniform(0, transform.transform);

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

        render_list.clear();
    }

    // this is called by the post event update (after glut mainloop event)
    void Renderer::DrawImGui() {
        bool switch_scene = false;
        std::string next_scene_title = "";

        ui::NewFrame();
        {
            ui::DrawMenuBar(curr_scene->title, next_scene_title);
            ui::DrawStatusBar();

            if (!next_scene_title.empty()) {
                switch_scene = true;
                Clear();
                ui::DrawLoadingScreen();
            }
            else if (Window::layer == Layer::ImGui) {
                curr_scene->OnImGuiRender();
            }
        }
        ui::EndFrame();

        // [side note] difficulties on using multiple threads in OpenGL.
        // ------------------------------------------------------------------------------
        // [1] at any given time, there should be only one active scene, no two scenes
        //     can be alive at the same time in the OpenGL context, so, each time we
        //     switch scenes, we must delete the previous one first to safely clean up
        //     global OpenGL states, before creating the new one from our factory.
        // ------------------------------------------------------------------------------
        // [2] the new scene must be fully loaded and initialized before being assigned
        //     to `curr_scene`, otherwise, `curr_scene` could be pointing to a scene
        //     that has dirty states, so a subsequent draw call could possibly throw an
        //     access violation exception that will crash the program.
        // ------------------------------------------------------------------------------
        // [3] cleaning and loading scenes can take quite a while, especially when there
        //     are complicated meshes. Ideally, this function should be scheduled as an
        //     asynchronous background task that runs in another thread, so as not to
        //     block and freeze the window. To do so, we can wrap the task in std::async,
        //     and then query the result from the std::future object, C++ will handle
        //     concurrency for us, just make sure that the lifespan of the future extend
        //     beyond the calling function.
        // ------------------------------------------------------------------------------
        // [4] sadly though, multi-threading in OpenGL is a pain, you can't multithread
        //     OpenGL calls easily without using complex context switching, because lots
        //     of buffer data cannot be shared between threads, also freeglut and your
        //     graphics card driver may not be supporting it anyway. Sharing context and
        //     resources between threads is not worth the effort, if at all possible.
        //     What we sure can do is to load images and compute textures concurrently,
        //     but for the sake of multi-threading, D3D11 would be a better option.
        // ------------------------------------------------------------------------------

        Flush();

        if (switch_scene) {
            Detach();                  // blocking call
            Attach(next_scene_title);  // blocking call (could take 30 minutes if scene is huge)
        }
    }

}
