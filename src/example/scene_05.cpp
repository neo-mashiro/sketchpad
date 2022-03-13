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
    static float skybox_lod = 0.0f;

    static int   tab_id = 0;
    static bool  pl_enabled = false;  // enable point light?
    static bool  sl_enabled = false;  // enable spotlight?
    static bool  bounce_ball = false;
    static float bounce_time = 0.0f;

    static vec4  albedo = vec4(color::black, 1.0f);
    static float roughness = 1.0f;
    static float metalness = 0.0f;
    static float clearcoat = 0.0f;
    static float specular = 0.5f;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene05::Init() {
        this->title = "PCSS VSSM Shadows";
        PrecomputeIBL();

        resource_manager.Add(10, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(11, MakeAsset<Shader>(paths::shader + "scene_05\\pbr.glsl"));
        resource_manager.Add(12, MakeAsset<Shader>(paths::shader + "scene_05\\skybox.glsl"));
        resource_manager.Add(13, MakeAsset<Shader>(paths::shader + "scene_05\\light.glsl"));
        resource_manager.Add(14, MakeAsset<Shader>(paths::shader + "scene_01\\blur.glsl"));////////////////
        resource_manager.Add(15, MakeAsset<Shader>(paths::shader + "scene_05\\post_process.glsl"));

        resource_manager.Add(21, MakeAsset<Material>(resource_manager.Get<Shader>(11)));
        resource_manager.Add(22, MakeAsset<Material>(resource_manager.Get<Shader>(12)));
        resource_manager.Add(23, MakeAsset<Material>(resource_manager.Get<Shader>(13)));

        resource_manager.Add(30, MakeAsset<Texture>(paths::texture + "Wood034_1K-JPG\\albedo.jpg"));
        resource_manager.Add(31, MakeAsset<Texture>(paths::texture + "Wood034_1K-JPG\\normal.jpg"));
        //resource_manager.Add(32, MakeAsset<Texture>(paths::texture + "Wood034_1K-JPG\\roughness.jpg"));

        resource_manager.Add(50, MakeAsset<Texture>(paths::model + "suzune\\Hair.png"));
        resource_manager.Add(51, MakeAsset<Texture>(paths::model + "suzune\\Body.png"));
        resource_manager.Add(52, MakeAsset<Texture>(paths::model + "suzune\\Cloth.png"));
        resource_manager.Add(53, MakeAsset<Texture>(paths::model + "suzune\\Head.png"));
        resource_manager.Add(54, MakeAsset<Texture>(paths::model + "suzune\\Head2.png"));

        resource_manager.Add(41, MakeAsset<Sampler>(FilterMode::Point));
        resource_manager.Add(42, MakeAsset<Sampler>(FilterMode::Bilinear));
        
        AddUBO(resource_manager.Get<Shader>(11)->ID());
        AddUBO(resource_manager.Get<Shader>(12)->ID());
        AddUBO(resource_manager.Get<Shader>(13)->ID());

        AddFBO(Window::width, Window::height);          // bloom filter pass
        AddFBO(Window::width, Window::height);          // MSAA resolve pass
        AddFBO(Window::width / 2, Window::height / 2);  // Gaussian blur pass

        FBOs[0].AddColorTexture(2, true);    // multisampled textures for MSAA
        FBOs[0].AddDepStRenderBuffer(true);  // multisampled RBO for MSAA
        FBOs[1].AddColorTexture(2);
        FBOs[2].AddColorTexture(2);

        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(0.0f, 6.0f, 9.0f);
        camera.AddComponent<Camera>(View::Perspective);
        
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(resource_manager.Get<Material>(22)); true) {
            mat.SetTexture(0, prefiltered_map);
            mat.BindUniform(0, &skybox_exposure);
            mat.BindUniform(1, &skybox_lod);
        }

        point_light = CreateEntity("Point Light");
        point_light.AddComponent<Mesh>(Primitive::Cube);
        point_light.GetComponent<Transform>().Translate(world::up * 6.0f);
        point_light.GetComponent<Transform>().Translate(world::forward * -4.0f);
        point_light.GetComponent<Transform>().Scale(0.1f);
        point_light.AddComponent<PointLight>(color::orange, 1.8f);
        point_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

        if (auto& mat = point_light.AddComponent<Material>(resource_manager.Get<Material>(23)); true) {
            auto& pl = point_light.GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        spotlight = CreateEntity("Spotlight");
        spotlight.AddComponent<Mesh>(Primitive::Tetrahedron);
        spotlight.GetComponent<Transform>().Translate(vec3(0.0f, 10.0f, -7.0f));
        spotlight.GetComponent<Transform>().Rotate(world::right, -90.0f, Space::Local);
        spotlight.GetComponent<Transform>().Scale(0.1f);
        spotlight.AddComponent<Spotlight>(color::white, 13.8f);
        spotlight.GetComponent<Spotlight>().SetCutoff(20.0f, 10.0f, 45.0f);

        if (auto& mat = spotlight.AddComponent<Material>(resource_manager.Get<Material>(23)); true) {
            auto& sl = spotlight.GetComponent<Spotlight>();
            mat.SetUniform(3, sl.color);
            mat.SetUniform(4, sl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        // spotlight  + floor + wall + suzune
        // pointlight + floor + bouncing balls
        // area light + floor + vehicle

        floor = CreateEntity("Floor");
        floor.AddComponent<Mesh>(Primitive::Plane);
        floor.GetComponent<Transform>().Translate(0.0f, -1.05f, 0.0f);
        floor.GetComponent<Transform>().Scale(3.0f);
        SetupMaterial(floor.AddComponent<Material>(resource_manager.Get<Material>(21)), 0);

        wall = CreateEntity("Wall");
        wall.AddComponent<Mesh>(Primitive::Cube);
        wall.GetComponent<Transform>().Translate(0.0f, 5.0f, -8.0f);
        wall.GetComponent<Transform>().Scale(12.0f, 6.0f, 0.25f);
        SetupMaterial(wall.AddComponent<Material>(resource_manager.Get<Material>(21)), 1);

        for (int i = 0; i < 3; i++) {
            ball[i] = CreateEntity("Sphere " + std::to_string(i));
            ball[i].GetComponent<Transform>().Translate(vec3(-i * 2, (i + 1) * 2, pow(-1, i) - 4));
            ball[i].AddComponent<Mesh>(Primitive::Sphere);

            auto& mat = ball[i].AddComponent<Material>(resource_manager.Get<Material>(21));
            SetupMaterial(mat, i + 2);
        }

        suzune = CreateEntity("Nekomimi Suzune");
        suzune.GetComponent<Transform>().Translate(vec3(0.0f, -1.05f, -3.0f));
        suzune.GetComponent<Transform>().Scale(0.05f);

        if (auto& model = suzune.AddComponent<Model>(paths::model + "suzune\\suzune.fbx", Quality::Auto); true) {
            SetupMaterial(model.SetMaterial("mat_Suzune_Hair", resource_manager.Get<Material>(21)), 50);
            SetupMaterial(model.SetMaterial("mat_Suzune_Body", resource_manager.Get<Material>(21)), 51);
            SetupMaterial(model.SetMaterial("mat_Suzune_Cloth",resource_manager.Get<Material>(21)), 52);
            SetupMaterial(model.SetMaterial("mat_Suzune_Head", resource_manager.Get<Material>(21)), 53);
            SetupMaterial(model.SetMaterial("mat_Suzune_EyeL", resource_manager.Get<Material>(21)), 54);
            SetupMaterial(model.SetMaterial("mat_Suzune_EyeR", resource_manager.Get<Material>(21)), 55);
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
            auto& main_camera = camera.GetComponent<Camera>();
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

        if (auto& ubo = UBOs[2]; true) {
            auto& sl = spotlight.GetComponent<Spotlight>();
            auto& st = spotlight.GetComponent<Transform>();
            float inner_cos = sl.GetInnerCosine();
            float outer_cos = sl.GetOuterCosine();
            ubo.SetUniform(0, val_ptr(sl.color));
            ubo.SetUniform(1, val_ptr(st.position));
            ubo.SetUniform(2, val_ptr(-st.forward));
            ubo.SetUniform(3, val_ptr(sl.intensity));
            ubo.SetUniform(4, val_ptr(inner_cos));
            ubo.SetUniform(5, val_ptr(outer_cos));
            ubo.SetUniform(6, val_ptr(sl.range));
        }

        //////////// bloom filter pass
        if (FBO& framebuffer = FBOs[0]; true) {
            framebuffer.Clear();
            framebuffer.Bind();
            Renderer::DepthTest(true);

            Renderer::Submit(floor.id);

            
            if (tab_id == 0) {
                pl_enabled = true;
                sl_enabled = false;

                if (bounce_ball) {  // simulate gravity using a cheap quadratic easing factor
                    bounce_time += Clock::delta_time;
                    for (int i = 0; i < 3; i++) {
                        float s = 2.0f - i * 0.4f;  // falling and bouncing speed
                        float t = math::Bounce(bounce_time * s, 1.0f);  // bounce between 0.0 and 1.0
                        float e = t * t;            // ease in, slow near 0 and fast near 1
                        float h = (i + 1) * 2.0f;   // initial height of the ball
                        float y = math::Lerp(h, -0.05f, e);

                        auto& T = ball[i].GetComponent<Transform>();
                        T.SetPosition(vec3(T.position.x, y, T.position.z));
                    }
                }

                Renderer::Submit(skybox.id);
                Renderer::Submit(point_light.id);
                Renderer::Submit(ball[0].id, ball[1].id);
                Renderer::Submit(ball[2].id);  // transparent ball is rendered last
            }
            else if (tab_id == 1) {
                pl_enabled = false;
                sl_enabled = true;
                Renderer::Submit(suzune.id);
                Renderer::Submit(wall.id);
                Renderer::Submit(spotlight.id);
                Renderer::Submit(skybox.id);
            }
            else if (tab_id == 2) {
                
            }

            
            Renderer::Render();

            if (show_grid) {
                auto grid_shader = resource_manager.Get<Shader>(10);
                grid_shader->Bind();
                grid_shader->SetUniform(0, grid_cell_size);
                grid_shader->SetUniform(1, thin_line_color);
                grid_shader->SetUniform(2, wide_line_color);
                Mesh::DrawGrid();
            }

            framebuffer.Unbind();
        }
        
        //////////// resolve msaa
        FBO& source = FBOs[0];
        FBO& target = FBOs[1];
        target.Clear();

        FBO::CopyColor(source, 0, target, 0);
        FBO::CopyColor(source, 1, target, 1);

        ////////////////////////
        if (FBO& framebuffer = FBOs[2]; true) {
            framebuffer.Clear();

            auto& ping_texture = framebuffer.GetColorTexture(0);
            auto& pong_texture = framebuffer.GetColorTexture(1);
            auto& source_texture = FBOs[1].GetColorTexture(1);

            // enable point filtering (nearest neighbor) on the ping-pong textures
            auto point_sampler = resource_manager.Get<Sampler>(41);
            point_sampler->Bind(0);
            point_sampler->Bind(1);

            // make sure the view port is resized to fit the entire texture
            Renderer::SetViewport(ping_texture.width, ping_texture.height);
            Renderer::DepthTest(false);

            auto blur_shader = resource_manager.Get<Shader>(14);

            if (framebuffer.Bind(); true) {
                blur_shader->Bind();
                source_texture.Bind(0);
                ping_texture.Bind(1);
                pong_texture.Bind(2);

                bool ping = true;  // read from ping, write to pong

                for (int i = 0; i < 3 * 2; i++, ping = !ping) {
                    framebuffer.SetDrawBuffer(static_cast<GLuint>(ping));
                    blur_shader->SetUniform(0, i == 0);
                    blur_shader->SetUniform(1, ping);
                    Mesh::DrawQuad();
                }

                source_texture.Unbind(0);
                ping_texture.Unbind(1);
                pong_texture.Unbind(2);
                blur_shader->Unbind();
                framebuffer.Unbind();
            }

            // after an even number of iterations, the blurred results are stored in texture "ping"
            // also don't forget to restore the view port back to normal window size
            Renderer::SetViewport(Window::width, Window::height);
        }

        // finally we are back on the default framebuffer
        auto& original_texture = FBOs[1].GetColorTexture(0);
        auto& blurred_texture = FBOs[2].GetColorTexture(0);

        original_texture.Bind(0);
        blurred_texture.Bind(1);

        auto bilinear_sampler = resource_manager.Get<Sampler>(42);
        bilinear_sampler->Bind(1);  // bilinear filtering will introduce extra blurred effects

        auto postprocess_shader = resource_manager.Get<Shader>(15);

        if (postprocess_shader->Bind(); true) {
            postprocess_shader->SetUniform(0, 3);
            Renderer::Clear();
            Mesh::DrawQuad();
            postprocess_shader->Unbind();
        }

        bilinear_sampler->Unbind(0);
        bilinear_sampler->Unbind(1);
    }

    void Scene05::OnImGuiRender() {
        using namespace ImGui;
        const ImVec4 tab_color_off = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
        const ImVec4 tab_color_on = ImVec4(0.0f, 0.4f, 0.8f, 1.0f);
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;

        if (ui::NewInspector()) {
            Indent(5.0f);
            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.5f, 2.0f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Separator();

            BeginTabBar("InspectorTab", ImGuiTabBarFlags_None);

            //PushItemWidth(130.0f);
            //ColorEdit3("Albedo", val_ptr(albedo), color_flags);
            //SliderFloat("Metalness", &metalness, 0.045f, 1.0f);
            //SliderFloat("Roughness", &roughness, 0.045f, 1.0f);
            //SliderFloat("Transparency", &(albedo.a), 0.0f, 1.0f);
            //PopItemWidth();

            if (BeginTabItem("Point Light")) {
                tab_id = 0;
                PushItemWidth(130.0f);
                Checkbox("Ball Bounce", &bounce_ball);
                PopItemWidth();
                EndTabItem();
            }

            if (BeginTabItem("Spotlight")) {
                tab_id = 1;
                EndTabItem();
            }

            if (BeginTabItem("Area Light")) {
                tab_id = 2;
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

    void Scene05::PrecomputeIBL() {
        Renderer::SeamlessCubemap(true);
        Renderer::DepthTest(false);
        Renderer::FaceCulling(true);

        auto irradiance_shader = CShader(paths::shader + "scene_03\\irradiance_map.glsl");
        auto prefilter_shader = CShader(paths::shader + "scene_03\\prefilter_envmap.glsl");
        auto envBRDF_shader = CShader(paths::shader + "scene_03\\environment_BRDF.glsl");

        std::string rootpath = utils::paths::root;
        if (rootpath.find("mashiro") == std::string::npos) {
            irradiance_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
            prefiltered_map = MakeAsset<Texture>(paths::texture + "test\\newport_loft.hdr", 1024, 8);
            Texture::Copy(*prefiltered_map, 3, *irradiance_map, 0);
            BRDF_LUT = MakeAsset<Texture>(paths::texture + "common\\checkboard.png", 1);
            Sync::WaitFinish();
            return;
        }

        std::string hdri = "newport_loft.hdr";
        auto env_map = MakeAsset<Texture>(utils::paths::texture + "test\\" + hdri, 2048, 0);
        env_map->Bind(0);

        irradiance_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
        prefiltered_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 2048, 2048, 6, GL_RGBA16F, 8);
        BRDF_LUT = MakeAsset<Texture>(GL_TEXTURE_2D, 1024, 1024, 1, GL_RGBA16F, 1);///////////////// 3 channels but ILS can only be RGBA

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

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene05::SetupMaterial(Material& pbr_mat, int mat_id) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        pbr_mat.BindUniform(0, &pl_enabled);
        pbr_mat.BindUniform(1, &sl_enabled);

        //pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f, 4.0f));
        //pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(50));
        //pbr_mat.SetTexture(pbr_t::normal, resource_manager.Get<Texture>(51));
        //pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(52));
        //pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
        //pbr_mat.SetTexture(pbr_t::ao, resource_manager.Get<Texture>(53));
        //pbr_mat.SetUniform(pbr_u::clearcoat, 1.0f);

        if (mat_id == 0) {  // floor
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f, 4.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 0.0f);
            pbr_mat.SetUniform(pbr_u::ao, 1.0f);
            //pbr_mat.SetUniform(pbr_u::clearcoat, 1.0f);
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(30));
            pbr_mat.SetTexture(pbr_t::normal, resource_manager.Get<Texture>(31));
            //pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(32));
            pbr_mat.SetUniform(pbr_u::roughness, 1.0f);
        }
        else if (mat_id == 1) {  // wall
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
        }
        else if (mat_id == 2) {  // ball 0, tweak done!
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.63f, 0.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.25f);
        }
        else if (mat_id == 3) {  // ball 1
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.83f, 0.83f, 0.32f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.74f);
        }
        else if (mat_id == 4) {  // ball 2
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::purple, 0.8f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.3f);
        }
        // Nekomimi Suzune anime character model
        else if (mat_id == 50) {  // hair
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(50));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::roughness, 0.8f);
            pbr_mat.SetUniform(pbr_u::specular, 0.7f);
        }
        else if (mat_id == 51) {  // body
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(51));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
        }
        else if (mat_id == 52) {  // cloth
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(52));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
        }
        else if (mat_id == 53) {  // head
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(53));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
        }
        else if (mat_id == 54) {  // L eye
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(53));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::roughness, 0.045f);
            pbr_mat.SetUniform(pbr_u::specular, 0.35f);
        }
        else if (mat_id == 55) {  // R eye
            pbr_mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(54));
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(1, 0));
            pbr_mat.SetUniform(pbr_u::roughness, 0.045f);
            pbr_mat.SetUniform(pbr_u::specular, 0.35f);
        }
        else if (mat_id == 6) {  // vehicle

        }
    }

}
