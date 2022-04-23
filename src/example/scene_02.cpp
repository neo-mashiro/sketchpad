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
#include "example/scene_02.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    static float skybox_exposure = 1.0f;
    static float skybox_lod      = 0.0f;

    static int   entity_id       = 0;  // 0: sphere, 1: torus, 2: cube, 3: motorbike
    static bool  motor_wireframe = false;
    static float tank_roughness  = 0.72f;

    static vec4  sphere_color[7]     { vec4(color::black, 1.0f) };
    static float sphere_metalness[7] { 0.05f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f };
    static float sphere_roughness[7] { 0.05f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f };
    static float sphere_ao = 0.5f;

    static float cube_metalness = 0.5f;
    static float cube_roughness = 0.5f;
    static int   cube_rotation  = -1;  // -1: no rotation, 0: up, 1: left, 2: right, 3: down
    static int   rotation_mode  = 1;
    static bool  reset_cube     = false;

    static vec4  torus_color     = vec4(color::white, 1.0f);
    static float torus_metalness = 0.5f;
    static float torus_roughness = 0.5f;
    static float torus_ao        = 0.5f;
    static bool  rotate_torus    = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene02::Init() {
        this->title = "Environment Lighting (IBL)";
        PrecomputeIBL(paths::texture + "HDRI\\Field-Path-Steinbacher-Street-4K2.hdr");

        resource_manager.Add(-1, MakeAsset<Mesh>(Primitive::Sphere));
        resource_manager.Add(-2, MakeAsset<Mesh>(Primitive::Cube));
        resource_manager.Add(00, MakeAsset<CShader>(paths::shader + "core\\bloom.glsl"));
        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(03, MakeAsset<Shader>(paths::shader + "core\\light.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_02\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_02\\post_process.glsl"));
        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(13, MakeAsset<Material>(resource_manager.Get<Shader>(03)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));
        resource_manager.Add(98, MakeAsset<Sampler>(FilterMode::Point));
        resource_manager.Add(99, MakeAsset<Sampler>(FilterMode::Bilinear));
        
        AddUBO(resource_manager.Get<Shader>(02)->ID());
        AddUBO(resource_manager.Get<Shader>(03)->ID());
        AddUBO(resource_manager.Get<Shader>(04)->ID());

        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width / 2, Window::height / 2);

        FBOs[0].AddColorTexture(2, true);    // multisampled textures for MSAA
        FBOs[0].AddDepStRenderBuffer(true);  // multisampled RBO for MSAA
        FBOs[1].AddColorTexture(2);
        FBOs[2].AddColorTexture(2);

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

        // create 10 spheres (3 w/ textures + 7 w/o textures)
        const float sphere_posx[] = { 0.0f, -1.5f, 1.5f, -3.0f, 0.0f, 3.0f, -4.5f, -1.5f, 1.5f, 4.5f };
        const float sphere_posy[] = { 10.5f, 7.5f, 7.5f, 4.5f, 4.5f, 4.5f, 1.5f, 1.5f, 1.5f, 1.5f };
        auto sphere_mesh = resource_manager.Get<Mesh>(-1);

        for (int i = 0; i < 10; i++) {
            sphere[i] = CreateEntity("Sphere " + std::to_string(i));
            sphere[i].GetComponent<Transform>().Translate(sphere_posx[i] * world::left);
            sphere[i].GetComponent<Transform>().Translate(sphere_posy[i] * world::up);
            sphere[i].AddComponent<Mesh>(sphere_mesh);

            auto& material = sphere[i].AddComponent<Material>(resource_manager.Get<Material>(14));
            SetupMaterial(material, i);
        }

        // create 3 cubes (2 translation + 1 rotation)
        for (int i = 0; i < 3; i++) {
            cube[i] = CreateEntity("Cube " + std::to_string(i));
            cube[i].AddComponent<Mesh>(resource_manager.Get<Mesh>(-2));
            cube[i].GetComponent<Transform>().Translate(world::left * (i - 1) * 6);
            cube[i].GetComponent<Transform>().Translate(world::up * 5.0f);

            auto& material = cube[i].AddComponent<Material>(resource_manager.Get<Material>(14));
            SetupMaterial(material, i + 10);
        }

        point_light = CreateEntity("Point Light");
        point_light.AddComponent<Mesh>(sphere_mesh);
        point_light.GetComponent<Transform>().Translate(world::up * 6.0f);
        point_light.GetComponent<Transform>().Translate(world::backward * 4.0f);
        point_light.GetComponent<Transform>().Scale(0.1f);
        point_light.AddComponent<PointLight>(color::orange, 1.8f);
        point_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

        if (auto& mat = point_light.AddComponent<Material>(resource_manager.Get<Material>(13)); true) {
            auto& pl = point_light.GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        torus = CreateEntity("Torus");
        torus.AddComponent<Mesh>(Primitive::Torus);
        torus.GetComponent<Transform>().Translate(world::up * 5.0f);
        SetupMaterial(torus.AddComponent<Material>(resource_manager.Get<Material>(14)), 20);

        motorbike = CreateEntity("Motorbike");
        motorbike.GetComponent<Transform>().Rotate(world::up, -90.0f, Space::Local);
        motorbike.GetComponent<Transform>().Scale(0.25f);
        motorbike.GetComponent<Transform>().Translate(vec3(10.0f, 0.0f, 5.0f));

        if (std::string model_path = utils::paths::model + "motorbike\\"; true) {
            auto& model = motorbike.AddComponent<Model>(model_path + "motor.fbx", Quality::Auto);

            SetupMaterial(model.SetMaterial("24 - Default",  resource_manager.Get<Material>(14)), 30);
            SetupMaterial(model.SetMaterial("15 - Default",  resource_manager.Get<Material>(14)), 31);
            SetupMaterial(model.SetMaterial("18 - Default",  resource_manager.Get<Material>(14)), 32);
            SetupMaterial(model.SetMaterial("21 - Default",  resource_manager.Get<Material>(14)), 33);
            SetupMaterial(model.SetMaterial("23 - Default",  resource_manager.Get<Material>(14)), 34);
            SetupMaterial(model.SetMaterial("20 - Default",  resource_manager.Get<Material>(14)), 35);
            SetupMaterial(model.SetMaterial("17 - Default",  resource_manager.Get<Material>(14)), 36);
            SetupMaterial(model.SetMaterial("22 - Default",  resource_manager.Get<Material>(14)), 37);
            SetupMaterial(model.SetMaterial("Material #308", resource_manager.Get<Material>(14)), 38);
            SetupMaterial(model.SetMaterial("Material #706", resource_manager.Get<Material>(14)), 39);
        }

        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::AlphaBlend(true);
        Renderer::FaceCulling(true);
    }

    void Scene02::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        if (auto& ubo = UBOs[0]; true) {
            ubo.SetUniform(0, val_ptr(main_camera.T->position));
            ubo.SetUniform(1, val_ptr(main_camera.T->forward));
            ubo.SetUniform(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetUniform(3, val_ptr(main_camera.GetProjectionMatrix()));
        }

        if (auto& ubo = UBOs[1]; true) {
            auto& pl = point_light.GetComponent<PointLight>();
            auto& pt = point_light.GetComponent<Transform>();
            ubo.SetUniform(0, val_ptr(pl.color));
            ubo.SetUniform(1, val_ptr(pt.position));
            ubo.SetUniform(2, val_ptr(pl.intensity));
            ubo.SetUniform(3, val_ptr(pl.linear));
            ubo.SetUniform(4, val_ptr(pl.quadratic));
            ubo.SetUniform(5, val_ptr(pl.range));
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];
        FBO& framebuffer_2 = FBOs[2];

        // ------------------------------ MRT render pass ------------------------------

        framebuffer_0.Clear();
        framebuffer_0.Bind();

        /**/ if (entity_id == 0) { RenderSphere(); }
        else if (entity_id == 1) { RenderTorus();  }
        else if (entity_id == 2) { RenderCubes();  }
        else if (entity_id == 3) { RenderMotor(); }

        Renderer::Submit(point_light.id);
        Renderer::Submit(skybox.id);
        Renderer::Render();

        framebuffer_0.Unbind();

        // ------------------------------ MSAA resolve pass ------------------------------

        framebuffer_1.Clear();
        FBO::CopyColor(framebuffer_0, 0, framebuffer_1, 0);
        FBO::CopyColor(framebuffer_0, 1, framebuffer_1, 1);

        // ------------------------------ apply Gaussian blur ------------------------------

        FBO::CopyColor(framebuffer_1, 1, framebuffer_2, 0);  // downsample the bloom target (nearest filtering)
        auto& ping = framebuffer_2.GetColorTexture(0);
        auto& pong = framebuffer_2.GetColorTexture(1);
        auto bloom_shader = resource_manager.Get<CShader>(00);

        bloom_shader->Bind();
        ping.BindILS(0, 0, GL_READ_WRITE);
        pong.BindILS(0, 1, GL_READ_WRITE);

        for (int i = 0; i < 6; ++i) {
            bloom_shader->SetUniform(0, i % 2 == 0);
            bloom_shader->Dispatch(ping.width / 32, ping.width / 18);
            bloom_shader->SyncWait(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        // ------------------------------ postprocessing pass ------------------------------

        framebuffer_1.GetColorTexture(0).Bind(0);  // color texture
        framebuffer_2.GetColorTexture(0).Bind(1);  // bloom texture

        auto bilinear_sampler = resource_manager.Get<Sampler>(99);
        bilinear_sampler->Bind(1);  // upsample the bloom texture (bilinear filtering)

        auto postprocess_shader = resource_manager.Get<Shader>(05);
        postprocess_shader->Bind();
        postprocess_shader->SetUniform(0, 3);  // select tone-mapping operator

        Renderer::Clear();
        Mesh::DrawQuad();

        postprocess_shader->Unbind();
        bilinear_sampler->Unbind(1);
    }

    void Scene02::OnImGuiRender() {
        using namespace ImGui;
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        static bool edit_sphere_metalness = false;
        static bool edit_sphere_roughness = false;
        static int z_mode = -1;  // cube gizmo mode

        // 3 x 3 cube rotation panel
        const bool cell_enabled[9] = { false, true, false, true, false, true, false, true, false };
        const char* cell_label[4] = { ICON_FK_LONG_ARROW_UP, ICON_FK_LONG_ARROW_LEFT, ICON_FK_LONG_ARROW_RIGHT, ICON_FK_LONG_ARROW_DOWN };
        const ImVec2 cell_size = ImVec2(40, 40);

        if (ui::NewInspector()) {
            Indent(5.0f);
            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.5f, 2.0f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Separator();

            Text("Entity to Render");
            Separator();
            RadioButton("Static Sphere", &entity_id, 0); SameLine(164);
            RadioButton("Color Torus",   &entity_id, 1);
            RadioButton("Spinning Cube", &entity_id, 2); SameLine(164);
            RadioButton("MotorCycle",    &entity_id, 3);
            Separator();

            if (CollapsingHeader("Static Sphere", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Ambient Occlusion##1", &sphere_ao, 0.05f, 0.5f);                
                PopItemWidth();

                if (Checkbox("Edit Metalness", &edit_sphere_metalness); edit_sphere_metalness) {
                    PushID("Metalness Sliders");
                    for (int i = 0; i < 7; i++) {
                        float hue = i / 7.0f;
                        PushID(i);
                        PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(hue, 0.5f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(hue, 0.6f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(hue, 0.7f, 0.5f));
                        PushStyleColor(ImGuiCol_SliderGrab,     (ImVec4)ImColor::HSV(hue, 0.9f, 0.9f));

                        VSliderFloat("##m", ImVec2(20, 160), &sphere_metalness[i], 0.0f, 1.0f, "");
                        if (IsItemActive() || IsItemHovered()) {
                            SetTooltip("%.3f", sphere_metalness[i]);
                        }

                        PopStyleColor(4);
                        PopID();
                        SameLine();
                    }
                    PopID();
                    NewLine();
                }

                if (Checkbox("Edit Roughness", &edit_sphere_roughness); edit_sphere_roughness) {
                    PushID("Roughness Sliders");
                    for (int i = 0; i < 7; i++) {
                        float hue = i / 7.0f;
                        PushID(i);
                        PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(hue, 0.5f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(hue, 0.6f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(hue, 0.7f, 0.5f));
                        PushStyleColor(ImGuiCol_SliderGrab,     (ImVec4)ImColor::HSV(hue, 0.9f, 0.9f));

                        VSliderFloat("##r", ImVec2(20, 160), &sphere_roughness[i], 0.01f, 1.0f, "");
                        if (IsItemActive() || IsItemHovered()) {
                            SetTooltip("%.3f", sphere_roughness[i]);
                        }

                        PopStyleColor(4);
                        PopID();
                        SameLine();
                    }
                    PopID();
                    NewLine();
                }
            }

            if (CollapsingHeader("Spinning Cube", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Metalness##2", &cube_metalness, 0.0f, 1.0f);
                SliderFloat("Roughness##2", &cube_roughness, 0.0f, 1.0f);
                PopItemWidth();
                Separator();

                Text("Rotation Mode");
                RadioButton("Local Space", &rotation_mode, 1); SameLine();
                RadioButton("World Space", &rotation_mode, 2);
                Separator();

                Text("Gizmo Edit Mode");
                RadioButton("T", &z_mode, 1); SameLine();
                RadioButton("R", &z_mode, 2); SameLine();
                RadioButton("S", &z_mode, 3); SameLine();
                RadioButton("N/A", &z_mode, 0);
                Separator();

                BeginGroup();
                for (int row = 0; row < 3; row++) {
                    for (int col = 0; col < 3; col++) {
                        int index = 3 * row + col;
                        ImVec2 alignment = ImVec2((float)col / 2.0f, (float)row / 2.0f);
                        PushStyleVar(ImGuiStyleVar_SelectableTextAlign, alignment);
                        PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);

                        if (col > 0) SameLine();
                        if (cell_enabled[index] && !reset_cube) {
                            int direction = (index - 1) / 2;
                            bool active = cube_rotation == direction;
                            if (Selectable(cell_label[direction], active, 0, cell_size)) {
                                cube_rotation = active ? -1 : direction;
                            }
                        }
                        else if (row == 1 && col == 1) {
                            ui::PushRotation();
                            Selectable(ICON_FK_REPEAT, false, ImGuiSelectableFlags_Disabled, cell_size);
                            ui::PopRotation(Clock::time * 4.0f, false);
                        }
                        else {
                            Selectable("##empty", false, ImGuiSelectableFlags_Disabled, cell_size);
                        }

                        if (index == 5) {
                            if (SameLine(170.0f); Button("RESET", ImVec2(80.0f, 42.0f))) {
                                reset_cube = true;
                            }
                        }

                        PopStyleVar(2);
                    }
                }
                EndGroup();
            }

            if (CollapsingHeader("Color Torus", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Metalness##3", &torus_metalness, 0.00f, 1.0f);
                SliderFloat("Roughness##3", &torus_roughness, 0.01f, 1.0f);
                SliderFloat("Ambient Occlusion##3", &torus_ao, 0.05f, 0.5f);
                PopItemWidth();
                Checkbox("Torus Rotation", &rotate_torus);
            }

            if (CollapsingHeader("Motorbike", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                Checkbox("Wireframe Mode", &motor_wireframe);
                SliderFloat("Tank Roughness", &tank_roughness, 0.1f, 0.72f);
                PopItemWidth();
            }

            Unindent(5.0f);
            ui::EndInspector();
        }

        if (entity_id == 2 && z_mode > 0) {
            ui::DrawGizmo(camera, cube[1], static_cast<ui::Gizmo>(z_mode));
        }
        else {
            ui::DrawGizmo(camera, point_light, ui::Gizmo::Translate);
        }
    }

    void Scene02::PrecomputeIBL(const std::string& hdri) {
        Renderer::SeamlessCubemap(true);
        Renderer::DepthTest(false);
        Renderer::FaceCulling(true);

        auto irradiance_shader = CShader(paths::shader + "core\\irradiance_map.glsl");
        auto prefilter_shader  = CShader(paths::shader + "core\\prefilter_envmap.glsl");
        auto envBRDF_shader    = CShader(paths::shader + "core\\environment_BRDF.glsl");

        std::string rootpath = utils::paths::root;
        if (rootpath.find("mashiro") == std::string::npos) {
            irradiance_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
            prefiltered_map = MakeAsset<Texture>(hdri, 1024, 8);
            Texture::Copy(*prefiltered_map, 3, *irradiance_map, 0);
            BRDF_LUT = MakeAsset<Texture>(paths::texture + "common\\checkboard.png", 1);
            Sync::WaitFinish();
            return;
        }

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

    void Scene02::SetupMaterial(Material& pbr_mat, int mat_id) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        pbr_mat.BindUniform(0, &skybox_exposure);

        if (mat_id == 0) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.0f);
        }
        else if (mat_id == 1) {
            std::string tex_path = paths::texture + "brick_072\\";
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "albedo.jpg"));
            pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(tex_path + "normal.jpg"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "roughness.jpg"));
            pbr_mat.SetTexture(pbr_t::ao,        MakeAsset<Texture>(tex_path + "ao.jpg"));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(3.0f));
        }
        else if (mat_id == 2) {
            std::string tex_path = paths::texture + "marble_020\\";
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "albedo.jpg"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "roughness.jpg"));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(3.0f));
        }
        else if (mat_id >= 3 && mat_id <= 9) {
            unsigned int offset = mat_id - 3;
            sphere_color[offset] = vec4(utils::math::HSV2RGB(offset / 7.0f, 0.9f, 0.9f), 1.0f);
            pbr_mat.BindUniform(pbr_u::albedo, sphere_color + offset);
            pbr_mat.BindUniform(pbr_u::metalness, sphere_metalness + offset);
            pbr_mat.BindUniform(pbr_u::roughness, sphere_roughness + offset);
            pbr_mat.BindUniform(pbr_u::ao, &sphere_ao);
        }
        else if (mat_id >= 10 && mat_id <= 12) {
            pbr_mat.BindUniform(pbr_u::metalness, &cube_metalness);
            pbr_mat.BindUniform(pbr_u::roughness, &cube_roughness);
            pbr_mat.SetUniform(pbr_u::ao, 0.5f);
        }
        else if (mat_id == 20) {
            pbr_mat.BindUniform(pbr_u::albedo, &torus_color);
            pbr_mat.BindUniform(pbr_u::metalness, &torus_metalness);
            pbr_mat.BindUniform(pbr_u::roughness, &torus_roughness);
            pbr_mat.BindUniform(pbr_u::ao, &torus_ao);
        }
        else if (mat_id == 30) {  // motorbike
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::black, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.62f);
        }
        else if (mat_id == 31) {
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(8.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(paths::model + "motorbike\\albedo.png"));
            pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(paths::model + "motorbike\\normal.png"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(paths::model + "motorbike\\roughness.png"));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
        }
        else if (mat_id == 32) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.138f, 0.0f, 1.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.BindUniform(pbr_u::roughness, &tank_roughness);
        }
        else if (mat_id == 33) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::black, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.68f);
        }
        else if (mat_id == 34) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.28f, 0.28f, 0.28f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.62f);
        }
        else if (mat_id == 35) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.53f, 0.65f, 0.87f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.7f);
        }
        else if (mat_id == 36) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.4f, 0.4f, 0.4f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.72f);
        }
        else if (mat_id == 37) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::black, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.76f);
            pbr_mat.SetTexture(pbr_t::normal, MakeAsset<Texture>(paths::model + "motorbike\\normal22.png"));
        }
        else if (mat_id == 38) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::white, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.0f);
        }
        else if (mat_id == 39) {
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.25f, 0.25f, 0.25f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.65f);
        }

        if (mat_id >= 30 && mat_id <= 39) {  // motorbike
            pbr_mat.BindUniform(1, &motor_wireframe);
        }
    }

    void Scene02::RenderSphere() {
        Renderer::Submit(sphere[0].id, sphere[1].id, sphere[2].id, sphere[3].id, sphere[4].id);
        Renderer::Submit(sphere[5].id, sphere[6].id, sphere[7].id, sphere[8].id, sphere[9].id);
    }

    void Scene02::RenderTorus() {
        float hue = math::Bounce(Clock::time * 0.05f, 1.0f);
        torus_color = vec4(math::HSV2RGB(vec3(hue, 1.0f, 1.0f)), 1.0f);

        if (rotate_torus) {
            torus.GetComponent<Transform>().Rotate(world::right, 0.36f, Space::Local);
        }

        Renderer::Submit(torus.id);
    }

    void Scene02::RenderCubes() {
        float delta_distance = cos(Clock::time * 1.5f) * 0.02f;
        cube[0].GetComponent<Transform>().Translate(delta_distance * world::up);
        cube[2].GetComponent<Transform>().Translate(delta_distance * world::down);

        auto& T = cube[1].GetComponent<Transform>();

        switch (cube_rotation) {
            case 00: T.Rotate(world::left,  0.5f, static_cast<Space>(rotation_mode)); break;
            case 01: T.Rotate(world::down,  0.5f, static_cast<Space>(rotation_mode)); break;
            case 02: T.Rotate(world::up,    0.5f, static_cast<Space>(rotation_mode)); break;
            case 03: T.Rotate(world::right, 0.5f, static_cast<Space>(rotation_mode)); break;
            case -1: default: break;
        }

        if (reset_cube) {
            const vec3 origin = vec3(0.0f, 5.0f, 0.0f);
            cube_rotation = -1;

            float t = math::EaseFactor(5.0f, Clock::delta_time);
            T.SetPosition(math::Lerp(T.position, origin, t));
            T.SetRotation(math::SlerpRaw(T.rotation, world::eye, t));

            if (math::Equals(T.position, origin) && math::Equals(T.rotation, world::eye)) {
                reset_cube = false;
            }
        }

        Renderer::Submit(cube[0].id, cube[1].id, cube[2].id);
    }

    void Scene02::RenderMotor() {
        Renderer::Submit(motorbike.id);
    }

}
