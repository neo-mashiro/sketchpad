#include "pch.h"

#include "core/clock.h"
#include "core/input.h"
#include "core/window.h"
#include "buffer/fbo.h"
#include "buffer/ubo.h"
#include "buffer/ssbo.h"
#include "components/all.h"
#include "components/component.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/math.h"
#include "utils/path.h"

#include "examples/scene_01.h"

using namespace core;
using namespace buffer;
using namespace components;

namespace scene {

    static bool show_plane = true;
    static bool orbit = true;
    static float orbit_time = 0.0f;
    static float orbit_speed = 2.0f;  // in radians per second
    static float orbit_radius = 3.2f;

    static float sphere_metalness = 0.0f;
    static float sphere_roughness = 0.0f;
    static float sphere_ao = 1.0f;

    static bool draw_depth_buffer = false;

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        // name your scene title (will appear in the top menu)
        this->title = "Example Scene";

        // load shader and texture assets upfront
        //this->depth_prepass_shader        = LoadAsset<Shader>(SHADER + "depth_prepass.glsl");
        this->light_cull_compute_shader   = LoadAsset<ComputeShader>(SHADER + "light_cull.glsl");

        asset_ref<Shader> pbr_shader      = LoadAsset<Shader>(SHADER + "01_pbr.glsl");
        asset_ref<Shader> skybox_shader   = LoadAsset<Shader>(SHADER + "skybox.glsl");
        asset_ref<Texture> space_cube     = LoadAsset<Texture>(GL_TEXTURE_CUBE_MAP, SKYBOX + "space");
        asset_ref<Texture> checkerboard   = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "misc\\checkboard.png");
        asset_ref<Texture> ball_albedo    = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\albedo.jpg");
        asset_ref<Texture> ball_normal    = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\normal.jpg");
        asset_ref<Texture> ball_metallic  = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\metallic.jpg");
        asset_ref<Texture> ball_roughness = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\roughness.jpg");
        asset_ref<Texture> ball_AO        = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\AO.jpg");

        std::vector<asset_ref<Texture>> runestone_pillar {
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\pillars_albedo.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\pillars_normal.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\pillars_metallic.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\pillars_roughness.png")
        };

        std::vector<asset_ref<Texture>> runestone_platform {
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\platform_albedo.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\platform_normal.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\platform_metallic.png"),
            LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\platform_roughness.png")
        };

        // create uniform buffer objects (UBO) from shaders
        AddUBO(pbr_shader->id);

        // create frame buffer objects (FBO)
        FBO& depth_prepass = AddFBO(1, Window::width, Window::height);
        //depth_prepass.GetVirtualMaterial()->SetShader(depth_prepass_shader);

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, 4.0f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(color::red, 1.8f);  // attach a spotlight to the camera (flashlight)
        camera.GetComponent<Spotlight>().SetCutoff(5.0f);
        
        // skybox
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        skybox.GetComponent<Material>().SetShader(skybox_shader);
        skybox.GetComponent<Material>().SetTexture(0, space_cube);

        // sphere
        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(Primitive::Sphere);
        sphere.GetComponent<Transform>().Translate(world::up * 7.0f);
        sphere.GetComponent<Transform>().Scale(2.0f);
        if (auto& mat = sphere.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            for (int i = 3; i <= 7; i++) {
                mat.SetUniform(i, 0);  // sphere does not have PBR textures
            }
            mat.SetUniform(8, glm::vec3(0.635f, 0.0f, 1.0f));  // albedo (diffuse color)
            // you can bind a uniform to a variable and observe changes in the shader automatically
            mat.SetUniform(9, sphere_metalness, true);
            mat.SetUniform(10, sphere_roughness, true);
            mat.SetUniform(11, sphere_ao, true);
        }

        // metallic ball
        ball = CreateEntity("Ball");
        Mesh& sphere_mesh = sphere.GetComponent<Mesh>();  // reuse sphere's vertices array (VAO)
        ball.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        ball.GetComponent<Transform>().Translate(world::up * 2.2f);
        ball.GetComponent<Transform>().Scale(2.0f);
        if (auto& mat = ball.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            for (int i = 3; i <= 7; i++) {
                mat.SetUniform(i, 1);  // ball does have all PBR textures
            }
            mat.SetTexture(0, ball_albedo);
            mat.SetTexture(1, ball_normal);
            mat.SetTexture(2, ball_metallic);
            mat.SetTexture(3, ball_roughness);
            mat.SetTexture(4, ball_AO);
            mat.SetUniform(12, 4.0f);  // uv scale
            if (false) {
                mat.SetShader(nullptr);      // this is how you can reset a material's shader
                mat.SetTexture(0, nullptr);  // this is how you can reset a material's texture
            }
        }

        // plane
        plane = CreateEntity("Plane");
        plane.AddComponent<Mesh>(Primitive::Plane);
        plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        plane.GetComponent<Transform>().Scale(3.0f);
        if (auto& mat = plane.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            mat.SetUniform(3, 1);  // plane only has an albedo_map
            mat.SetUniform(4, 0);  // no normal_map
            mat.SetUniform(5, 0);  // no metallic_map
            mat.SetUniform(6, 0);  // no roughness_map
            mat.SetUniform(7, 0);  // no ao_map
            mat.SetTexture(0, checkerboard);
            mat.SetUniform(9, 0.0f);   // metalness
            mat.SetUniform(10, 0.0f);  // roughness
            mat.SetUniform(11, 1.0f);  // ambient occlussion
            mat.SetUniform(12, 20.0f);  // uv scale
        }

        // runestone model
        runestone = CreateEntity("Runestone");
        runestone.GetComponent<Transform>().Scale(0.01f);
        runestone.GetComponent<Transform>().Translate(world::up * -4.0f);
        auto& model = runestone.AddComponent<Model>(MODEL + "runestone\\runestone.fbx", Quality::Auto);

        model.Import("pillars", runestone_pillar);  // 6
        model.Import("platform", runestone_platform);  // 5
        model.Report();

        if (auto& mat = runestone.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            mat.SetUniform(3, 1);
            mat.SetUniform(4, 1);
            mat.SetUniform(5, 1);
            mat.SetUniform(6, 1);
            mat.SetUniform(7, 0);  // runestone does not have ao_map
            mat.SetUniform(11, 1.0f);  // ambient occlussion
        }

        // directional light (for lights that are static, we only need to set its UBO data once)
        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(glm::radians(-45.0f), world::right);
        direct_light.AddComponent<DirectionLight>(color::yellow, 0.25f);

        if (auto& ubo = UBOs[1]; true) {
            ubo.Bind();
            ubo.SetData(0, val_ptr(direct_light.GetComponent<DirectionLight>().color));
            ubo.SetData(1, val_ptr(direct_light.GetComponent<Transform>().forward * (-1.0f)));
            ubo.SetData(2, &(direct_light.GetComponent<DirectionLight>().intensity));
            ubo.Unbind();
        }

        // orbit light
        orbit_light = CreateEntity("Orbit Light");
        orbit_light.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, orbit_radius));
        orbit_light.AddComponent<PointLight>(color::white, 0.8f);
        orbit_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

        // forward+ rendering setup
        nx = (Window::width + tile_size - 1) / tile_size;
        ny = (Window::height + tile_size - 1) / tile_size;
        n_tiles = nx * ny;

        // setup the shader storage buffers for 28 point lights
        const unsigned int n_pl = 28;
        pl_color_ssbo    = LoadBuffer<SSBO<glm::vec4>>(n_pl, 0);
        pl_position_ssbo = LoadBuffer<SSBO<glm::vec4>>(n_pl, 1);
        pl_range_ssbo    = LoadBuffer<SSBO<GLfloat>>(n_pl, 2);
        pl_index_ssbo    = LoadBuffer<SSBO<GLint>>(n_pl * n_tiles, 3);

        light_cull_compute_shader->Bind();
        light_cull_compute_shader->SetUniform(0, n_pl);

        // light culling in forward+ rendering can be applied to both static and dynamic lights,
        // in the latter case, it is required that users update the input SSBO buffer data every
        // frame. In most cases, perform light culling on static lights alone is already enough,
        // unless you have thousands of lights whose colors or positions are constantly changing.
        // in this demo, we will only cull the 28 (or even 2800 if you have space) static point
        // lights, so the input SSBO buffer data only needs to be set up once. The spotlight and
        // orbit light will participate in lighting calculations anyway.

        std::vector<glm::vec4> colors {};
        std::vector<glm::vec4> positions {};
        std::vector<GLfloat> ranges {};

        // make a grid of size 8 x 8 (64 cells), sample each border cell to be a point light.
        // thus we would have a total of 28 point lights that surrounds our scene, which are
        // evenly distributed on the boundaries of the plane.

        for (int i = 0, index = 0; i < 64; i++) {
            int row = i / 8;  // ~ [0, 7]
            int col = i % 8;  // ~ [0, 7]

            if (bool on_boundary = (row == 0 || row == 7 || col == 0 || col == 7); !on_boundary) {
                continue;  // skip cells in the middle
            }

            // translate light positions to the range [-3, 3] (centered at origin)
            auto position = glm::vec3(row - 3.5f, 2.5f, col - 3.5f) * 0.857f;
            auto rand_color = glm::vec3(0.0f);

            // for each point light, generate a random color that is not too dark
            while (glm::length2(rand_color) < 1.5f) {
                rand_color = glm::vec3(RandomFloat01(), RandomFloat01(), RandomFloat01());
            }

            point_lights[index] = CreateEntity("Point Light " + std::to_string(index));
            auto& pl = point_lights[index];
            pl.GetComponent<Transform>().Translate(position - world::origin);
            pl.AddComponent<PointLight>(rand_color, 0.8f);
            pl.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

            // the effective range of the light is calculated for you by `SetAttenuation()`
            colors.push_back(glm::vec4(rand_color, 1.0f));
            positions.push_back(glm::vec4(position, 1.0f));
            ranges.push_back(pl.GetComponent<PointLight>().range);

            index++;
        }

        pl_color_ssbo->Write(colors);
        pl_color_ssbo->Bind();

        pl_position_ssbo->Write(positions);
        pl_position_ssbo->Bind();

        pl_range_ssbo->Write(ranges);
        pl_range_ssbo->Bind();

        Renderer::FaceCulling(true);
    }

    // this is called every frame, update your scene here and submit the entities to the renderer
    void Scene01::OnSceneRender() {
        (void)glGetError();
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        // update camera's uniform buffer
        if (auto& ubo = UBOs[0]; true) {
            ubo.Bind();
            ubo.SetData(0, val_ptr(main_camera.T->position));
            ubo.SetData(1, val_ptr(main_camera.T->forward));
            ubo.SetData(2, val_ptr(main_camera.GetViewMatrix()));
            ubo.SetData(3, val_ptr(main_camera.GetProjectionMatrix()));
            ubo.Unbind();
        }

        // update spotlight's uniform buffer
        if (auto& ubo = UBOs[2]; true) {
            auto& spotlight = camera.GetComponent<Spotlight>();
            auto& transform = camera.GetComponent<Transform>();
            float inner_cos = spotlight.GetInnerCosine();
            float outer_cos = spotlight.GetOuterCosine();
            ubo.Bind();
            ubo.SetData(0, val_ptr(spotlight.color));
            ubo.SetData(1, val_ptr(transform.position));
            ubo.SetData(2, val_ptr(transform.forward));
            ubo.SetData(3, &(spotlight.intensity));
            ubo.SetData(4, &(inner_cos));
            ubo.SetData(5, &(outer_cos));
            ubo.SetData(6, &(spotlight.range));
            ubo.Unbind();
        }

        // update orbit light's uniform buffer
        if (auto& ubo = UBOs[3]; true) {
            auto& ol = orbit_light.GetComponent<PointLight>();
            ubo.Bind();
            ubo.SetData(0, val_ptr(ol.color));
            ubo.SetData(1, val_ptr(orbit_light.GetComponent<Transform>().position));
            ubo.SetData(2, &(ol.intensity));
            ubo.SetData(3, &(ol.linear));
            ubo.SetData(4, &(ol.quadratic));
            ubo.SetData(5, &(ol.range));
            ubo.Unbind();
        }

        // update the orbit light
        if (orbit) {
            auto& transform = orbit_light.GetComponent<Transform>();
            float x = orbit_radius * sin(orbit_time * orbit_speed);
            float y = orbit_radius * cos(orbit_time * orbit_speed);
            orbit_time += Clock::delta_time;
            transform.Translate(glm::vec3(x, 0.0f, y) - transform.position);
        }

        // a demo of how to do physically-based shading in the context of tiled forward renderers
        // pass 1: render depth values into the prepass depth buffer
        auto& depth_prepass = FBOs[0];
        depth_prepass.Bind();
        Renderer::DepthTest(true);
        Renderer::DepthPrepass(true);  // enable early z-test
        Renderer::Clear();
        Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id, skybox.id);
        Renderer::Render();
        depth_prepass.Unbind();

        if (draw_depth_buffer) {
            Renderer::DepthTest(false);
            Renderer::Clear();
            depth_prepass.DebugDraw(-1);
            return;
        }

        // pass 2: light culling on the compute shader. In this pass we only run the compute
        // shader and update the SSBO, there's no rendering operations involved...
        light_cull_compute_shader->Bind();
        depth_prepass.BindBuffer(-1, 0);  // bind the depth buffer
        pl_index_ssbo->Clear();  // recalculate light indices every frame
        pl_index_ssbo->Bind();
        light_cull_compute_shader->Dispatch(nx, ny, 1);
        CheckGLError(3);

        // wait until the compute shader finishes and then unbind the states. Note that this
        // sync step should ideally be placed closest to the code that actually uses the SSBO
        // in order to avoid unnecessary waits, but in this demo we need it right away...
        light_cull_compute_shader->SyncWait();
        light_cull_compute_shader->Unbind();
        depth_prepass.UnbindBuffer(0);  // unbind the depth buffer
        CheckGLError(4);

        // pass 3: render objects as you normally do, but this time using the SSBOs to look up
        // visible lights rather than looping through every light source in the fragment shader
        Renderer::DepthTest(true);
        Renderer::DepthPrepass(false);
        Renderer::MSAA(true);  // unlike deferred rendering, built-in MSAA will work fine here
        Renderer::Clear();
        Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id, skybox.id);
        Renderer::Render();
        CheckGLError(5);

        // optionally you can include another pass for postprocessing stuff (HDR, Bloom, Blur Effects, etc)
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        static bool sphere_gizmo = false;
        static bool plane_gizmo = false;
        static bool edit_sphere_albedo = false;
        static bool edit_flashlight_color = false;

        static ImVec4 sphere_albedo = ImVec4(0.635f, 0.0f, 1.0f, 0.0f);
        static ImVec4 flashlight_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        static ImGuiColorEditFlags color_flags
            = ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_NoPicker;

        ui::LoadInspectorConfig();

        if (ImGui::Begin("Inspector##1", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Checkbox("Show Plane", &show_plane);
            ImGui::Separator();
            ImGui::Checkbox("Orbit Light", &orbit);
            ImGui::Separator();
            ImGui::Checkbox("Show Sphere Gizmo", &sphere_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Show Plane Gizmo", &plane_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Visualize Depth Buffer", &draw_depth_buffer);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushItemWidth(100.0f);
            ImGui::SliderFloat("Sphere Metalness", &sphere_metalness, 0.05f, 1.0f);
            ImGui::SliderFloat("Sphere Roughness", &sphere_roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Sphere AO", &sphere_ao, 0.0f, 1.0f);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::Checkbox("Edit Sphere Albedo", &edit_sphere_albedo);
            if (edit_sphere_albedo) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Sphere Albedo", (float*)&sphere_albedo, color_flags);
                ImGui::Unindent(15.0f);
            }

            ImGui::Checkbox("Edit Flashlight Color", &edit_flashlight_color);
            if (edit_flashlight_color) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Flashlight Color", (float*)&flashlight_color, color_flags);
                ImGui::Unindent(15.0f);
            }
        }

        ImGui::End();

        if (sphere_gizmo) {
            ui::DrawGizmo(camera, sphere);
        }

        if (plane_gizmo && show_plane) {
            ui::DrawGizmo(camera, plane);
        }

        if (auto& mat = sphere.GetComponent<Material>(); true) {
            mat.SetUniform(8, glm::vec3(sphere_albedo.x, sphere_albedo.y, sphere_albedo.z));
        }

        if (auto& sl = camera.GetComponent<Spotlight>(); true) {
            sl.color = glm::vec3(flashlight_color.x, flashlight_color.y, flashlight_color.z);
        }

        //ImGui::ShowDemoWindow();
    }
}
