#include "pch.h"

#include "core/clock.h"
#include "core/input.h"
#include "core/window.h"
#include "buffer/all.h"
#include "components/all.h"
#include "components/component.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/math.h"
#include "utils/filesystem.h"

#include "examples/scene_02.h"


using namespace core;
using namespace buffer;
using namespace components;
using namespace utils;

namespace scene {

    //static bool show_plane = true;
    //static bool show_light_cluster = true;
    //static bool draw_depth_buffer = false;
    //static bool orbit = true;

    static unsigned int environment_index = 0;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene02::Init() {
        this->title = "IBL";

        auto& model_path   = utils::paths::model;
        auto& shader_path  = utils::paths::shader;
        auto& texture_path = utils::paths::texture;

        //asset_ref<Shader> pbr_shader    = LoadAsset<Shader>(shader_path + "scene_01\\pbr.glsl");
        //asset_ref<Shader> light_shader  = LoadAsset<Shader>(shader_path + "light_source.glsl");
        asset_ref<Shader> test_shader = LoadAsset<Shader>(shader_path + "scene_02\\reflect.glsl");
        asset_ref<Shader> skybox_shader = LoadAsset<Shader>(shader_path + "scene_02\\skybox.glsl");

        //asset_ref<Material> pbr_material    = CreateAsset<Material>(pbr_shader);
        //asset_ref<Material> light_material  = CreateAsset<Material>(light_shader);
        asset_ref<Material> test_material = CreateAsset<Material>(test_shader);
        asset_ref<Material> skybox_material = CreateAsset<Material>(skybox_shader);

        //blur_shader       = std::make_unique<Shader>(shader_path + "scene_01\\blur.glsl");
        //blend_shader      = std::make_unique<Shader>(shader_path + "scene_01\\blend.glsl");

        //asset_ref<Texture> checkerboard = LoadAsset<Texture>(texture_path + "common\\checkboard.png");
        
        //std::vector<asset_ref<Texture>> motorcycle {
        //    LoadAsset<Texture>(model_path + "runestone\\platform_albedo.png"),
        //    LoadAsset<Texture>(model_path + "runestone\\platform_normal.png"),
        //    LoadAsset<Texture>(model_path + "runestone\\platform_metallic.png"),
        //    LoadAsset<Texture>(model_path + "runestone\\platform_roughness.png"),
        //    nullptr,  // runestone platform does not have AO map, use a null placeholder
        //    LoadAsset<Texture>(model_path + "runestone\\platform_emissive.png")
        //};

        //irradiance_map = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, 32, 32, GL_RGB16F, 1);  // empty irradiance map, no mipmaps
        //prefilter_env_map = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, 512, 512, GL_RGB16F, 0);  // empty environment map, with mipmaps
        //brdf_lut = std::make_unique<Texture>(GL_TEXTURE_2D, 256, 256, GL_RG16F, 1);  // empty BRDF lookup texture, no mipmaps

        CheckGLError(0);

        //AddUBO(pbr_shader->GetID());
        //AddUBO(light_shader->GetID());
        AddUBO(skybox_shader->GetID());

        FBO& precompute_framebuffer = AddFBO(32, 32);  // precompute irradiance map framebuffer
        precompute_framebuffer.AddDepStRenderBuffer();

        CheckGLError(1);

        PrecomputeIrradianceMap();

        CheckGLError(2);

        //FBO& bloom_filter_pass = AddFBO(Window::width, Window::height);
        //bloom_filter_pass.AddColorTexture(2, true);    // multisampled textures for MSAA
        //bloom_filter_pass.AddDepStRenderBuffer(true);  // multisampled RBO for MSAA

        //FBO& msaa_resolve_pass = AddFBO(Window::width, Window::height);
        //msaa_resolve_pass.AddColorTexture(2);

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 6.0f, 16.0f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(glm::vec3(1.0f, 0.553f, 0.0f), 3.8f);  // attach a flashlight
        camera.GetComponent<Spotlight>().SetCutoff(4.0f);
        
        // skybox
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(skybox_material); true) {
            mat.SetTexture(0, environments[environment_index]);
            //mat.SetUniform(3, skybox_brightness, true);
        }

        // shared spherical mesh data
        buffer_ref<VAO> shared_vao = nullptr;
        size_t n_verts = 0;

        // ball x 3 (with textures)
        const float ball_posx[] = { 0.0f, -1.5f, 1.5f };
        const float ball_posy[] = { 10.5f, 7.5f, 7.5f };

        for (int i = 0; i < 3; i++) {
            ball[i] = CreateEntity("Ball " + std::to_string(i));
            ball[i].GetComponent<Transform>().Translate(world::right * ball_posx[i]);
            ball[i].GetComponent<Transform>().Translate(world::up * ball_posy[i]);

            if (i == 0) {
                Mesh& mesh = ball[0].AddComponent<Mesh>(Primitive::Sphere);
                shared_vao = mesh.GetVAO();
                n_verts = mesh.n_verts;
            }
            else {
                ball[i].AddComponent<Mesh>(shared_vao, n_verts);
            }

            if (auto& mat = ball[i].AddComponent<Material>(test_material); true) {
                mat.SetTexture(0, irradiance_maps[environment_index]);
                //mat.SetTexture(1, ball_normal);
                //mat.SetTexture(2, ball_metallic);
                //mat.SetTexture(3, ball_roughness);
            }
        }

        // sphere x 7 (without textures)
        const float sphere_posx[] = { -3.0f, 0.0f, 3.0f, -4.5f, -1.5f, 1.5f, 4.5f };
        const float sphere_posy[] = { 4.5f, 4.5f, 4.5f, 1.5f, 1.5f, 1.5f, 1.5f };

        for (int i = 0; i < 7; i++) {
            sphere[i] = CreateEntity("Sphere " + std::to_string(i));
            sphere[i].AddComponent<Mesh>(shared_vao, n_verts);
            sphere[i].GetComponent<Transform>().Translate(world::right * sphere_posx[i]);
            sphere[i].GetComponent<Transform>().Translate(world::up * sphere_posy[i]);

            if (auto& mat = sphere[i].AddComponent<Material>(test_material); true) {
                mat.SetTexture(0, irradiance_maps[environment_index]);
                //mat.SetUniform(10, sphere_albedo, true);
                //mat.SetUniform(11, sphere_metalness, true);
                //mat.SetUniform(12, sphere_roughness, true);
                //mat.SetUniform(13, sphere_ao, true);
            }
        }

        //plane = CreateEntity("Plane");
        //plane.AddComponent<Mesh>(Primitive::Plane);
        //plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        //plane.GetComponent<Transform>().Scale(3.0f);
        //if (auto& mat = plane.AddComponent<Material>(pbr_material); true) {
        //    mat.SetUniform(12, plane_roughness, true);
        //    mat.SetTexture(0, checkerboard);
        //}

        //motor = CreateEntity("Motorcycle");
        //motor.GetComponent<Transform>().Scale(0.2f);
        //motor.GetComponent<Transform>().Translate(world::up * -4.0f);
        //motor.AddComponent<Material>(test_material);
        //auto& model = motor.AddComponent<Model>(model_path + "ada\\117.fbx", Quality::Auto);
        ////model.Import("pillars", runestone_pillar);
        ////model.Import("platform", runestone_platform);
        //model.Report();  // a report that helps you learn how to load the model asset

        CheckGLError(33);

        Renderer::FaceCulling(true);
        
        
    }

    void Scene02::OnSceneRender() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        // update camera's uniform buffer
        if (auto& ubo = UBOs[0]; true) {
            auto& main_camera = camera.GetComponent<Camera>();
            ubo.Bind();
            ubo.SetData(0, val_ptr(main_camera.T->position));
            ubo.SetData(1, val_ptr(main_camera.T->forward));
            ubo.SetData(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetData(3, val_ptr(main_camera.GetProjectionMatrix()));
            ubo.Unbind();
        }

        Renderer::DepthTest(true);
        Renderer::Clear();
        Renderer::Submit(ball[0].id, ball[1].id, ball[2].id);
        Renderer::Submit(sphere[0].id, sphere[1].id, sphere[2].id, sphere[3].id, sphere[4].id, sphere[5].id, sphere[6].id);
        Renderer::Submit(skybox.id);
        Renderer::Render();
    }

    void Scene02::OnImGuiRender() {
        //static ImVec4 sphere_color = ImVec4(0.22f, 0.0f, 1.0f, 0.0f);
        //static ImVec4 flashlight_color = ImVec4(1.0f, 0.553f, 0.0f, 1.0f);
        //static const ImVec4 text_color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);

        //static ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoSidePreview
        //    | ImGuiColorEditFlags_PickerHueWheel
        //    | ImGuiColorEditFlags_DisplayRGB
        //    | ImGuiColorEditFlags_NoPicker;

        //static const char* tone_mappings[] = {
        //    "Simple Reinhard",
        //    // "Extended Reinhard (Luminance)",
        //    "Reinhard-Jodie (Shadertoy)",
        //    "Uncharted 2 Hable Filmic",
        //    "Approximated ACES (Unreal Engine 4)"
        //};

        //static const char tone_mapping_guide[] = "Here is a number of algorithms for high dynamic "
        //    "range tone mapping, select one that looks best on your scene.";

        //static const char tone_mapping_note[] = "Approximated ACES filmic tonemapper might not be "
        //    "the most visually attractive one, but it is the most physically correct.";

        ui::LoadInspectorConfig();

        if (ImGui::Begin("Inspector##2", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            if (ImGui::BeginTabBar("InspectorTab", ImGuiTabBarFlags_None)) {

                if (ImGui::BeginTabItem("Scene")) {
                    ImGui::Indent(5.0f);
                    if (ImGui::Button("  Next Skybox  ")) {
                        ChangeEnvironment();
                    }
                    ImGui::Unindent(5.0f);
                    ImGui::EndTabItem();
                }

        //        if (ImGui::BeginTabItem("Material")) {
        //            ImGui::Indent(5.0f);
        //            ImGui::Unindent(5.0f);
        //            ImGui::EndTabItem();
        //        }

        //        if (ImGui::BeginTabItem("HDR")) {
        //            ImGui::Indent(5.0f);
        //            ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        //            ImGui::PushTextWrapPos(280.0f);
        //            ImGui::TextWrapped(tone_mapping_guide);
        //            ImGui::TextWrapped(tone_mapping_note);
        //            ImGui::PopTextWrapPos();
        //            ImGui::PopStyleColor();
        //            ImGui::Separator();
        //            ImGui::PushItemWidth(295.0f);
        //            ImGui::Combo(" ", &tone_mapping_mode, tone_mappings, 4);
        //            ImGui::PopItemWidth();
        //            ImGui::Unindent(5.0f);
        //            ImGui::EndTabItem();
        //        }

                ImGui::EndTabBar();
            }
        }

        ImGui::End();

    }

    void Scene02::PrecomputeIrradianceMap() {
        Renderer::SeamlessCubemap(true);

        using namespace glm;
        static const mat4 projection = glm::perspective(radians(90.0f), 1.0f, 0.1f, 10.0f);
        static const mat4 views[6] = {
            glm::lookAt(vec3(0.0f), vec3(+1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),  // posx
            glm::lookAt(vec3(0.0f), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),  // negx
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)),  // posy
            glm::lookAt(vec3(0.0f), vec3(+0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)),  // negy
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)),  // posz
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))   // negz
        };

        std::string& shader_path = utils::paths::shader;
        std::string& texture_path = utils::paths::texture;

        irradiance_shader = std::make_unique<Shader>(shader_path + "scene_02\\irradiance.glsl");

        //std::string hdri[2] = { "CasualDay4K.hdr", "DayInTheClouds4k.hdr" };
        std::string hdri[2] = { "CasualDay4K.hdr", "CasualDay4K.hdr" };

        FBO& precompute_framebuffer = FBOs[0];

        auto virtual_cube = Mesh(Primitive::Cube);
        Renderer::SetViewport(32, 32);

        for (unsigned int i = 0; i < 2; i++) {
            environments[i] = LoadAsset<Texture>(texture_path + "test\\" + hdri[i], 2048, 1);
            irradiance_maps[i] = CreateAsset<Texture>(GL_TEXTURE_CUBE_MAP, 32, 32, GL_RGBA16F, 1);  // no mipmaps

            precompute_framebuffer.Bind();
            irradiance_shader->Bind();

            environments[i]->Bind(0);
            irradiance_shader->SetUniform(1, projection);

            for (GLuint face = 0; face < 6; face++) {
                precompute_framebuffer.SetColorTexture(0, irradiance_maps[i]->GetID(), face);
                precompute_framebuffer.Clear(0);
                precompute_framebuffer.Clear(-1);
                irradiance_shader->SetUniform(0, views[face]);
                virtual_cube.Draw();
            }

            irradiance_shader->Unbind();
            precompute_framebuffer.Unbind();
        }
        Renderer::SetViewport(Window::width, Window::height);
    }

    void Scene02::ChangeEnvironment() {
        if (environment_index == 1) {
            environment_index = 0;
        }
        else {
            environment_index++;
        }
        
        skybox.GetComponent<Material>().SetTexture(0, environments[environment_index]);
        for (int i = 0; i < 3; i++) {
            ball[i].GetComponent<Material>().SetTexture(0, irradiance_maps[environment_index]);
        }
        for (int i = 0; i < 7; i++) {
            sphere[i].GetComponent<Material>().SetTexture(0, irradiance_maps[environment_index]);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    //void Scene02::UpdateEntities() {
    //    auto& main_camera = camera.GetComponent<Camera>();
    //    main_camera.Update();

    //    // rotate the orbit light
    //    if (orbit) {
    //        auto& transform = orbit_light.GetComponent<Transform>();
    //        float x = orbit_radius * sin(orbit_time * orbit_speed);
    //        float z = orbit_radius * cos(orbit_time * orbit_speed);
    //        float y = transform.position.y;
    //        orbit_time += Clock::delta_time;
    //        transform.Translate(glm::vec3(x, y, z) - transform.position);
    //    }
    //}

    //void Scene02::UpdateUBOs() {
    //    // update camera's uniform buffer
    //    if (auto& ubo = UBOs[0]; true) {
    //        auto& main_camera = camera.GetComponent<Camera>();
    //        ubo.Bind();
    //        ubo.SetData(0, val_ptr(main_camera.T->position));
    //        ubo.SetData(1, val_ptr(main_camera.T->forward));
    //        ubo.SetData(2, val_ptr(main_camera.GetViewMatrix()));
    //        ubo.SetData(3, val_ptr(main_camera.GetProjectionMatrix()));
    //        ubo.Unbind();
    //    }

    //    // update spotlight's uniform buffer
    //    if (auto& ubo = UBOs[2]; true) {
    //        auto& spotlight = camera.GetComponent<Spotlight>();
    //        auto& transform = camera.GetComponent<Transform>();
    //        float inner_cos = spotlight.GetInnerCosine();
    //        float outer_cos = spotlight.GetOuterCosine();
    //        ubo.Bind();
    //        ubo.SetData(0, val_ptr(spotlight.color));
    //        ubo.SetData(1, val_ptr(transform.position));
    //        ubo.SetData(2, val_ptr(transform.forward));
    //        ubo.SetData(3, &(spotlight.intensity));
    //        ubo.SetData(4, &(inner_cos));
    //        ubo.SetData(5, &(outer_cos));
    //        ubo.SetData(6, &(spotlight.range));
    //        ubo.Unbind();
    //    }

    //    // update orbit light's uniform buffer
    //    if (auto& ubo = UBOs[3]; true) {
    //        auto& ol = orbit_light.GetComponent<PointLight>();
    //        auto& pos = orbit_light.GetComponent<Transform>().position;
    //        ubo.Bind();
    //        ubo.SetData(0, val_ptr(ol.color));
    //        ubo.SetData(1, val_ptr(pos));
    //        ubo.SetData(2, &(ol.intensity));
    //        ubo.SetData(3, &(ol.linear));
    //        ubo.SetData(4, &(ol.quadratic));
    //        ubo.SetData(5, &(ol.range));
    //        ubo.Unbind();
    //    }

    //    // update uniform buffer for the cluster of 28 point lights
    //    if (auto& ubo = UBOs[4]; true) {
    //        auto& pl = point_lights[0].GetComponent<PointLight>();
    //        ubo.Bind();
    //        ubo.SetData(0, &(light_cluster_intensity));
    //        ubo.SetData(1, &(pl.linear));
    //        ubo.SetData(2, &(pl.quadratic));
    //        ubo.Unbind();
    //    }
    //}

    //void Scene02::UpdateUniforms() {
    //    if (auto& mat = sphere.GetComponent<Material>(); true) {
    //        for (int i = 3; i <= 9; i++) {
    //            mat.SetUniform(i, 0);  // sphere doesn't have any PBR textures
    //        }
    //    }

    //    if (auto& mat = ball.GetComponent<Material>(); true) {
    //        for (int i = 3; i <= 6; i++) {
    //            mat.SetUniform(i, 1);
    //        }
    //        mat.SetUniform(7, 0);
    //        mat.SetUniform(8, 0);
    //        mat.SetUniform(9, 0);
    //        mat.SetUniform(13, 1.0f);  // ambient occlussion
    //        mat.SetUniform(14, glm::vec2(1.6f, 0.9f));  // uv scale
    //    }

    //    if (auto& mat = plane.GetComponent<Material>(); true) {
    //        mat.SetUniform(3, 1);  // plane only has an albedo map
    //        for (int i = 4; i <= 9; i++) {
    //            mat.SetUniform(i, 0);
    //        }
    //        mat.SetUniform(11, 0.1f);  // metalness
    //        mat.SetUniform(13, 1.0f);  // ambient occlussion
    //        mat.SetUniform(14, glm::vec2(8.0f));  // uv scale
    //    }

    //    if (auto& mat = runestone.GetComponent<Material>(); true) {
    //        for (int i = 3; i <= 6; i++) {
    //            mat.SetUniform(i, 1);
    //        }
    //        mat.SetUniform(7, 0);      // runestone does not have AO map
    //        mat.SetUniform(8, 1);      // runestone does have an emission map
    //        mat.SetUniform(9, 0);      // runestone does not use displacement map
    //        mat.SetUniform(13, 1.0f);  // specify ambient occlussion explicitly
    //        mat.SetUniform(14, glm::vec2(1.0f));  // uv scale
    //    }

    //    if (auto& mat = orbit_light.GetComponent<Material>(); true) {
    //        auto& pl = orbit_light.GetComponent<PointLight>();
    //        mat.SetUniform(3, pl.color);
    //        mat.SetUniform(4, pl.intensity);
    //        mat.SetUniform(5, 2.0f);  // bloom multiplier (color saturation)
    //    }

    //    for (int i = 0; i < 28; i++) {
    //        auto& mat = point_lights[i].GetComponent<Material>();
    //        auto& pl = point_lights[i].GetComponent<PointLight>();
    //        mat.SetUniform(3, pl.color);
    //        mat.SetUniform(4, pl.intensity);
    //        mat.SetUniform(5, 7.0f);  // bloom multiplier (color saturation)
    //    }
    //}

    ///////////////////////////////////////////////////////////////////////////////////////////////

    //void Scene02::DepthPrepass() {
    //    auto& depth_prepass_framebuffer = FBOs[0];
    //    depth_prepass_framebuffer.Bind();
    //    depth_prepass_framebuffer.Clear(-1);

    //    // the written depth values are only used for light culling in the next pass,
    //    // we can skip the 28 light sources here because shading of the static light
    //    // cube itself is not affected by any other incoming lights.

    //    Renderer::DepthTest(true);
    //    Renderer::DepthPrepass(true);  // enable early z-test
    //    Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
    //    Renderer::Submit(orbit_light.id);
    //    Renderer::Submit(skybox.id);
    //    Renderer::Render();
    //    Renderer::DepthPrepass(false);  // disable early z-test

    //    depth_prepass_framebuffer.Unbind();
    //}

    //void Scene02::BloomFilterPass() {
    //    FBO& bloom_filter_framebuffer = FBOs[1];
    //    bloom_filter_framebuffer.ClearAll();
    //    bloom_filter_framebuffer.Bind();

    //    Renderer::MSAA(true);  // make sure to enable MSAA before actual rendering
    //    Renderer::DepthTest(true);
    //    Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
    //    Renderer::Submit(orbit_light.id);
    //    Renderer::Submit(skybox.id);

    //    if (show_light_cluster) {
    //        for (int i = 0; i < n_point_lights; i++) {
    //            Renderer::Submit(point_lights[i].id);
    //        }
    //    }

    //    Renderer::Render();

    //    bloom_filter_framebuffer.Unbind();
    //}

    //void Scene01::MSAAResolvePass() {
    //    FBO& source = FBOs[1];
    //    FBO& target = FBOs[2];
    //    target.ClearAll();

    //    FBO::TransferColor(source, 0, target, 0);
    //    FBO::TransferColor(source, 1, target, 1);
    //}

    //void Scene02::BloomBlendPass() {
    //    // finally we are back on the default framebuffer
    //    auto& original_texture = FBOs[2].GetColorTexture(0);
    //    auto& blurred_texture = FBOs[3].GetColorTexture(0);
    //    
    //    original_texture.Bind(0);
    //    blurred_texture.Bind(1);
    //    bilinear_sampler->Bind(1);  // bilinear filtering will introduce extra blurred effects

    //    if (blend_shader->Bind(); true) {
    //        blend_shader->SetUniform(0, tone_mapping_mode);
    //        Renderer::Clear();
    //        Renderer::DrawQuad();
    //        blend_shader->Unbind();
    //    }

    //    bilinear_sampler->Unbind(0);
    //    bilinear_sampler->Unbind(1);
    //}

}
