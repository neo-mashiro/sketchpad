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
#include "example/scene_06.h"

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

    static int   tab_id = 0;
    static bool  enable_pl = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene06::Init() {
        this->title = "Bezier Area Lights with LTC";
        PrecomputeIBL(paths::texture + "HDRI\\Evening_07_4K.hdr");

        resource_manager.Add(00, MakeAsset<CShader>(paths::shader + "core\\bloom.glsl"));
        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(03, MakeAsset<Shader>(paths::shader + "core\\light.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_06\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_06\\post_process.glsl"));

        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(13, MakeAsset<Material>(resource_manager.Get<Shader>(03)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));

        resource_manager.Add(20, MakeAsset<Texture>(paths::model + "sibenik\\kamen_zid_albedo.jpg"));
        resource_manager.Add(21, MakeAsset<Texture>(paths::model + "sibenik\\kamen_zid_normal.jpg"));
        resource_manager.Add(22, MakeAsset<Texture>(paths::model + "sibenik\\kamen_zid_rough.jpg"));
        resource_manager.Add(23, MakeAsset<Texture>(paths::model + "sibenik\\kamen_zid_ao.jpg"));

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

        const float intensity[] = { 3.000f, 3.000f, 3.000f, 3.000f };
        const float linear[]    = { 0.500f, 0.010f, 0.010f, 0.010f };
        const float quadratic[] = { 0.035f, 0.069f, 0.069f, 0.090f };

        const vec4 position[] = {
            vec4(0.0f, 13.86f, -18.0f, 1.0f), vec4(3.25f, 9.68f, -11.5f, 1.0f),
            vec4(-3.25f, 9.68f, -11.5f, 1.0f), vec4(0.0f, 6.4f, 19.0f, 1.0f)
        };

        for (unsigned int i = 0; i < 4; i++) {
            light[i] = CreateEntity("Light " + std::to_string(i));
            light[i].AddComponent<Mesh>(Primitive::Cube);
            light[i].GetComponent<Transform>().SetPosition(vec3(position[i]));
            light[i].GetComponent<Transform>().Scale(0.05f);
            light[i].AddComponent<PointLight>(color::orange, intensity[i]);
            light[i].GetComponent<PointLight>().SetAttenuation(linear[i], quadratic[i]);

            auto& mat = light[i].AddComponent<Material>(resource_manager.Get<Material>(13));
            auto& pl = light[i].GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 3.0f);
        }

        // lights x 4 are static, we only need to setup UBO once in `Init()`
        if (auto& ubo = UBOs[1]; true) {
            vec4 arr_0[4] {};  // array element in std140 is padded to a `vec4`
            vec4 arr_2[4] {};
            vec4 arr_3[4] {};
            vec4 arr_4[4] {};
            vec4 arr_5[4] {};

            for (int i = 0; i < 4; i++) {
                auto& pl = light[i].GetComponent<PointLight>();
                arr_0[i] = vec4(pl.color, 1.0f);
                arr_2[i] = vec4(pl.intensity);
                arr_3[i] = vec4(pl.linear);
                arr_4[i] = vec4(pl.quadratic);
                arr_5[i] = vec4(pl.range);
            }

            ubo.SetUniform(0, arr_0);
            ubo.SetUniform(1, position);
            ubo.SetUniform(2, arr_2);
            ubo.SetUniform(3, arr_3);
            ubo.SetUniform(4, arr_4);
            ubo.SetUniform(5, arr_5);
        }

        cathedral = CreateEntity("Cathedral");
        cathedral.GetComponent<Transform>().Rotate(world::up, 90.0f, Space::Local);
        cathedral.GetComponent<Transform>().Translate(0.0f, 18.0f, 0.0f);

        if (std::string model_path = paths::model + "sibenik\\sibenik.obj"; true) {
            auto& model = cathedral.AddComponent<Model>(model_path, Quality::Auto);

            SetupMaterial(model.SetMaterial("pod_rub",           resource_manager.Get<Material>(14)), 10);
            SetupMaterial(model.SetMaterial("sprljci",           resource_manager.Get<Material>(14)), 11);
            SetupMaterial(model.SetMaterial("kamen_zid",         resource_manager.Get<Material>(14)), 12);
            SetupMaterial(model.SetMaterial("pod_tepih",         resource_manager.Get<Material>(14)), 13);
            SetupMaterial(model.SetMaterial("staklo_crveno",     resource_manager.Get<Material>(14)), 14);
            SetupMaterial(model.SetMaterial("staklo",            resource_manager.Get<Material>(14)), 15);
            SetupMaterial(model.SetMaterial("stupovi",           resource_manager.Get<Material>(14)), 16);
            SetupMaterial(model.SetMaterial("staklo_zuto",       resource_manager.Get<Material>(14)), 17);
            SetupMaterial(model.SetMaterial("pod",               resource_manager.Get<Material>(14)), 18);
            SetupMaterial(model.SetMaterial("kamen_zid_prozor",  resource_manager.Get<Material>(14)), 19);
            SetupMaterial(model.SetMaterial("zid_vani",          resource_manager.Get<Material>(14)), 20);
            SetupMaterial(model.SetMaterial("kamen_zid_parapet", resource_manager.Get<Material>(14)), 21);
            SetupMaterial(model.SetMaterial("rozeta",            resource_manager.Get<Material>(14)), 22);
            SetupMaterial(model.SetMaterial("staklo_zeleno",     resource_manager.Get<Material>(14)), 23);
            SetupMaterial(model.SetMaterial("staklo_plavo",      resource_manager.Get<Material>(14)), 24);
        }

        // TODO: import shader ball models, disable `ai_Pre_transformVertices` by setting
        // `animate` to true, so that we can shade every individual mesh differently

        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::AlphaBlend(true);
        Renderer::FaceCulling(false);
    }

    void Scene06::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        if (auto& ubo = UBOs[0]; true) {
            ubo.SetUniform(0, val_ptr(main_camera.T->position));
            ubo.SetUniform(1, val_ptr(main_camera.T->forward));
            ubo.SetUniform(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetUniform(3, val_ptr(main_camera.GetProjectionMatrix()));
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];
        FBO& framebuffer_2 = FBOs[2];

        // ------------------------------ MRT render pass ------------------------------

        framebuffer_0.Clear();
        framebuffer_0.Bind();

        if (tab_id == 0) {

        }
        else if (tab_id == 1) {

        }

        if (enable_pl) {
            Renderer::Submit(light[0].id, light[1].id, light[2].id, light[3].id);
        }
        Renderer::Submit(cathedral.id);
        Renderer::Submit(skybox.id);
        Renderer::Render();

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

    void Scene06::OnImGuiRender() {
        using namespace ImGui;
        const ImVec4 tab_color_off = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
        const ImVec4 tab_color_on = ImVec4(0.0f, 0.4f, 0.8f, 1.0f);
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;

        if (ui::NewInspector()) {
            Indent(5.0f);
            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.1f, 1.5f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Separator();

            BeginTabBar("InspectorTab", ImGuiTabBarFlags_None);

            if (BeginTabItem("Polygon 1")) {
                tab_id = 0;
                PushItemWidth(130.0f);
                Checkbox("Enable Point Lights", &enable_pl);
                //Checkbox("Ball Bounce", &bounce_ball);
                //SliderFloat("Animation Speed", &animate_speed, 0.1f, 3.0f);
                //ColorEdit3("Window Border Color", val_ptr(window_border_albedo), color_flags);
                EndTabItem();
            }

            if (BeginTabItem("Polygon 2")) {
                tab_id = 1;
                EndTabItem();
            }

            //ui::DrawGizmo(camera, light[plid - 1], ui::Gizmo::Translate);
            
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

    void Scene06::PrecomputeIBL(const std::string& hdri) {
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

    void Scene06::SetupMaterial(Material& pbr_mat, int mat_id) {
        pbr_mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        pbr_mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        pbr_mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        pbr_mat.BindUniform(0, &skybox_exposure);
        pbr_mat.BindUniform(1, &enable_pl);

        std::string tex_path = utils::paths::model + "sibenik\\";

        if (mat_id == 10) {  // hallway curb
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(8.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "pod_rub_albedo.jpg"));
            pbr_mat.SetTexture(pbr_t::normal,    MakeAsset<Texture>(tex_path + "pod_rub_normal.jpg"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "pod_rub_rough.jpg"));
            pbr_mat.SetTexture(pbr_t::ao,        MakeAsset<Texture>(tex_path + "pod_rub_ao.jpg"));
        }
        else if (mat_id == 11) {  // square window frames
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.0f, 0.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.75f);
        }
        else if (mat_id == 12) {  // main body
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    resource_manager.Get<Texture>(20));
            pbr_mat.SetTexture(pbr_t::normal,    resource_manager.Get<Texture>(21));
            pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(22));
            pbr_mat.SetTexture(pbr_t::ao,        resource_manager.Get<Texture>(23));
        }
        else if (mat_id == 13) {  // red carpet
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(3, 0));
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.0f, 0.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 1.0f);
            pbr_mat.SetUniform(pbr_u::sheen_color, vec3(1.0f, 0.0f, 0.0f));
        }
        else if (mat_id == 14) {  // circle window inner frame
            pbr_mat.SetUniform(pbr_u::albedo, vec4(1.0f));
        }
        else if (mat_id == 15) {  // window glasses
            pbr_mat.SetUniform(pbr_u::shading_model, uvec2(2, 0));
            pbr_mat.SetUniform(pbr_u::roughness, 0.045f);
            pbr_mat.SetUniform(pbr_u::transmittance, color::white);
            pbr_mat.SetUniform(pbr_u::transmission, 1.0f);
            pbr_mat.SetUniform(pbr_u::volume_type, 1U);
        }
        else if (mat_id == 16) {  // pillars
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(8.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "stupovi_albedo.jpg"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "stupovi_rough.jpg"));
        }
        else if (mat_id == 17) {  // circle window inner dots
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.0f, 1.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.25f);
        }
        else if (mat_id == 18) {  // floor
            pbr_mat.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(tex_path + "tile_albedo.png"));
            pbr_mat.SetTexture(pbr_t::metallic,  MakeAsset<Texture>(tex_path + "tile_metalness.png"));
            pbr_mat.SetTexture(pbr_t::roughness, MakeAsset<Texture>(tex_path + "tile_roughness.png"));
        }
        else if (mat_id == 19) {  // window dent
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    resource_manager.Get<Texture>(20));
            pbr_mat.SetTexture(pbr_t::normal,    resource_manager.Get<Texture>(21));
            pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(22));
            pbr_mat.SetTexture(pbr_t::ao,        resource_manager.Get<Texture>(23));
        }
        else if (mat_id == 20) {  // exterior border
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    resource_manager.Get<Texture>(20));
            pbr_mat.SetTexture(pbr_t::normal,    resource_manager.Get<Texture>(21));
            pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(22));
            pbr_mat.SetTexture(pbr_t::ao,        resource_manager.Get<Texture>(23));
        }
        else if (mat_id == 21) {  // ladder bars
            pbr_mat.SetUniform(pbr_u::uv_scale, vec2(4.0f));
            pbr_mat.SetTexture(pbr_t::albedo,    resource_manager.Get<Texture>(20));
            pbr_mat.SetTexture(pbr_t::normal,    resource_manager.Get<Texture>(21));
            pbr_mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(22));
            pbr_mat.SetTexture(pbr_t::ao,        resource_manager.Get<Texture>(23));
        }
        else if (mat_id == 22) {  // circle window frame
            pbr_mat.SetUniform(pbr_u::albedo, vec4(0.0f, 0.0f, 0.0f, 1.0f));
            pbr_mat.SetUniform(pbr_u::metalness, 1.0f);
            pbr_mat.SetUniform(pbr_u::roughness, 0.72f);
        }
        else if (mat_id == 23) {  // circle window outer dots
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::purple, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.25f);
        }
        else if (mat_id == 24) {  // circle window middle dots
            pbr_mat.SetUniform(pbr_u::albedo, vec4(color::green, 1.0f));
            pbr_mat.SetUniform(pbr_u::roughness, 0.25f);
        }
    }

}
