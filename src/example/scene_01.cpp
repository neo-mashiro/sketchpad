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
#include "example/scene_01.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    // the global scope in the cpp file should only be used to define config variables
    // these are internal to this translation unit so won't be visible to other scenes
    // since the lifetime of a static variable is persistent, these data are preserved
    // in global memory even when we switch back and forth between scenes

    static bool  show_grid       = false;
    static float grid_cell_size  = 2.0f;
    static vec4  thin_line_color = vec4(0.1f, 0.1f, 0.1f, 1.0f);
    static vec4  wide_line_color = vec4(0.2f, 0.2f, 0.2f, 1.0f);

    static float skybox_exposure = 1.0f;
    static float skybox_lod      = 0.0f;

    static bool  show_plane         = true;
    static bool  show_light_cluster = true;
    static bool  draw_depth_buffer  = false;
    static bool  orbit              = true;
    static float orbit_speed        = 0.5f;
    static int   tone_mapping_mode  = 3;
    static int   n_blurs            = 3;

    static vec4  sphere_albedo { 0.22f, 0.0f, 1.0f, 1.0f };
    static float sphere_metalness = 0.05f;
    static float sphere_roughness = 0.05f;
    static float sphere_ao        = 1.0f;
    static float plane_roughness  = 0.1f;
    static float light_cluster_intensity = 10.0f;

    constexpr GLuint tile_size = 16;
    constexpr GLuint n_pls = 28;  // number of point lights
    static GLuint nx = 0, ny = 0;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        this->title = "Tiled Forward Renderer";
        PrecomputeIBL(paths::texture + "HDRI\\cosmic\\");

        resource_manager.Add(-1, MakeAsset<Mesh>(Primitive::Sphere));
        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(03, MakeAsset<Shader>(paths::shader + "core\\light.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_01\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_01\\post_process.glsl"));
        resource_manager.Add(00, MakeAsset<CShader>(paths::shader + "core\\bloom.glsl"));
        resource_manager.Add(10, MakeAsset<CShader>(paths::shader + "scene_01\\cull.glsl"));
        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(13, MakeAsset<Material>(resource_manager.Get<Shader>(03)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));
        resource_manager.Add(98, MakeAsset<Sampler>(FilterMode::Point));
        resource_manager.Add(99, MakeAsset<Sampler>(FilterMode::Bilinear));

        // check errors periodically in case the built-in debug message callback fails
        Debug::CheckGLError(0);  // checkpoint 0

        // create uniform buffer objects (UBO) from shaders (duplicates will be skipped)
        AddUBO(resource_manager.Get<Shader>(02)->ID());
        AddUBO(resource_manager.Get<Shader>(03)->ID());
        AddUBO(resource_manager.Get<Shader>(04)->ID());

        Debug::CheckGLError(1);

        // create intermediate framebuffer objects (FBO)
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width / 2, Window::height / 2);

        FBOs[0].AddDepStTexture();
        FBOs[1].AddColorTexture(2, true);
        FBOs[1].AddDepStRenderBuffer(true);
        FBOs[2].AddColorTexture(2);
        FBOs[3].AddColorTexture(2);

        Debug::CheckGLError(2);

        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(vec3(0.0f, 6.0f, -16.0f));
        camera.GetComponent<Transform>().Rotate(world::up, 180.0f, Space::Local);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(vec3(1.0f, 0.553f, 0.0f), 3.8f);  // attach a flashlight
        camera.GetComponent<Spotlight>().SetCutoff(12.0f);
        
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(resource_manager.Get<Material>(12)); true) {
            mat.SetTexture(0, prefiltered_map);
            mat.BindUniform(0, &skybox_exposure);
            mat.BindUniform(1, &skybox_lod);
        }

        auto sphere_mesh = resource_manager.Get<Mesh>(-1);  // all spheres share the same mesh (VAO)

        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(sphere_mesh);
        sphere.GetComponent<Transform>().Translate(world::up * 8.5f);
        sphere.GetComponent<Transform>().Scale(2.0f);
        SetupMaterial(sphere.AddComponent<Material>(resource_manager.Get<Material>(14)), 0);

        plane = CreateEntity("Plane");
        plane.AddComponent<Mesh>(Primitive::Plane);
        plane.GetComponent<Transform>().Translate(world::down * 4.0f);
        plane.GetComponent<Transform>().Scale(3.0f);
        SetupMaterial(plane.AddComponent<Material>(resource_manager.Get<Material>(14)), 2);

        runestone = CreateEntity("Runestone");
        runestone.GetComponent<Transform>().Scale(0.02f);
        runestone.GetComponent<Transform>().Translate(world::down * 4.0f);

        if (std::string model_path = paths::model + "runestone\\runestone.fbx"; true) {
            auto& model = runestone.AddComponent<Model>(model_path, Quality::Auto);
            SetupMaterial(model.SetMaterial("pillars",  resource_manager.Get<Material>(14)), 31);
            SetupMaterial(model.SetMaterial("platform", resource_manager.Get<Material>(14)), 32);
        }

        Debug::CheckGLError(3);

        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(world::left, 45.0f, Space::Local);
        direct_light.AddComponent<DirectionLight>(color::white, 0.2f);

        // for static lights, we only need to set the uniform buffer once in `Init()`
        if (auto& ubo = UBOs[1]; true) {
            auto& dl = direct_light.GetComponent<DirectionLight>();
            auto& dt = direct_light.GetComponent<Transform>();
            ubo.SetUniform(0, val_ptr(dl.color));
            ubo.SetUniform(1, val_ptr(-dt.forward));
            ubo.SetUniform(2, val_ptr(dl.intensity));
        }

        orbit_light = CreateEntity("Orbit Light");
        orbit_light.GetComponent<Transform>().Translate(0.0f, 8.0f, 4.5f);
        orbit_light.GetComponent<Transform>().Scale(0.3f);
        orbit_light.AddComponent<PointLight>(color::lime, 0.8f);
        orbit_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);
        orbit_light.AddComponent<Mesh>(sphere_mesh);

        if (auto& mat = orbit_light.AddComponent<Material>(resource_manager.Get<Material>(13)); true) {
            auto& pl = orbit_light.GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 2.0f);  // bloom multiplier (color saturation)
        }

        Debug::CheckGLError(4);

        // make a grid of size 8x8 (64 cells), pick each border cell to be a point light (total = 28)
        for (int i = 0, index = 0; i < 64; i++) {
            int row = i / 8;  // ~ [0, 7]
            int col = i % 8;  // ~ [0, 7]
            if (bool on_border = row == 0 || row == 7 || col == 0 || col == 7; !on_border) {
                continue;  // skip cells in the middle
            }

            float ksi = math::RandomGenerator<float>();
            vec3 color = math::HSL2RGB(ksi, 0.7f + ksi * 0.3f, 0.4f + ksi * 0.2f);  // random bright color
            vec3 position = vec3(row - 3.5f, 1.5f, col - 3.5f) * 9.0f;

            auto& pl = point_lights[index];
            pl = CreateEntity("Point Light " + std::to_string(index));
            pl.GetComponent<Transform>().Translate(position - world::origin);
            pl.GetComponent<Transform>().Scale(0.8f);
            pl.AddComponent<PointLight>(color, 1.5f);
            pl.AddComponent<Mesh>(sphere_mesh);

            auto& mat = pl.AddComponent<Material>(resource_manager.Get<Material>(13));
            auto& ppl = pl.GetComponent<PointLight>();

            ppl.SetAttenuation(0.09f, 0.032f);
            mat.BindUniform(3, &ppl.color);
            mat.SetUniform(4, ppl.intensity);
            mat.SetUniform(5, 7.0f);  // bloom multiplier (color saturation)
            index++;
        }

        SetupPLBuffers();  // setup shader storage buffers for our tiled forward renderer
        Debug::CheckGLError(5);

        Renderer::FaceCulling(true);
        Renderer::AlphaBlend(false);
        Renderer::SeamlessCubemap(true);
    }

    // this is called every frame, update your scene here and submit entities to the renderer
    void Scene01::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        if (auto& ubo = UBOs[0]; true) {
            ubo.SetUniform(0, val_ptr(main_camera.T->position));
            ubo.SetUniform(1, val_ptr(main_camera.T->forward));
            ubo.SetUniform(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetUniform(3, val_ptr(main_camera.GetProjectionMatrix()));
        }

        if (auto& ubo = UBOs[2]; true) {
            auto& spotlight = camera.GetComponent<Spotlight>();
            auto& transform = camera.GetComponent<Transform>();
            float inner_cos = spotlight.GetInnerCosine();
            float outer_cos = spotlight.GetOuterCosine();

            ubo.SetUniform(0, val_ptr(spotlight.color));
            ubo.SetUniform(1, val_ptr(transform.position));
            ubo.SetUniform(2, val_ptr(-transform.forward));
            ubo.SetUniform(3, val_ptr(spotlight.intensity));
            ubo.SetUniform(4, val_ptr(inner_cos));
            ubo.SetUniform(5, val_ptr(outer_cos));
            ubo.SetUniform(6, val_ptr(spotlight.range));
        }

        if (auto& ubo = UBOs[3]; true) {
            auto& ot = orbit_light.GetComponent<Transform>();
            auto& ol = orbit_light.GetComponent<PointLight>();

            ubo.SetUniform(0, val_ptr(ol.color));
            ubo.SetUniform(1, val_ptr(ot.position));
            ubo.SetUniform(2, val_ptr(ol.intensity));
            ubo.SetUniform(3, val_ptr(ol.linear));
            ubo.SetUniform(4, val_ptr(ol.quadratic));
            ubo.SetUniform(5, val_ptr(ol.range));
        }

        if (auto& ubo = UBOs[4]; true) {
            auto& pl = point_lights[0].GetComponent<PointLight>();
            ubo.SetUniform(0, val_ptr(light_cluster_intensity));
            ubo.SetUniform(1, val_ptr(pl.linear));
            ubo.SetUniform(2, val_ptr(pl.quadratic));
        }

        if (orbit) {
            orbit_light.GetComponent<Transform>().Rotate(world::up, orbit_speed, Space::World);
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];
        FBO& framebuffer_2 = FBOs[2];
        FBO& framebuffer_3 = FBOs[3];

        // ------------------------------ depth prepass ------------------------------

        framebuffer_0.Bind();
        framebuffer_0.Clear(-1);

        Renderer::DepthTest(true);
        Renderer::DepthPrepass(true);  // enable early z-test

        // light sources are shaded independently so we don't need to render them in the prepass
        Renderer::Submit(sphere.id, runestone.id);
        Renderer::Submit(show_plane ? plane.id : entt::null);
        Renderer::Submit(skybox.id);
        Renderer::Render();
        Renderer::DepthPrepass(false);

        if (draw_depth_buffer) {  // visualize the depth buffer and return early
            Renderer::DepthTest(false);
            framebuffer_0.Unbind();  // return early to the default framebuffer
            Renderer::Clear();
            framebuffer_0.Draw(-1);
            return;
        }

        // ------------------------------ dispatch light culling ------------------------------

        framebuffer_0.GetDepthTexture().Bind(0);
        pl_index->Clear();

        auto cull_shader = resource_manager.Get<CShader>(10);
        cull_shader->SetUniform(0, n_pls);

        // in this pass we only dispatch the compute shader to update the light indices SSBO
        // note that `SyncWait()` should ideally be placed closest to the code that actually uses
        // the compute shader's output so as to avoid unnecessary waits when computation is heavy

        cull_shader->Bind();
        cull_shader->Dispatch(nx, ny, 1);
        cull_shader->SyncWait();
        cull_shader->Unbind();

        // ------------------------------ MRT render pass ------------------------------

        // this is the actual shading pass after light culling, now that we know the indices of all
        // visible lights that will contribute to each tile we no longer need to loop through every
        // light in the fragment shader. In this pass, we still have the geometry data of entities
        // in the scene so MSAA will work fine, after this, we will no longer be able to apply MSAA

        framebuffer_1.Clear();
        framebuffer_1.Bind();

        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::Submit(sphere.id, runestone.id);
        Renderer::Submit(show_plane ? plane.id : entt::null);

        if (show_light_cluster) {
            for (int i = 0; i < n_pls; ++i) {
                Renderer::Submit(point_lights[i].id);
            }
        }

        Renderer::Submit(orbit_light.id);
        Renderer::Submit(skybox.id);
        Renderer::Render();

        framebuffer_1.Unbind();

        // ------------------------------ MSAA resolve pass ------------------------------

        framebuffer_2.Clear();
        FBO::CopyColor(framebuffer_1, 0, framebuffer_2, 0);
        FBO::CopyColor(framebuffer_1, 1, framebuffer_2, 1);

        // ------------------------------ apply Gaussian blur ------------------------------

        FBO::CopyColor(framebuffer_2, 1, framebuffer_3, 0);  // downsample the bloom target (nearest filtering)
        auto& ping = framebuffer_3.GetColorTexture(0);
        auto& pong = framebuffer_3.GetColorTexture(1);
        auto bloom_shader = resource_manager.Get<CShader>(00);

        bloom_shader->Bind();
        ping.BindILS(0, 0, GL_READ_WRITE);
        pong.BindILS(0, 1, GL_READ_WRITE);

        for (int i = 0; i < 2 * n_blurs; ++i) {
            bloom_shader->SetUniform(0, i % 2 == 0);
            bloom_shader->Dispatch(ping.width / 32, ping.width / 18);
            bloom_shader->SyncWait(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        }

        // ------------------------------ postprocessing pass ------------------------------

        framebuffer_2.GetColorTexture(0).Bind(0);  // color texture
        framebuffer_3.GetColorTexture(0).Bind(1);  // bloom texture

        auto bilinear_sampler = resource_manager.Get<Sampler>(99);
        bilinear_sampler->Bind(1);  // upsample the bloom texture (bilinear filtering)

        auto postprocess_shader = resource_manager.Get<Shader>(05);
        postprocess_shader->Bind();
        postprocess_shader->SetUniform(0, tone_mapping_mode);

        Renderer::Clear();
        Mesh::DrawQuad();

        postprocess_shader->Unbind();
        bilinear_sampler->Unbind(1);
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        using namespace ImGui;
        const ImVec4 text_color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_NoPicker;

        const char tone_mapping_guide[] = "Select a tone mapping operator below.";
        const char tone_mapping_note[] = "Approximated ACES filmic tonemapper might not be "
            "the most visually attractive one, but it is the most physically correct.";
        const char* tone_mappings[] = {
            "Simple Reinhard", "Reinhard-Jodie (Shadertoy)", "Uncharted 2 Hable Filmic", "Approximated ACES (UE4)"
        };

        static bool show_sphere_gizmo     = false;
        static bool show_plane_gizmo      = false;
        static bool edit_sphere_albedo    = false;
        static bool edit_flashlight_color = false;

        if (ui::NewInspector() && BeginTabBar("InspectorTab", ImGuiTabBarFlags_None)) {
            Indent(5.0f);

            if (BeginTabItem("Scene")) {
                Checkbox("Show Plane", &show_plane);                    Separator();
                Checkbox("Show Light Cluster", &show_light_cluster);    Separator();
                Checkbox("Orbit Light", &orbit);                        Separator();
                Checkbox("Show Sphere Gizmo", &show_sphere_gizmo);      Separator();
                Checkbox("Show Plane Gizmo", &show_plane_gizmo);        Separator();
                Checkbox("Visualize Depth Buffer", &draw_depth_buffer); Separator();

                PushItemWidth(130.0f);
                SliderFloat("Light Cluster Intensity", &light_cluster_intensity, 3.0f, 20.0f); Separator();
                SliderFloat("Skybox Exposure", &skybox_exposure, 1.2f, 8.0f); Separator();
                SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f); Separator();
                PopItemWidth();
                Spacing();

                if (Button("New Colors", ImVec2(130.0f, 0.0f))) {
                    UpdatePLColors();
                }

                SameLine(0.0f, 10.0f);
                Text("Refresh Cluster RGB");
                if (IsItemHovered()) {
                    SetTooltip("New colors will be created at random.");
                }

                Spacing();
                Separator();
                EndTabItem();
            }

            if (BeginTabItem("Material")) {
                PushItemWidth(100.0f);
                SliderFloat("Sphere Metalness", &sphere_metalness, 0.05f, 1.0f);
                SliderFloat("Sphere Roughness", &sphere_roughness, 0.05f, 1.0f);
                SliderFloat("Sphere AO", &sphere_ao, 0.0f, 1.0f);
                SliderFloat("Plane Roughness", &plane_roughness, 0.1f, 0.3f);
                PopItemWidth();
                Separator();

                Checkbox("Edit Sphere Albedo", &edit_sphere_albedo);
                if (edit_sphere_albedo) {
                    Spacing();
                    Indent(10.0f);
                    ColorPicker3("##Sphere Albedo", val_ptr(sphere_albedo), color_flags);
                    Unindent(10.0f);
                }

                Checkbox("Edit Flashlight Color", &edit_flashlight_color);
                if (edit_flashlight_color) {
                    Spacing();
                    Indent(10.0f);
                    ColorPicker3("##Flash", val_ptr(camera.GetComponent<Spotlight>().color), color_flags);
                    Unindent(10.0f);
                }
                EndTabItem();
            }

            if (BeginTabItem("HDR/Bloom")) {
                PushItemWidth(180.0f);
                Text("Bloom Effect Multiplier (Light Cluster)");
                SliderInt("##Bloom", &n_blurs, 3, 5);
                PopItemWidth();
                Separator();
                PushStyleColor(ImGuiCol_Text, text_color);
                PushTextWrapPos(280.0f);
                TextWrapped(tone_mapping_guide);
                TextWrapped(tone_mapping_note);
                PopTextWrapPos();
                PopStyleColor();
                Separator();
                PushItemWidth(295.0f);
                Combo(" ", &tone_mapping_mode, tone_mappings, 4);
                PopItemWidth();
                EndTabItem();
            }

            EndTabBar();
            Unindent(5.0f);
            ui::EndInspector();
        }

        if (show_sphere_gizmo) {
            ui::DrawGizmo(camera, sphere, ui::Gizmo::Translate);
        }

        if (show_plane_gizmo && show_plane) {
            ui::DrawGizmo(camera, plane, ui::Gizmo::Translate);
        }
    }

    void Scene01::UpdatePLColors() {
        auto color_ptr = reinterpret_cast<vec4* const>(pl_color->Data());
        for (int i = 0; i < n_pls; ++i) {
            auto hue = math::RandomGenerator<float>();
            auto color = math::HSV2RGB(vec3(hue, 1.0f, 1.0f));
            color_ptr[i] = vec4(color, 1.0f);
            point_lights[i].GetComponent<PointLight>().color = color;
        }
    }

    void Scene01::PrecomputeIBL(const std::string& hdri) {
        Renderer::SeamlessCubemap(true);
        Renderer::DepthTest(false);
        Renderer::FaceCulling(true);

        auto irradiance_shader = CShader(paths::shader + "core\\irradiance_map.glsl");
        auto prefilter_shader  = CShader(paths::shader + "core\\prefilter_envmap.glsl");
        auto envBRDF_shader    = CShader(paths::shader + "core\\environment_BRDF.glsl");

        std::string rootpath = utils::paths::root;
        if (rootpath.find("mashiro") == std::string::npos) {
            irradiance_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
            prefiltered_map = MakeAsset<Texture>(paths::texture + "HDRI\\cosmic\\", ".hdr", 2048, 8);
            Texture::Copy(*prefiltered_map, 4, *irradiance_map, 0);
            BRDF_LUT = MakeAsset<Texture>(paths::texture + "common\\checkboard.png", 1);
            Sync::WaitFinish();
            return;
        }

        auto env_map = MakeAsset<Texture>(hdri, ".hdr", 2048, 0);
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

    void Scene01::SetupMaterial(Material& pbr_mat, int mat_id) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        if (mat_id == 0) {  // sphere
            pbr_mat.BindUniform(pbr_u::albedo, &sphere_albedo);
            pbr_mat.BindUniform(pbr_u::metalness, &sphere_metalness);
            pbr_mat.BindUniform(pbr_u::roughness, &sphere_roughness);
            pbr_mat.BindUniform(pbr_u::ao, &sphere_ao);
        }
        else if (mat_id == 2) {  // plane
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::texture + "common\\checkboard.png"));
            pbr_mat.SetUniform(pbr_u::metalness, 0.1f);
            pbr_mat.BindUniform(pbr_u::roughness, &plane_roughness);
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(8.0f));
        }
        else if (mat_id == 31) {  // runestone pillars
            std::string tex_path = paths::model + "runestone\\";
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "pillars_albedo.png"));
            pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(tex_path + "pillars_normal.png"));
            pbr_mat.SetTexture(pbr_t::metallic,  MakeAsset<Texture>(tex_path + "pillars_metallic.png"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "pillars_roughness.png"));
        }
        else if (mat_id == 32) {  // runestone platform
            std::string tex_path = paths::model + "runestone\\";
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "platform_albedo.png"));
            pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(tex_path + "platform_normal.png"));
            pbr_mat.SetTexture(pbr_t::metallic,  MakeAsset<Texture>(tex_path + "platform_metallic.png"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "platform_roughness.png"));
            pbr_mat.SetTexture(pbr_t::emission,  MakeAsset<Texture>(tex_path + "platform_emissive.png"));
        }
    }

    void Scene01::SetupPLBuffers() {
        nx = (Window::width  + tile_size - 1) / tile_size;
        ny = (Window::height + tile_size - 1) / tile_size;
        GLuint n_tiles = nx * ny;

        // for our tiled forward renderer we need 4 SSBOs in total: color, position and range
        // are read-only in GLSL shaders, they're supposed to be updated on the CPU side from
        // time to time so we will keep a pointer to the data store using coherent persistent
        // mapping. The pointer can be acquired only once, there's no need to release it, and
        // all writes are automatically flushed to the GPU. On the other hand, the index SSBO
        // is updated by the compute shader to hold indices of lights that are not culled, it
        // should not be visible to the CPU, we don't need a map so use a dynamic storage bit

        GLbitfield CPU_access = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
        GLbitfield GPU_access = GL_DYNAMIC_STORAGE_BIT;

        pl_color    = WrapAsset<SSBO>(0, n_pls * sizeof(vec4), CPU_access);
        pl_position = WrapAsset<SSBO>(1, n_pls * sizeof(vec4), CPU_access);
        pl_range    = WrapAsset<SSBO>(2, n_pls * sizeof(float), CPU_access);
        pl_index    = WrapAsset<SSBO>(3, n_pls * n_tiles * sizeof(int), GPU_access);

        // light culling in a tiled forward renderer can apply to both static and dynamic
        // lights, the latter case requires users to update the SSBO buffers every frame.
        // for simplicity, we'll only perform light culling on the 28 static point lights
        // since that's enough for most cases, so the SSBO only needs to be set up once.

        pl_position->Acquire(CPU_access);
        pl_color->Acquire(CPU_access);
        pl_range->Acquire(CPU_access);

        auto* const posit_ptr = reinterpret_cast<vec4* const>(pl_position->Data());
        auto* const color_ptr = reinterpret_cast<vec4* const>(pl_color->Data());
        auto* const range_ptr = reinterpret_cast<float* const>(pl_range->Data());

        for (int i = 0; i < n_pls; i++) {
            auto& pt = point_lights[i].GetComponent<Transform>();
            auto& pl = point_lights[i].GetComponent<PointLight>();

            posit_ptr[i] = vec4(pt.position, 1.0f);
            color_ptr[i] = vec4(pl.color, 1.0f);
            range_ptr[i] = pl.range;
        }
    }

}
