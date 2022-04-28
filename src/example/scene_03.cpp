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
#include "example/scene_03.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    static bool  show_grid       = false;
    static float grid_cell_size  = 2.0f;
    static vec4  thin_line_color = vec4(0.1f, 0.1f, 0.1f, 1.0f);
    static vec4  wide_line_color = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    static vec3  dl_direction    = vec3(0.7f, -0.7f, 0.0f);

    static float skybox_exposure = 1.0f;
    static float skybox_lod      = 0.0f;

    static bool  show_gizmo    = false;
    static bool  rotate_model  = false;
    static bool  reset_model   = false;
    static vec3  model_axis    = world::right;

    static float const_zero    = 0.0f;
    static int   entity_id     = 1;
    static uint  shading_model = 11;

    static vec4  albedo        = vec4(color::white, 1.0f);
    static float roughness     = 1.0f;
    static float ao            = 1.0f;
    static float metalness     = 0.0f;
    static float specular      = 0.5f;
    static float anisotropy    = 0.0f;
    static vec3  aniso_dir     = world::right;
    static float transmission  = 0.0f;
    static float thickness     = 2.0f;
    static float ior           = 1.5f;
    static vec3  transmittance = color::purple;
    static float tr_distance   = 4.0f;
    static uint  volume_type   = 0U;
    static float clearcoat     = 0.0f;
    static float cc_roughness  = 0.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene03::Init() {
        this->title = "Disney Principled BSDF";
        PrecomputeIBL(utils::paths::texture + "HDRI\\hotel_room_4k2.hdr");

        resource_manager.Add(01, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));
        resource_manager.Add(02, MakeAsset<Shader>(paths::shader + "core\\skybox.glsl"));
        resource_manager.Add(04, MakeAsset<Shader>(paths::shader + "scene_03\\pbr.glsl"));
        resource_manager.Add(05, MakeAsset<Shader>(paths::shader + "scene_03\\post_process.glsl"));
        resource_manager.Add(12, MakeAsset<Material>(resource_manager.Get<Shader>(02)));
        resource_manager.Add(14, MakeAsset<Material>(resource_manager.Get<Shader>(04)));
        
        AddUBO(resource_manager.Get<Shader>(02)->ID());
        AddUBO(resource_manager.Get<Shader>(04)->ID());

        AddFBO(Window::width, Window::height);
        AddFBO(Window::width, Window::height);

        FBOs[0].AddColorTexture(1, true);
        FBOs[0].AddDepStRenderBuffer(true);
        FBOs[1].AddColorTexture(1);

        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(0.0f, 6.0f, 9.0f);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(color::red, 3.8f);
        camera.GetComponent<Spotlight>().SetCutoff(4.0f, 10.0f, 45.0f);
        
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(resource_manager.Get<Material>(12)); true) {
            mat.SetTexture(0, prefiltered_map);
            mat.BindUniform(0, &skybox_exposure);
            mat.BindUniform(1, &skybox_lod);
        }

        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(45.0f, 180.0f, 0.0f, Space::World);
        direct_light.AddComponent<DirectionLight>(color::yellow, 0.2f);  // 0 attenuation -> small intensity

        pistol = CreateEntity("Pistol");
        pistol.GetComponent<Transform>().Translate(vec3(0.0f, 5.0f, 0.0f));
        pistol.GetComponent<Transform>().Scale(0.3f);

        if (auto m_path = paths::model + "SW500\\"; true) {
            auto& model = pistol.AddComponent<Model>(m_path + "SW500.fbx", Quality::Auto);
            auto& mat_b = model.SetMaterial("TEX_Bullet",  resource_manager.Get<Material>(14));
            auto& mat_p = model.SetMaterial("TEX_Lowpoly", resource_manager.Get<Material>(14));

            SetupMaterial(mat_b);
            mat_b.SetUniform(pbr_u::shading_model, uvec2(1, 0));  // bullet ignores clear coat layer
            mat_b.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(m_path + "bullet_albedo.jpg"));
            mat_b.SetTexture(pbr_t::normal,    MakeAsset<Texture>(m_path + "bullet_normal.png"));
            mat_b.SetTexture(pbr_t::metallic,  MakeAsset<Texture>(m_path + "bullet_metallic.jpg"));
            mat_b.SetTexture(pbr_t::roughness, MakeAsset<Texture>(m_path + "bullet_roughness.jpg"));
            mat_b.SetTexture(pbr_t::ao,        MakeAsset<Texture>(m_path + "bullet_AO.jpg"));

            SetupMaterial(mat_p);
            mat_p.SetUniform(pbr_u::shading_model, uvec2(1, 1));
            mat_p.SetTexture(pbr_t::albedo,    MakeAsset<Texture>(m_path + "SW500_albedo.png"));
            mat_p.SetTexture(pbr_t::normal,    MakeAsset<Texture>(m_path + "SW500_normal.png"));
            mat_p.SetTexture(pbr_t::metallic,  MakeAsset<Texture>(m_path + "SW500_metallic.png"));
            mat_p.SetTexture(pbr_t::roughness, MakeAsset<Texture>(m_path + "SW500_roughness.png"));
            mat_p.SetTexture(pbr_t::ao,        MakeAsset<Texture>(m_path + "SW500_AO.jpg"));
        }

        helmet = CreateEntity("Helmet");
        helmet.GetComponent<Transform>().Translate(vec3(0.2f, 4.0f, -2.0f));
        helmet.GetComponent<Transform>().Scale(0.02f);

        if (auto& model = helmet.AddComponent<Model>(paths::model + "mandalorian.fbx", Quality::Auto); true) {
            SetupMaterial(model.SetMaterial("DefaultMaterial", resource_manager.Get<Material>(14)));
            SetupMaterial(model.SetMaterial("Material #26", resource_manager.Get<Material>(14)));
        }

        pyramid = CreateEntity("Pyramid");
        pyramid.AddComponent<Mesh>(Primitive::Tetrahedron);
        pyramid.GetComponent<Transform>().Translate(world::up * 5.0f);
        pyramid.GetComponent<Transform>().Scale(2.0f);

        if (auto& mat = pyramid.AddComponent<Material>(resource_manager.Get<Material>(14)); true) {
            SetupMaterial(mat);
            mat.SetUniform(pbr_u::shading_model, uvec2(2, 0));
        }

        capsule = CreateEntity("Capsule");
        capsule.AddComponent<Mesh>(Primitive::Capsule);
        capsule.GetComponent<Transform>().Translate(world::up * 5.0f);
        capsule.GetComponent<Transform>().Scale(2.0f);

        if (auto& mat = capsule.AddComponent<Material>(resource_manager.Get<Material>(14)); true) {
            SetupMaterial(mat);
            mat.SetUniform(pbr_u::shading_model, uvec2(2, 0));
        }

        Renderer::MSAA(true);
        Renderer::DepthTest(true);
        Renderer::AlphaBlend(true);
    }

    Entity& Scene03::GetEntity(int entity_id) {
        switch (entity_id) {
            case 1: return pistol;
            case 2: return helmet;
            case 3: return pyramid;
            case 4: return capsule;
            default: throw std::runtime_error("Invalid entity id!");
        }
    }

    void Scene03::OnSceneRender() {
        auto& e = GetEntity(entity_id);
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

        if (auto& ubo = UBOs[2]; true) {
            auto& sl = camera.GetComponent<Spotlight>();
            auto& ct = camera.GetComponent<Transform>();
            float inner_cos = sl.GetInnerCosine();
            float outer_cos = sl.GetOuterCosine();
            ubo.SetUniform(0, val_ptr(sl.color));
            ubo.SetUniform(1, val_ptr(ct.position));
            ubo.SetUniform(2, val_ptr(-ct.forward));
            ubo.SetUniform(3, val_ptr(sl.intensity));
            ubo.SetUniform(4, val_ptr(inner_cos));
            ubo.SetUniform(5, val_ptr(outer_cos));
            ubo.SetUniform(6, val_ptr(sl.range));
        }

        FBO& framebuffer_0 = FBOs[0];
        FBO& framebuffer_1 = FBOs[1];

        // ------------------------------ MRT render pass ------------------------------

        framebuffer_0.Clear();
        framebuffer_0.Bind();

        if (reset_model) {
            const vec3 origin = vec3(0.0f, 5.0f, 0.0f);
            auto& T = e.GetComponent<Transform>();
            float t = math::EaseFactor(5.0f, Clock::delta_time);
            T.SetPosition(math::Lerp(T.position, origin, t));
            T.SetRotation(math::SlerpRaw(T.rotation, world::eye, t));

            if (math::Equals(T.position, origin) && math::Equals(T.rotation, world::eye)) {
                reset_model = false;
            }
        }
        else if (rotate_model) {
            e.GetComponent<Transform>().Rotate(model_axis, 0.36f, Space::Local);
        }

        Renderer::FaceCulling(entity_id != 2);
        Renderer::Submit(e.id);
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

        // ------------------------------ postprocessing pass ------------------------------

        framebuffer_1.GetColorTexture(0).Bind(0);
        auto postprocess_shader = resource_manager.Get<Shader>(05);
        postprocess_shader->Bind();
        postprocess_shader->SetUniform(0, 3);

        Renderer::Clear();
        Mesh::DrawQuad();
        postprocess_shader->Unbind();
    }

    void Scene03::OnImGuiRender() {
        using namespace ImGui;
        const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        const ImVec2 rainbow_offset = ImVec2(5.0f, 105.0f);
        const ImVec4 tab_color_off  = ImVec4(0.0f, 0.3f, 0.6f, 1.0f);
        const ImVec4 tab_color_on   = ImVec4(0.0f, 0.4f, 0.8f, 1.0f);

        if (ui::NewInspector()) {
            Indent(5.0f);
            Text(ICON_FK_SUN_O "  Directional Light Vector");
            DragFloat3("###", val_ptr(dl_direction), 0.01f, -1.0f, 1.0f, "%.3f");
            ui::DrawRainbowBar(rainbow_offset, 2.0f);
            Spacing();

            PushItemWidth(130.0f);
            SliderFloat("Skybox Exposure", &skybox_exposure, 0.5f, 4.0f);
            SliderFloat("Skybox LOD", &skybox_lod, 0.0f, 7.0f);
            PopItemWidth();
            Checkbox("Gizmo", &show_gizmo); SameLine();
            reset_model |= Button("###", ImVec2(30.0f, 0.0f)); SameLine();
            Text("Reset"); SameLine();
            if (Checkbox("Rotation", &rotate_model); rotate_model) {
                DragFloat3("Local Axis", val_ptr(model_axis), 0.01f, 0.0f, 1.0f, "%.2f");
            }
            Separator();

            BeginTabBar("InspectorTab", ImGuiTabBarFlags_None);

            if (BeginTabItem("ClearCoat")) {
                if (entity_id != 1) {
                    entity_id = 1;
                    anisotropy = 0.0f;
                }
                PushItemWidth(130.0f);
                SliderFloat("Specular", &specular, 0.35f, 1.0f);
                SliderFloat("Clearcoat", &clearcoat, 0.0f, 1.0f);
                SliderFloat("Clearcoat Roughness", &cc_roughness, 0.045f, 1.0f);
                PopItemWidth();
                EndTabItem();
            }

            if (BeginTabItem("Anisotropy")) {
                if (entity_id != 2) {
                    entity_id = 2;
                    metalness = roughness = 1.0f;
                }
                PushItemWidth(130.0f);
                ColorEdit4("Albedo", val_ptr(albedo), ImGuiColorEditFlags_NoInputs);
                SliderFloat("Roughness", &roughness, 0.045f, 1.0f);
                SliderFloat("Ambient Occlusion", &ao, 0.05f, 1.0f);
                SliderFloat("Anisotropy", &anisotropy, -1.0f, 1.0f);
                DragFloat3("Anisotropy Direction", val_ptr(aniso_dir), 0.01f, 0.1f, 1.0f, "%.1f");
                PopItemWidth();
                EndTabItem();
            }

            if (BeginTabItem("Refraction")) {
                if (entity_id < 3) {
                    entity_id = std::max(entity_id, 3);
                    roughness = 0.2f;
                }
                RadioButton("Cubic/Flat", &entity_id, 3); SameLine(164);
                RadioButton("Spherical", &entity_id, 4);
                volume_type = entity_id == 4 ? 0U : 1U;
                
                PushItemWidth(130.0f);
                ColorEdit4("Albedo", val_ptr(albedo), ImGuiColorEditFlags_NoInputs); SameLine(164);
                ColorEdit4("Transmittance", val_ptr(transmittance), ImGuiColorEditFlags_NoInputs);
                SliderFloat("Roughness", &roughness, 0.045f, 1.0f);
                SliderFloat("Ambient Occlusion", &ao, 0.05f, 1.0f);
                SliderFloat("Transmission", &transmission, 0.0f, 1.0f);
                SliderFloat("Thickness", &thickness, 2.0f, 4.0f);
                SliderFloat("IOR", &ior, 1.0f, 1.5f);
                SliderFloat("Transmission Distance", &tr_distance, 0.0f, 4.0f);
                PopItemWidth();
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

        if (show_gizmo) {
            ui::DrawGizmo(camera, GetEntity(entity_id), ui::Gizmo::Translate);
        }
    }

    void Scene03::PrecomputeIBL(const std::string& hdri) {
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

    void Scene03::SetupMaterial(Material& mat) {
        mat.SetTexture(pbr_t::irradiance_map, irradiance_map);
        mat.SetTexture(pbr_t::prefiltered_map, prefiltered_map);
        mat.SetTexture(pbr_t::BRDF_LUT, BRDF_LUT);

        mat.BindUniform(0, &skybox_exposure);
        mat.BindUniform(pbr_u::albedo, &albedo);
        mat.BindUniform(pbr_u::roughness, &roughness);
        mat.BindUniform(pbr_u::ao, &ao);
        mat.BindUniform(pbr_u::metalness, &metalness);
        mat.BindUniform(pbr_u::specular, &specular);
        mat.BindUniform(pbr_u::anisotropy, &anisotropy);
        mat.BindUniform(pbr_u::aniso_dir, &aniso_dir);
        mat.BindUniform(pbr_u::transmission, &transmission);
        mat.BindUniform(pbr_u::thickness, &thickness);
        mat.BindUniform(pbr_u::ior, &ior);
        mat.BindUniform(pbr_u::transmittance, &transmittance);
        mat.BindUniform(pbr_u::tr_distance, &tr_distance);
        mat.BindUniform(pbr_u::volume_type, &volume_type);
        mat.BindUniform(pbr_u::clearcoat, &clearcoat);
        mat.BindUniform(pbr_u::cc_roughness, &cc_roughness);
    }

}
