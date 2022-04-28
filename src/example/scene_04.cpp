#include "pch.h"

#include "core/base.h"
#include "core/clock.h"
#include "core/debug.h"
#include "core/input.h"
#include "core/window.h"
#include "core/sync.h"
#include "asset/all.h"
#include "component/all.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/ext.h"
#include "utils/math.h"
#include "utils/path.h"
#include "example/scene_04.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    static const ivec2 n_verts = ivec2(32, 32);       // number of vertices in each dimension
    static const vec2 cloth_sz = vec2(16.0f, 12.0f);  // size of the cloth/lattice

    static bool  show_grid       = true;
    static float grid_cell_size  = 2.0f;
    static vec4  thin_line_color = vec4(0.1f, 0.1f, 0.1f, 1.0f);
    static vec4  wide_line_color = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    static vec3  dl_direction    = vec3(0.0f, -1.0f, 1.0f);

    static float skybox_exposure = 1.0f;
    static float skybox_lod      = 0.0f;

    static bool  rotate_model    = false;
    static bool  simulate        = false;
    static bool  show_wireframe  = false;
    static vec4  wireframe_color = vec4(1.0f);
    static int   n_indices       = 0;
    static uint  rd_buffer       = 0;
    static uint  wt_buffer       = 1;

    static vec4  albedo        = vec4(color::black, 1.0f);
    static float roughness     = 1.0f;
    static float ao            = 1.0f;
    static vec3  sheen_color   = color::blue;
    static vec3  subsurf_color = vec3(0.15f);
    static uvec2 shading_model = uvec2(3, 0);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene04::Init() {
        this->title = "Compute Shader Cloth Simulation";
        PrecomputeIBL(utils::paths::texture + "HDRI\\loc00184-22-4k.hdr");

        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_04\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_04\\post_process.glsl"));
        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));
        resource_manager.Add(30, MakeAsset<CShader>(paths::shader + "scene_04\\cloth_position.glsl"));
        resource_manager.Add(31, MakeAsset<CShader>(paths::shader + "scene_04\\cloth_normal.glsl"));
        
        AddUBO(resource_manager.Get<Shader>(04)->ID());
        AddUBO(resource_manager.Get<Shader>(02)->ID());

        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);

        FBOs[0].AddColorTexture(1, true);
        FBOs[0].AddDepStRenderBuffer(true);
        FBOs[1].AddColorTexture(1);

        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(0.0f, 6.0f, 9.0f);
        camera.AddComponent<Camera>(View::Perspective);
        
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(resource_manager.Get<Material>(12)); true) {
            mat.SetTexture(0, prefiltered_map);
            mat.BindUniform(0, &skybox_exposure);
            mat.BindUniform(1, &skybox_lod);
        }

        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(45.0f, 180.0f, 0.0f, Space::World);
        direct_light.AddComponent<DirectionLight>(color::white, 0.5f);

        cloth_model = CreateEntity("Cloth Model");
        cloth_model.GetComponent<Transform>().Translate(world::up * 4.0f);
        cloth_model.GetComponent<Transform>().Scale(2.0f);

        if (auto& model = cloth_model.AddComponent<Model>(paths::model + "cloth.obj", Quality::Auto); true) {
            SetupMaterial(model.SetMaterial("cloth", resource_manager.Get<Material>(14)), true, false);
            SetupMaterial(model.SetMaterial("outside", resource_manager.Get<Material>(14)), false, false);
        }

        // dynamic cloth simulation
        SetupBuffers();
        auto cloth_mat = resource_manager.Get<Material>(14);
        SetupMaterial(*cloth_mat, true, true);
        cloth_mat->SetUniform(1000U, world::identity);
        cloth_mat->BindUniform(pbr_u::shading_model, &shading_model);

        Renderer::PrimitiveRestart(true);
        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::AlphaBlend(true);
    }

    void Scene04::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        if (auto& ubo = UBOs[0]; true) {
            ubo.SetUniform(0, val_ptr(main_camera.T->position));
            ubo.SetUniform(1, val_ptr(main_camera.T->forward));
            ubo.SetUniform(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetUniform(3, val_ptr(main_camera.GetProjectionMatrix()));
        }

        if (auto& ubo = UBOs[1]; true) {
            auto& dl = direct_light.GetComponent<DirectionLight>();
            vec3 direction = -glm::normalize(dl_direction);
            ubo.SetUniform(0, val_ptr(dl.color));
            ubo.SetUniform(1, val_ptr(direction));
            ubo.SetUniform(2, val_ptr(dl.intensity));
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];

        // ------------------------------ simulation & render pass ------------------------------

        framebuffer_0.Clear();
        framebuffer_0.Bind();

        // since cloth depends on alpha blending we need to render the skybox before it
        Renderer::FaceCulling(true);
        Renderer::Submit(skybox.id);
        Renderer::Render();
        Renderer::FaceCulling(false);

        if (simulate) {
            // update cloth vertex positions
            auto simulation_cs = resource_manager.Get<CShader>(30);
            simulation_cs->Bind();

            for (int i = 0; i < 512; ++i) {
                simulation_cs->Dispatch(n_verts.x / 32, n_verts.y / 32);
                simulation_cs->SyncWait();
                std::swap(rd_buffer, wt_buffer);

                cloth_pos[rd_buffer]->Reset(0);
                cloth_pos[wt_buffer]->Reset(1);
                cloth_vel[rd_buffer]->Reset(2);
                cloth_vel[wt_buffer]->Reset(3);
            }

            // update cloth vertex normals
            auto normal_cs = resource_manager.Get<CShader>(31);
            normal_cs->Bind();
            normal_cs->Dispatch(n_verts.x / 32, n_verts.y / 32);
            normal_cs->SyncWait();

            resource_manager.Get<Material>(14)->Bind();
            cloth_vao->Draw(GL_TRIANGLE_STRIP, n_indices);
        }
        else {
            if (rotate_model) {
                cloth_model.GetComponent<Transform>().Rotate(world::up, 0.2f, Space::Local);
            }
            Renderer::Submit(cloth_model.id);
            Renderer::Render();
        }

        if (show_grid) {
            auto grid_shader = resource_manager.Get<Shader>(01);
            grid_shader->Bind();
            grid_shader->SetUniform(0, grid_cell_size);
            grid_shader->SetUniform(1, thin_line_color);
            grid_shader->SetUniform(2, wide_line_color);
            Mesh::DrawGrid();
        }

        framebuffer_0.Unbind();

        // ------------------------------ MSAA resolve pass ------------------------------
        
        framebuffer_1.Clear();
        FBO::CopyColor(framebuffer_0, 0, framebuffer_1, 0);

        // ------------------------------ postprocessing pass ------------------------------

        framebuffer_1.GetColorTexture(0).Bind(0);
        auto postprocess_shader = resource_manager.Get<Shader>(05);
        postprocess_shader->Bind();
        postprocess_shader->SetUniform(0, 3);

        Renderer::Clear();
        Mesh::DrawQuad();
        postprocess_shader->Unbind();
    }

    void Scene04::OnImGuiRender() {
        using namespace ImGui;
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        const ImVec2 rainbow_offset = ImVec2(5.0f, 105.0f);
        const ImVec4 tab_color_off  = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
        const ImVec4 tab_color_on   = ImVec4(0.0f, 0.4f, 0.8f, 1.0f);
        static bool simulation_clearcoat = false;
        static float cloth_alpha = 1.0f;

        if (ui::NewInspector()) {
            Indent(5.0f);
            Text(ICON_FK_SUN_O "  Directional Light Vector");
            DragFloat3("###", val_ptr(dl_direction), 0.01f, -1.0f, 1.0f, "%.3f");
            ui::DrawRainbowBar(rainbow_offset, 2.0f);
            Spacing();

            // to create a velvet-like material, set albedo to black and use a bright saturated
            // sheen color, for cotton or denim, use albedo as base color, then set sheen color
            // to a brighter one of the same hue, or a blend of albedo and incoming light color
            // to simulate the forward and backward scattering of fibers, for leather and silk,
            // since there's barely any surface lusters and subsurface scattering, the standard
            // shading model with properly tweaked anisotropic patterns usually fits better.

            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.5f, 4.0f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Separator();

            BeginTabBar("InspectorTab", ImGuiTabBarFlags_None);

            if (BeginTabItem("Static Model")) {
                PushItemWidth(130.0f);
                Checkbox("Show Wireframe", &show_wireframe); SameLine();
                ColorEdit3("Line Color", val_ptr(wireframe_color), color_flags);
                Checkbox("Auto Rotation", &rotate_model);
                SliderFloat("Roughness", &roughness, 0.045f, 1.0f);
                SliderFloat("Ambient Occlusion", &ao, 0.05f, 1.0f);
                if (SliderFloat("Transparency", &cloth_alpha, 0.5f, 1.0f)) {
                    albedo.a = cloth_alpha * 0.1f + 0.9f;
                }
                ColorEdit3("Albedo", val_ptr(albedo), color_flags); SameLine();
                ColorEdit3("Sheen", val_ptr(sheen_color), color_flags); SameLine();
                ColorEdit3("Subsurface", val_ptr(subsurf_color), color_flags);
                PopItemWidth();
                EndTabItem();
            }

            if (simulate = BeginTabItem("Simulation"); simulate) {
                Checkbox("Show Wireframe", &show_wireframe); SameLine();
                ColorEdit3("Line Color", val_ptr(wireframe_color), color_flags);
                Checkbox("Apply Clearcoat", &simulation_clearcoat);
                shading_model = simulation_clearcoat ? uvec2(3, 1) : uvec2(3, 0);

                if (ArrowButton("##1", ImGuiDir_Left)) {
                    resource_manager.Get<CShader>(30)->SetUniform(1, 20.0f * world::left);
                }
                if (SameLine(); ArrowButton("##2", ImGuiDir_Right)) {
                    resource_manager.Get<CShader>(30)->SetUniform(1, 20.0f * world::right);
                }
                if (SameLine(); ArrowButton("##3", ImGuiDir_Up)) {
                    resource_manager.Get<CShader>(30)->SetUniform(1, 20.0f * world::up);
                }
                if (SameLine(); ArrowButton("##4", ImGuiDir_Down)) {
                    resource_manager.Get<CShader>(30)->SetUniform(1, world::zero);
                }

                SameLine(); Text("Wind Direction");
                Spacing();
                if (Button("Reset Lattice", ImVec2(150.0f, 0.0f))) {
                    cloth_pos[0]->SetData(&init_pos[0]);
                    cloth_vel[0]->SetData(&init_vel[0]);
                    cloth_pos[1]->SetData(NULL);
                    cloth_vel[1]->SetData(NULL);
                }
                EndTabItem();
            }

            PushStyleColor(ImGuiCol_Tab, tab_color_off);
            PushStyleColor(ImGuiCol_TabHovered, tab_color_on);
            PushStyleColor(ImGuiCol_TabActive, tab_color_on);

            if (BeginTabItem(ICON_FK_TH_LARGE)) {
                PushItemWidth(130.0f);
                Checkbox("Show Infinite Grid", &show_grid);
                SliderFloat("Grid Cell Size", &grid_cell_size, 0.25f, 8.0f);
                PopItemWidth();
                ColorEdit4("Line Color Minor", val_ptr(thin_line_color), color_flags);
                ColorEdit4("Line Color Main", val_ptr(wide_line_color), color_flags);
                EndTabItem();
            }

            PopStyleColor(3);
            EndTabBar();

            Unindent(5.0f);
            ui::EndInspector();
        }
    }

    void Scene04::PrecomputeIBL(const std::string& hdri) {
        Renderer::SeamlessCubemap(true);
        Renderer::DepthTest(false);
        Renderer::FaceCulling(true);

        auto irradiance_shader = CShader(paths::shader + "core\\irradiance_map.glsl");
        auto prefilter_shader  = CShader(paths::shader + "core\\prefilter_envmap.glsl");
        auto envBRDF_shader    = CShader(paths::shader + "core\\environment_BRDF.glsl");

        auto env_map = MakeAsset<Texture>(hdri, 2048, 0);
        env_map->Bind(0);

        irradiance_map  = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
        prefiltered_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 2048, 2048, 6, GL_RGBA16F, 8);
        BRDF_LUT        = MakeAsset<Texture>(GL_TEXTURE_2D, 1024, 1024, 1, GL_RGBA16F, 1);

        CORE_INFO("Precomputing diffuse irradiance map from {0}", hdri);
        irradiance_map->BindILS(0, 0, GL_WRITE_ONLY);

        if (irradiance_shader.Bind(); true) {
            irradiance_shader.Dispatch(128 / 32, 128 / 32, 6);
            irradiance_shader.SyncWait(GL_TEXTURE_FETCH_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

            auto irradiance_fence = Sync(0);
            irradiance_fence.ClientWaitSync();
            irradiance_map->UnbindILS(0);
        }

        CORE_INFO("Precomputing specular prefiltered envmap from {0}", hdri);
        Texture::Copy(*env_map, 0, *prefiltered_map, 0);  // copy the base level

        const GLuint max_level = prefiltered_map->n_levels - 1;
        GLuint resolution = prefiltered_map->width / 2;
        prefilter_shader.Bind();

        for (unsigned int level = 1; level <= max_level; level++, resolution /= 2) {
            float roughness = level / static_cast<float>(max_level);
            GLuint n_groups = glm::max<GLuint>(resolution / 32, 1);

            prefiltered_map->BindILS(level, 1, GL_WRITE_ONLY);
            prefilter_shader.SetUniform(0, roughness);
            prefilter_shader.Dispatch(n_groups, n_groups, 6);
            prefilter_shader.SyncWait(GL_TEXTURE_FETCH_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

            auto prefilter_fence = Sync(level);
            prefilter_fence.ClientWaitSync();
            prefiltered_map->UnbindILS(1);
        }

        CORE_INFO("Precomputing specular environment BRDF from {0}", hdri);
        BRDF_LUT->BindILS(0, 2, GL_WRITE_ONLY);

        if (envBRDF_shader.Bind(); true) {
            envBRDF_shader.Dispatch(1024 / 32, 1024 / 32, 1);
            envBRDF_shader.SyncWait(GL_ALL_BARRIER_BITS);
            Sync::WaitFinish();
            BRDF_LUT->UnbindILS(2);
        }
    }

    void Scene04::SetupBuffers() {
        const GLuint n_rows = n_verts.y;
        const GLuint n_cols = n_verts.x;
        const GLuint n = n_cols * n_rows;
        const float dx = cloth_sz.x / (n_cols - 1);
        const float dy = cloth_sz.y / (n_rows - 1);
        const float du = 1.0f / (n_cols - 1);
        const float dv = 1.0f / (n_rows - 1);

        init_pos.reserve(n);
        init_vel.reserve(n);
        tex_coord.reserve(n);

        Transform T = Transform();
        T.Rotate(world::right, -90.0f, Space::Local);
        T.Translate(-cloth_sz.x * 0.5f, 4.0f, cloth_sz.y * 0.5f);

        for (int row = 0; row < n_rows; row++) {
            for (int col = 0; col < n_cols; col++) {
                vec4 position = T.transform * vec4(dx * col, dy * row, 0.0f, 1.0f);
                vec4 velocity = vec4(0.0f);
                vec2 uv = vec2(du * col, dv * row);

                init_pos.push_back(position);
                init_vel.push_back(velocity);
                tex_coord.push_back(uv);
            }
        }

        // indices are built such that every two adjacent rows define a triangle strip
        // note that face direction of the strip is determined by the winding order of
        // the 1st triangle, and each successive triangle will have its effective face
        // order reversed to maintain that order, this is automatically done by OpenGL

        for (int row = 0; row < n_rows - 1; row++) {
            indices.push_back(0xFFFFFF);  // primitive restart index
            for (int col = 0; col < n_cols; col++) {
                indices.push_back(row * n_cols + col + n_cols);
                indices.push_back(row * n_cols + col);
            }
        }

        n_indices = static_cast<int>(indices.size());
        static_assert(sizeof(vec4) == 4 * sizeof(GLfloat), "GL floats in vec4 are not tightly packed!");
        static_assert(sizeof(vec2) == 2 * sizeof(GLfloat), "GL floats in vec2 are not tightly packed!");

        cloth_vao = WrapAsset<VAO>();
        cloth_vbo = WrapAsset<VBO>(n * sizeof(vec2), &tex_coord[0]);
        cloth_ibo = WrapAsset<IBO>(n_indices * sizeof(GLuint), &indices[0]);

        cloth_pos[0] = WrapAsset<SSBO>(0, n * sizeof(vec4), GL_DYNAMIC_STORAGE_BIT);
        cloth_pos[1] = WrapAsset<SSBO>(1, n * sizeof(vec4), GL_DYNAMIC_STORAGE_BIT);
        cloth_vel[0] = WrapAsset<SSBO>(2, n * sizeof(vec4), GL_DYNAMIC_STORAGE_BIT);
        cloth_vel[1] = WrapAsset<SSBO>(3, n * sizeof(vec4), GL_DYNAMIC_STORAGE_BIT);
        cloth_normal = WrapAsset<SSBO>(4, n * sizeof(vec4), GL_DYNAMIC_STORAGE_BIT);

        cloth_pos[0]->SetData(&init_pos[0]);
        cloth_vel[0]->SetData(&init_vel[0]);

        cloth_vao->SetVBO(cloth_pos[0]->ID(), 0, 0, 3, sizeof(vec4), GL_FLOAT);  // in vec3 position
        cloth_vao->SetVBO(cloth_normal->ID(), 1, 0, 3, sizeof(vec4), GL_FLOAT);  // in vec3 normal
        cloth_vao->SetVBO(cloth_vbo->ID(), 2, 0, 2, sizeof(vec2), GL_FLOAT);     // in vec2 uv
        cloth_vao->SetIBO(cloth_ibo->ID());

        auto simulation_cs = resource_manager.Get<CShader>(30);
        simulation_cs->SetUniform(0, vec3(0.0f, -10.0f, 0.0f));
        simulation_cs->SetUniform(1, vec3(0.0f));
        simulation_cs->SetUniform(2, dx);
        simulation_cs->SetUniform(3, dy);
        simulation_cs->SetUniform(4, sqrtf(dx * dx + dy * dy));
    }

    void Scene04::SetupMaterial(Material& pbr_mat, bool cloth, bool textured) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        pbr_mat.BindUniform(0, &show_wireframe);
        pbr_mat.BindUniform(1, &wireframe_color);
        pbr_mat.SetUniform(2, 0.05f);
        pbr_mat.BindUniform(3, &skybox_exposure);

        if (cloth) {
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(3, 0));
            pbr_mat.SetUniform(pbr_u::clearcoat, 1.0f);
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f, 4.0f));
            std::string tex_path = paths::texture + "fabric\\";

            if (textured) {
                pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "albedo.jpg"));
                pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(tex_path + "normal.jpg"));
                pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "roughness.jpg"));
                pbr_mat.SetTexture(pbr_t::ao,        MakeAsset<Texture>(tex_path + "ao.jpg"));
                pbr_mat.SetUniform(pbr_u::sheen_color, color::white);
                pbr_mat.SetUniform(pbr_u::subsurf_color, color::black);
            }
            else {
                pbr_mat.BindUniform(pbr_u::albedo, &albedo);
                pbr_mat.SetTexture(pbr_t::normal, MakeAsset<Texture>(tex_path + "normal.jpg"));
                pbr_mat.BindUniform(pbr_u::roughness, &roughness);
                pbr_mat.BindUniform(pbr_u::ao, &ao);
                pbr_mat.BindUniform(pbr_u::sheen_color, &sheen_color);
                pbr_mat.BindUniform(pbr_u::subsurf_color, &subsurf_color);
            }
        }
        else {
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.8f);
            pbr_mat.SetUniform(pbr_u::ao, 1.0f);
        }
    }

}