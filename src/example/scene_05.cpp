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
#include "example/scene_05.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    static bool  show_grid       = false;
    static float grid_cell_size  = 2.0f;
    static vec4  thin_line_color = vec4(0.1f, 0.1f, 0.1f, 1.0f);
    static vec4  wide_line_color = vec4(0.2f, 0.2f, 0.2f, 1.0f);

    static float skybox_exposure = 1.0f;
    static float skybox_lod      = 0.0f;

    static int   tab_id           = 0;
    static bool  bounce_ball      = false;
    static float bounce_time      = 0.0f;
    static bool  enable_spotlight = false;
    static bool  enable_moonlight = false;
    static bool  enable_lantern   = false;
    static bool  enable_shadow    = false;
    static bool  animate_suzune   = false;
    static float animate_speed    = 1.0f;
    static float light_radius     = 0.001f;
    static float lantern_radius   = 0.001f;

    constexpr uint shadow_width   = 2048;
    constexpr uint shadow_height  = 2048;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene05::Init() {
        this->title = "Animation and Realtime Shadows";
        PrecomputeIBL(paths::texture + "HDRI\\moonlit_sky2.hdr");

        resource_manager.Add(00, MakeAsset<CShader>(paths::shader + "core\\bloom.glsl"));
        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(03, MakeAsset<Shader>(paths::shader + "core\\light.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_05\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_05\\post_process.glsl"));
        resource_manager.Add(06, MakeAsset<Shader>(paths::shader + "scene_05\\shadow.glsl"));
        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(13, MakeAsset<Material>(resource_manager.Get<Shader>(03)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));
        resource_manager.Add(98, MakeAsset<Sampler>(FilterMode::Point));
        resource_manager.Add(99, MakeAsset<Sampler>(FilterMode::Bilinear));
        
        AddUBO(resource_manager.Get<Shader>(02)->ID());
        AddUBO(resource_manager.Get<Shader>(03)->ID());
        AddUBO(resource_manager.Get<Shader>(04)->ID());

        AddFBO(shadow_width, shadow_height);
        AddFBO(shadow_width, shadow_height);
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);
        AddFBO(Window::width / 2, Window::height / 2);

        FBOs[0].AddDepthCubemap();
        FBOs[1].AddDepthCubemap();
        FBOs[2].AddColorTexture(2, true);    // multisampled textures for MSAA
        FBOs[2].AddDepStRenderBuffer(true);  // multisampled RBO for MSAA
        FBOs[3].AddColorTexture(2);
        FBOs[4].AddColorTexture(2);

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

        moonlight = CreateEntity("Moonlight");
        moonlight.GetComponent<Transform>().Rotate(-45.0f, 0.0f, 0.0f, Space::World);
        moonlight.AddComponent<DirectionLight>(vec3(0.0f, 0.43f, 1.0f), 0.5f);

        // moonlight is static so we only need to set the uniform buffer once in `Init()`
        if (auto& ubo = UBOs[3]; true) {
            auto& dl = moonlight.GetComponent<DirectionLight>();
            auto& dt = moonlight.GetComponent<Transform>();

            vec4 R = vec4(dt.right, 0.0f);
            vec4 F = vec4(dt.forward, 0.0f);
            vec4 U = vec4(dt.up, 0.0f);
            vec4 directions[] = { -F, -U, R, -R, vec4(world::backward, 0.0f) };

            ubo.SetUniform(0, val_ptr(dl.color));
            ubo.SetUniform(1, directions);
            ubo.SetUniform(2, val_ptr(dl.intensity));
        }

        point_light = CreateEntity("Point Light");
        point_light.AddComponent<Mesh>(Primitive::Cube);
        point_light.GetComponent<Transform>().Translate(world::up * 6.0f);
        point_light.GetComponent<Transform>().Translate(world::forward * -4.0f);
        point_light.GetComponent<Transform>().Scale(0.05f);
        point_light.AddComponent<PointLight>(color::orange, 3.8f);
        point_light.GetComponent<PointLight>().SetAttenuation(0.03f, 0.015f);

        if (auto& mat = point_light.AddComponent<Material>(resource_manager.Get<Material>(13)); true) {
            auto& pl = point_light.GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        lantern = CreateEntity("Lantern");
        lantern.AddComponent<Mesh>(Primitive::Sphere);
        lantern.GetComponent<Transform>().Translate(-2.0f, 6.0f, 7.0f);
        lantern.GetComponent<Transform>().Scale(0.5f);
        lantern.AddComponent<PointLight>(color::white, 4.8f);
        lantern.GetComponent<PointLight>().SetAttenuation(0.03f, 0.015f);

        if (auto& mat = lantern.AddComponent<Material>(resource_manager.Get<Material>(13)); true) {
            auto& pl = lantern.GetComponent<PointLight>();
            mat.BindUniform(3, &pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 3.0f);
        }

        spotlight = CreateEntity("Spotlight");
        spotlight.AddComponent<Mesh>(Primitive::Tetrahedron);
        spotlight.GetComponent<Transform>().Translate(vec3(0.0f, 10.0f, -7.0f));
        spotlight.GetComponent<Transform>().Scale(0.1f);
        spotlight.AddComponent<Spotlight>(color::white, 13.8f);
        spotlight.GetComponent<Spotlight>().SetCutoff(20.0f, 10.0f, 45.0f);

        if (auto& mat = spotlight.AddComponent<Material>(resource_manager.Get<Material>(13)); true) {
            auto& sl = spotlight.GetComponent<Spotlight>();
            mat.SetUniform(3, sl.color);
            mat.SetUniform(4, sl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        floor = CreateEntity("Floor");
        floor.AddComponent<Mesh>(Primitive::Plane);
        floor.GetComponent<Transform>().Translate(0.0f, -1.05f, 0.0f);
        floor.GetComponent<Transform>().Scale(20.0f);
        SetupMaterial(floor.AddComponent<Material>(resource_manager.Get<Material>(14)), 0);

        wall = CreateEntity("Wall");
        wall.AddComponent<Mesh>(Primitive::Cube);
        wall.GetComponent<Transform>().Translate(0.0f, 5.0f, -8.0f);
        wall.GetComponent<Transform>().Scale(12.0f, 6.0f, 0.25f);
        SetupMaterial(wall.AddComponent<Material>(resource_manager.Get<Material>(14)), 1);

        for (int i = 0; i < 3; i++) {
            ball[i] = CreateEntity("Sphere " + std::to_string(i));
            ball[i].GetComponent<Transform>().Translate(vec3(-i * 4, (i + 1) * 2, pow(-1, i) - 4));
            ball[i].AddComponent<Mesh>(Primitive::Sphere);
            SetupMaterial(ball[i].AddComponent<Material>(resource_manager.Get<Material>(14)), i + 2);
        }

        suzune = CreateEntity("Nekomimi Suzune");
        suzune.GetComponent<Transform>().Translate(vec3(0.0f, -0.9f, -3.0f));
        suzune.GetComponent<Transform>().Scale(0.05f);

        if (std::string model_path = paths::model + "suzune\\suzune.fbx"; true) {
            auto& model = suzune.AddComponent<Model>(model_path, Quality::Auto, true);
            model.AttachMotion(model_path);
            auto& animator = suzune.AddComponent<Animator>();
            animator.Update(suzune.GetComponent<Model>(), Clock::delta_time);

            SetupMaterial(model.SetMaterial("mat_Suzune_Hair.001", resource_manager.Get<Material>(14)), 50);
            SetupMaterial(model.SetMaterial("mat_Suzune_Body.001", resource_manager.Get<Material>(14)), 51);
            SetupMaterial(model.SetMaterial("mat_Suzune_Cloth.001",resource_manager.Get<Material>(14)), 52);
            SetupMaterial(model.SetMaterial("mat_Suzune_Head.001", resource_manager.Get<Material>(14)), 53);
            SetupMaterial(model.SetMaterial("mat_Suzune_EyeL.001", resource_manager.Get<Material>(14)), 54);
            SetupMaterial(model.SetMaterial("mat_Suzune_EyeR.001", resource_manager.Get<Material>(14)), 55);
        }

        const std::vector<int> pillar_id { 8, 9, 10, 18, 20, 21, 29, 30 };

        for (int i = 0; i < pillar_id.size(); i++) {
            int id = pillar_id[i];
            int row = i / 4;
            int col = i % 4;
            pillars[i] = CreateEntity("Pillar " + std::to_string(i));
            pillars[i].GetComponent<Transform>().Translate((col - 1.5f) * 6.0f, -0.9f, row * 6.5f - 7.5f);

            auto file = (id >= 10 ? "cloumn_" : "cloumn_0") + std::to_string(id) + ".obj";
            auto& model = pillars[i].AddComponent<Model>(paths::model + "pillars\\" + file, Quality::Auto);
            SetupMaterial(model.SetMaterial("initialShadingGroup", resource_manager.Get<Material>(14)), 70);
        }

        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::AlphaBlend(true);
        Renderer::FaceCulling(true);
    }

    void Scene05::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        if (auto& ubo = UBOs[0]; true) {
            ubo.SetUniform(0, val_ptr(main_camera.T->position));
            ubo.SetUniform(1, val_ptr(main_camera.T->forward));
            ubo.SetUniform(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetUniform(3, val_ptr(main_camera.GetProjectionMatrix()));
        }

        if (auto& ubo = UBOs[1]; true) {
            auto& pl_1 = point_light.GetComponent<PointLight>();
            auto& pt_1 = point_light.GetComponent<Transform>();
            auto& pl_2 = lantern.GetComponent<PointLight>();
            auto& pt_2 = lantern.GetComponent<Transform>();

            // for uniform arrays in std140, every element is padded to a vec4 (16 bytes)
            vec4 color[]      = { vec4(pl_1.color, 0),    vec4(pl_2.color, 0) };
            vec4 position[]   = { vec4(pt_1.position, 0), vec4(pt_2.position, 0) };
            vec4 intensity[]  = { vec4(pl_1.intensity),   vec4(pl_2.intensity) };
            vec4 linear[]     = { vec4(pl_1.linear),      vec4(pl_2.linear) };
            vec4 quadratic[]  = { vec4(pl_1.quadratic),   vec4(pl_2.quadratic) };
            vec4 range[]      = { vec4(pl_1.range),       vec4(pl_2.range) };

            ubo.SetUniform(0, color);
            ubo.SetUniform(1, position);
            ubo.SetUniform(2, intensity);
            ubo.SetUniform(3, linear);
            ubo.SetUniform(4, quadratic);
            ubo.SetUniform(5, range);
        }

        if (auto& ubo = UBOs[2]; true) {
            auto& sl = spotlight.GetComponent<Spotlight>();
            auto& st = spotlight.GetComponent<Transform>();
            float inner_cos = sl.GetInnerCosine();
            float outer_cos = sl.GetOuterCosine();
            ubo.SetUniform(0, val_ptr(sl.color));
            ubo.SetUniform(1, val_ptr(st.position));
            ubo.SetUniform(2, val_ptr(st.up));
            ubo.SetUniform(3, val_ptr(sl.intensity));
            ubo.SetUniform(4, val_ptr(inner_cos));
            ubo.SetUniform(5, val_ptr(outer_cos));
            ubo.SetUniform(6, val_ptr(sl.range));
        }

        // update entities
        if (tab_id == 0 && bounce_ball) {
            // simulate balls gravity using a cheap quadratic easing factor
            bounce_time += Clock::delta_time;
            for (int i = 0; i < 3; ++i) {
                float h = (i + 1) * 2.0f;                       // initial height of the ball
                float s = 2.0f - i * 0.4f;                      // falling and bouncing speed
                float t = math::Bounce(bounce_time * s, 1.0f);  // bounce between 0.0 and 1.0
                float y = math::Lerp(h, -0.05f, t * t);         // ease in, slow near 0 and fast near 1
                auto& T = ball[i].GetComponent<Transform>();
                T.SetPosition(vec3(T.position.x, y, T.position.z));
            }
        }
        else if (tab_id == 1 && animate_suzune) {
            auto& animator = suzune.GetComponent<Animator>();
            auto& model = suzune.GetComponent<Model>();
            animator.Update(model, Clock::delta_time * animate_speed);
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];
        FBO& framebuffer_2 = FBOs[2];
        FBO& framebuffer_3 = FBOs[3];
        FBO& framebuffer_4 = FBOs[4];

        // ------------------------------ shadow pass 1 ------------------------------

        Renderer::SetViewport(shadow_width, shadow_height);
        Renderer::SetShadowPass(1);
        framebuffer_0.Clear(-1);
        framebuffer_0.Bind();
        auto shadow_shader = resource_manager.Get<Shader>(06);

        if (tab_id == 1) {
            auto& bone_transforms = suzune.GetComponent<Animator>().bone_transforms;
            for (size_t i = 0; i < bone_transforms.size(); ++i) {
                shadow_shader->SetUniform(100 + i, bone_transforms[i]);
            }
        }

        float& near_clip = main_camera.near_clip;
        float& far_clip = main_camera.far_clip;
        mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, near_clip, far_clip);

        auto& pl_t = point_light.GetComponent<Transform>();
        std::vector<mat4> pl_transform = {
            projection * pl_t.GetLocalTransform(world::right,    world::down),
            projection * pl_t.GetLocalTransform(world::left,     world::down),
            projection * pl_t.GetLocalTransform(world::up,       world::backward),
            projection * pl_t.GetLocalTransform(world::down,     world::forward),
            projection * pl_t.GetLocalTransform(world::backward, world::down),
            projection * pl_t.GetLocalTransform(world::forward,  world::down)
        };

        shadow_shader->SetUniformArray(250, 6, pl_transform);

        if (tab_id == 0) {
            Renderer::Submit(ball[0].id, ball[1].id, ball[2].id);
        }
        else if (tab_id == 1) {
            Renderer::Submit(suzune.id, wall.id);
        }
        else if (tab_id == 2) {
            for (int i = 0; i < 8; ++i) {
                Renderer::Submit(pillars[i].id);
            }
        }

        Renderer::Submit(floor.id);
        Renderer::Render(shadow_shader);
        Renderer::SetViewport(Window::width, Window::height);
        Renderer::SetShadowPass(0);

        // ------------------------------ shadow pass 2 (optional) ------------------------------

        if (tab_id == 2) {
            Renderer::SetViewport(shadow_width, shadow_height);
            Renderer::SetShadowPass(2);
            framebuffer_1.Clear(-1);
            framebuffer_1.Bind();

            auto& lt_t = lantern.GetComponent<Transform>();
            std::vector<mat4> lt_transform = {
                projection * lt_t.GetLocalTransform(world::right,    world::down),
                projection * lt_t.GetLocalTransform(world::left,     world::down),
                projection * lt_t.GetLocalTransform(world::up,       world::backward),
                projection * lt_t.GetLocalTransform(world::down,     world::forward),
                projection * lt_t.GetLocalTransform(world::backward, world::down),
                projection * lt_t.GetLocalTransform(world::forward,  world::down)
            };

            shadow_shader->SetUniformArray(256, 6, lt_transform);

            for (int i = 0; i < 8; ++i) {
                Renderer::Submit(pillars[i].id);
            }

            Renderer::Submit(floor.id);
            Renderer::Render(shadow_shader);
            Renderer::SetViewport(Window::width, Window::height);
            Renderer::SetShadowPass(0);
        }

        // ------------------------------ MRT render pass ------------------------------

        framebuffer_0.GetDepthTexture().Bind(15);
        framebuffer_1.GetDepthTexture().Bind(16);
        framebuffer_2.Clear();
        framebuffer_2.Bind();

        if (tab_id == 0) {
            Renderer::Submit(floor.id);
            Renderer::Submit(point_light.id);
            Renderer::Submit(skybox.id);
            Renderer::Submit(ball[0].id, ball[1].id, ball[2].id);
        }
        else if (tab_id == 1) {
            Renderer::FaceCulling(false);
            Renderer::Submit(suzune.id);
            Renderer::Render();
            Renderer::FaceCulling(true);

            Renderer::Submit(floor.id);
            Renderer::Submit(wall.id);
            Renderer::Submit(point_light.id);
            Renderer::Submit(spotlight.id);
            Renderer::Submit(skybox.id);
        }
        else if (tab_id == 2) {
            Renderer::Submit(point_light.id);
            Renderer::Submit(lantern.id);
            for (int i = 0; i < 8; ++i) {
                Renderer::Submit(pillars[i].id);
            }
            Renderer::Submit(floor.id);
            Renderer::Submit(skybox.id);
        }

        Renderer::Render();

        if (show_grid) {
            auto grid_shader = resource_manager.Get<Shader>(01);
            grid_shader->Bind();
            grid_shader->SetUniform(0, grid_cell_size);
            grid_shader->SetUniform(1, thin_line_color);
            grid_shader->SetUniform(2, wide_line_color);
            Mesh::DrawGrid();
        }

        framebuffer_2.Unbind();

        // ------------------------------ MSAA resolve pass ------------------------------
        
        framebuffer_3.Clear();
        FBO::CopyColor(framebuffer_2, 0, framebuffer_3, 0);
        FBO::CopyColor(framebuffer_2, 1, framebuffer_3, 1);

        // ------------------------------ apply Gaussian blur ------------------------------

        FBO::CopyColor(framebuffer_3, 1, framebuffer_4, 0);  // downsample the bloom target (nearest filtering)
        auto& ping = framebuffer_4.GetColorTexture(0);
        auto& pong = framebuffer_4.GetColorTexture(1);
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

        framebuffer_3.GetColorTexture(0).Bind(0);  // color texture
        framebuffer_4.GetColorTexture(0).Bind(1);  // bloom texture

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

    void Scene05::OnImGuiRender() {
        using namespace ImGui;
        const ImVec4 tab_color_off = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
        const ImVec4 tab_color_on = ImVec4(0.0f, 0.4f, 0.8f, 1.0f);
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        static bool show_gizmo_pl = false;
        static bool show_gizmo_sl = false;
        static bool show_gizmo_lt = false;
        static vec3 lantern_color = color::white;

        if (ui::NewInspector()) {
            Indent(5.0f);
            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.5f, 4.0f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Separator();

            BeginTabBar("InspectorTab", ImGuiTabBarFlags_None);

            if (BeginTabItem("Bouncing Ball")) {
                tab_id = 0;
                enable_spotlight = enable_moonlight = enable_lantern = false;
                show_gizmo_sl = show_gizmo_lt = false;
                PushItemWidth(130.0f);
                Checkbox("Enable Shadow", &enable_shadow);
                Checkbox("Show Gizmo PL", &show_gizmo_pl);
                Checkbox("Ball Bounce", &bounce_ball);
                SliderFloat("Light Radius", &light_radius, 0.001f, 0.1f);
                PopItemWidth();
                EndTabItem();
            }

            if (BeginTabItem("Nekomimi")) {
                tab_id = 1;
                enable_spotlight = true;
                enable_lantern = show_gizmo_lt = false;
                PushItemWidth(130.0f);
                Checkbox("Enable Shadow", &enable_shadow);
                Checkbox("Enable Moonlight", &enable_moonlight);
                Checkbox("Show Gizmo PL", &show_gizmo_pl);
                Checkbox("Show Gizmo SL", &show_gizmo_sl);
                if (show_gizmo_pl && show_gizmo_sl) { show_gizmo_pl = false; }
                Checkbox("Play Animation", &animate_suzune);
                SliderFloat("Animation Speed", &animate_speed, 0.1f, 3.0f);
                SliderFloat("Light Radius", &light_radius, 0.001f, 0.1f);
                PopItemWidth();
                EndTabItem();
            }

            if (BeginTabItem("Pillars")) {
                tab_id = 2;
                enable_lantern = true;
                enable_spotlight = enable_moonlight = show_gizmo_sl = false;
                PushItemWidth(130.0f);
                Checkbox("Enable Shadow", &enable_shadow);
                Checkbox("Gizmo PL", &show_gizmo_pl);
                Checkbox("Gizmo Lantern", &show_gizmo_lt);
                if (show_gizmo_pl && show_gizmo_lt) { show_gizmo_pl = false; }
                if (ColorEdit3("Lantern Color", val_ptr(lantern_color), color_flags)) {
                    lantern.GetComponent<PointLight>().color = lantern_color;
                }
                SliderFloat("Light Radius", &light_radius, 0.001f, 0.1f);
                SliderFloat("Lantern Radius", &lantern_radius, 0.001f, 0.1f);
                PopItemWidth();
                EndTabItem();
            }

            if (show_gizmo_pl) { ui::DrawGizmo(camera, point_light, ui::Gizmo::Translate); }
            if (show_gizmo_sl) { ui::DrawGizmo(camera, spotlight, ui::Gizmo::Translate); }
            if (show_gizmo_lt) { ui::DrawGizmo(camera, lantern, ui::Gizmo::Translate); }
            
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

    void Scene05::PrecomputeIBL(const std::string& hdri) {
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

    void Scene05::SetupMaterial(Material& pbr_mat, int mat_id) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        pbr_mat.BindUniform(0, &skybox_exposure);
        pbr_mat.BindUniform(1, &enable_spotlight);
        pbr_mat.BindUniform(2, &enable_moonlight);
        pbr_mat.BindUniform(3, &enable_lantern);
        pbr_mat.BindUniform(4, &enable_shadow);
        pbr_mat.BindUniform(5, &light_radius);
        pbr_mat.BindUniform(6, &lantern_radius);

        if (mat_id == 0) {  // floor
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(32.0f));
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::texture + "wood_parquet_15-1K\\albedo.jpg"));
            pbr_mat.SetTexture(pbr_t::normal, MakeAsset<Texture>(paths::texture + "wood_parquet_15-1K\\normal.jpg"));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.37f);
        }
        else if (mat_id == 1) {  // wall
            pbr_mat.SetUniform(pbr_u::albedo, vec4(1.0f, 0.6f, 1.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.37f);
        }
        else if (mat_id == 2) {  // ball 0
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.63f, 0.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.25f);
        }
        else if (mat_id == 3) {  // ball 1
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.83f, 0.83f, 0.32f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.74f);
        }
        else if (mat_id == 4) {  // ball 2
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::purple, 0.8f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.3f);
        }
        else if (mat_id == 50) {  // hair (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Hair.png"));
            pbr_mat.SetUniform(pbr_u::roughness, 0.8f);
            pbr_mat.SetUniform(pbr_u::specular, 0.7f);
        }
        else if (mat_id == 51) {  // body (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Cloth.png"));
        }
        else if (mat_id == 52) {  // cloth (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Body.png"));
        }
        else if (mat_id == 53) {  // head (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Head.png"));
        }
        else if (mat_id == 54) {  // L eye (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Head.png"));
            pbr_mat.SetUniform(pbr_u::roughness, 0.045f);
            pbr_mat.SetUniform(pbr_u::specular, 0.35f);
        }
        else if (mat_id == 55) {  // R eye (Nekomimi Suzune)
            pbr_mat.SetTexture(pbr_t::albedo, MakeAsset<Texture>(paths::model + "suzune\\Head2.png"));
            pbr_mat.SetUniform(pbr_u::roughness, 0.045f);
            pbr_mat.SetUniform(pbr_u::specular, 0.35f);
        }
        else if (mat_id == 70) {  // pillars
            pbr_mat.SetUniform(pbr_u::albedo, vec4(1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.18f);
        }

        if (mat_id >= 50 && mat_id <= 55) {  // Nekomimi Suzune
            auto& bone_transforms = suzune.GetComponent<Animator>().bone_transforms;
            pbr_mat.SetUniformArray(100U, bone_transforms.size(), &bone_transforms);
        }
    }

}
