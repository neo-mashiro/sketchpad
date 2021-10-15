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

    static bool draw_depth_buffer = false;

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        // name your scene title (will appear in the top menu)
        this->title = "Example Scene";

        // load shader and texture assets upfront
        depth_prepass_shader      = LoadAsset<Shader>(SHADER + "depth_prepass.glsl");
        light_cull_compute_shader = LoadAsset<ComputeShader>(SHADER + "light_cull.glsl");

        asset_ref<Shader> pbr_shader = LoadAsset<Shader>(SHADER + "01_pbr.glsl");
        asset_ref<Shader> pbr_shader_t = LoadAsset<Shader>(SHADER + "01_pbr_textured.glsl");
        asset_ref<Shader> skybox_shader      = LoadAsset<Shader>(SHADER + "skybox.glsl");

        asset_ref<Shader> blinn_phong_shader       = LoadAsset<Shader>(SHADER + "01_plane.glsl");

        asset_ref<Texture> skybox_cubemap = LoadAsset<Texture>(GL_TEXTURE_CUBE_MAP, SKYBOX + "space");
        asset_ref<Texture> checkerboard   = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "misc\\checkboard.png");
        asset_ref<Texture> ball_albedo    = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\albedo.jpg");
        asset_ref<Texture> ball_normal    = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\normal.jpg");
        asset_ref<Texture> ball_metallic  = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\metallic.jpg");
        asset_ref<Texture> ball_roughness = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\roughness.jpg");
        asset_ref<Texture> ball_AO        = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "meshball\\AO.jpg");

        // create uniform buffer objects (UBO) from shaders
        AddUBO(pbr_shader->id);

        // create frame buffer objects (FBO)
        FBO& depth_prepass = AddFBO(0, Window::width, Window::height);
        depth_prepass.GetVirtualMaterial()->SetShader(depth_prepass_shader);

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, 15.0f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(color::red, 1.8f);  // attach a spotlight to the camera (flashlight)
        camera.GetComponent<Spotlight>().SetCutoff(5.0f);

        // skybox
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        skybox.GetComponent<Material>().SetShader(skybox_shader);
        skybox.GetComponent<Material>().SetTexture(0, skybox_cubemap);

        // sphere
        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(Primitive::Sphere);
        sphere.GetComponent<Transform>().Translate(world::up * 7.0f);
        sphere.GetComponent<Transform>().Scale(2.0f);
        if (auto& mat = sphere.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            mat.SetUniform(1, glm::vec3(0.0215f, 0.1745f, 0.0215f));
            mat.SetUniform(2, glm::vec3(0.07568f, 0.61424f, 0.07568f));
            mat.SetUniform(3, glm::vec3(0.633f, 0.727811f, 0.633f));
            mat.SetUniform(4, sphere_shininess, true);  // observe the uniform variable
        }

        // metallic ball
        ball = CreateEntity("Ball");
        Mesh& sphere_mesh = sphere.GetComponent<Mesh>();  // reuse the mesh's vertices array (VAO)
        ball.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        ball.GetComponent<Transform>().Translate(world::up * 2.2f);
        ball.GetComponent<Transform>().Scale(2.0f);
        if (auto& mat = ball.GetComponent<Material>(); true) {
            mat.SetShader(ball_shader);
            mat.SetTexture(0, ball_albedo);
            mat.SetTexture(1, ball_metallic);
            mat.SetUniform(1, plane_shininess, true);
            // this is how you can reset a material's shader or texture
            // mat.SetShader(nullptr);
            // mat.SetTexture(0, nullptr);
        }

        // plane
        plane = CreateEntity("Plane");
        plane.AddComponent<Mesh>(Primitive::Plane);
        plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        plane.GetComponent<Transform>().Scale(2.0f);
        if (auto& mat = plane.GetComponent<Material>(); true) {
            mat.SetShader(plane_shader);
            mat.SetTexture(0, checkerboard);
            mat.SetUniform(1, plane_shininess, true);
        }

        // runestone model
        //suzanne = CreateEntity("Suzanne");
        //suzanne.GetComponent<Transform>().Scale(0.01f);
        //suzanne.GetComponent<Transform>().Translate(world::up * -4.0f);
        //auto& model = suzanne.AddComponent<Model>(MODEL + "runestone\\source\\runestone_lp.fbx", Quality::Auto, true);

        //std::vector<asset_ref<Texture>> txr_refs_pillar {
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_pillars_BaseColor.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_pillars_Normal.png"),
        //    t_plane,
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_pillars_Metallic.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_pillars_Roughness.png")
        //};
        //std::vector<asset_ref<Texture>> txr_refs_platform {
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_platform_BaseColor.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_platform_Normal.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_platform_Emissive.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_platform_Metallic.png"),
        //    LoadAsset<Texture>(GL_TEXTURE_2D, MODEL + "runestone\\source\\runestone_lp1_platform_Roughness.png")
        //};

        //model.Import("Bike", txr_refs_pillar);
        //model.Import("Floor", txr_refs_platform);
        //model.Report();

        //auto& m4 = suzanne.GetComponent<Material>();
        //m4.SetShader(s_model);

        // directional light (for lights that are static, we only need to set its UBO data once)
        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(glm::radians(-45.0f), world::right);
        direct_light.AddComponent<DirectionLight>(color::white, 0.25f);

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

        // setup the shader storage buffers for 16 point lights
        const int n_pl = 16;
        light_position_ssbo = SSBO<glm::vec4>(n_pl, 0);
        light_range_ssbo    = SSBO<GLfloat>(n_pl, 1);
        light_index_ssbo    = SSBO<GLint>(n_pl * n_tiles, 2);

        light_cull_compute_shader->SetUniform(0, n_pl);

        // light culling in forward+ rendering can be applied to both static and dynamic lights,
        // in the latter case, it is required that users update the input SSBO buffer data every
        // frame. In most cases, perform light culling on static lights alone is already enough,
        // unless you have thousands of lights whose colors or positions are constantly changing.
        // in this demo, we will only cull the 16 (or even 1600 if you have space) static point
        // lights, so the input SSBO buffer data only needs to be set up once. The spotlight and
        // orbit light will participate in lighting calculations anyway.

        std::vector<glm::vec4> positions {};
        std::vector<GLfloat> ranges {};

        // make a grid of size 5 x 5 (25 cells), sample each border cell to be a point light.
        // thus we would have a total of 16 point lights that surrounds our scene, which are
        // evenly distributed on the boundaries of the plane.

        for (int i = 0, index = 0; i < 25; i++) {
            int row = i / static_cast<int>(sqrt(n_pl));  // ~ [0, 4]
            int col = i % static_cast<int>(sqrt(n_pl));  // ~ [0, 4]

            if (bool on_boundary = (row == 0 || row == 4 || col == 0 || col == 4); !on_boundary) {
                continue;  // skip cells in the middle
            }

            auto position = glm::vec3(row - 2, 2.5f, col - 2);  // ~ [-2, 2] (centered at origin)
            auto color = glm::vec3(RandomFloat01(), RandomFloat01(), RandomFloat01());

            point_lights[index] = CreateEntity("Point Light " + std::to_string(index));
            auto& pl = point_lights[index];
            pl.GetComponent<Transform>().Translate(position - world::origin);
            pl.AddComponent<PointLight>(color, 0.8f);
            pl.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

            if (auto& ubo = UBOs[index + 3]; true) {
                ubo.Bind();
                ubo.SetData(0, val_ptr(pl.GetComponent<PointLight>().color));
                ubo.SetData(1, val_ptr(pl.GetComponent<Transform>().position));
                ubo.SetData(2, &(pl.GetComponent<PointLight>().intensity));
                ubo.SetData(3, &(pl.GetComponent<PointLight>().linear));
                ubo.SetData(4, &(pl.GetComponent<PointLight>().quadratic));
                ubo.SetData(5, &(pl.GetComponent<PointLight>().range));
                ubo.Unbind();
            }

            // the effective range of the light is calculated for you by `SetAttenuation()`
            float range = point_lights[i].GetComponent<PointLight>().range;
            ranges.push_back(range);
            positions.push_back(glm::vec4(position, 1.0f));

            index++;
        }

        light_position_ssbo.Write(positions);
        light_position_ssbo.Bind(0);

        light_range_ssbo.Write(ranges);
        light_range_ssbo.Bind(1);

        Renderer::FaceCulling(true);
    }

    // this is called every frame, update your scene here and submit the entities to the renderer
    void Scene01::OnSceneRender() {
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
        if (auto& ubo = UBOs[19]; true) {
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
        Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, suzanne.id, skybox.id);
        Renderer::Render();
        depth_prepass.Unbind();

        if (draw_depth_buffer) {
            depth_prepass.DebugDraw(-1);
            return;
        }

        // pass 2: light culling on the compute shader. In this pass we only run the compute
        // shader and update the SSBO, there's no rendering operations involved...
        light_cull_compute_shader->Bind();
        depth_prepass.BindBuffer(-1, 0);  // bind the depth buffer
        light_index_ssbo.Clear();  // recalculate light indices every frame
        light_index_ssbo.Bind(2);
        light_cull_compute_shader->Dispatch(nx, ny, 1);

        // wait until the compute shader finishes and then unbind the states. Note that this
        // sync step should ideally be placed closest to the code that actually uses the SSBO
        // in order to avoid unnecessary waits, but in this demo we need it right away...
        light_cull_compute_shader->SyncWait();
        light_cull_compute_shader->Unbind();
        depth_prepass.UnbindBuffer(0);  // unbind the depth buffer

        // pass 3: render objects as you normally do, but this time using the SSBOs to look up
        // visible lights rather than looping through every light source in the fragment shader
        Renderer::DepthTest(true);
        Renderer::DepthPrepass(false);
        Renderer::MSAA(true);  // unlike deferred rendering, built-in MSAA will work fine here
        Renderer::Clear();
        Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, suzanne.id, skybox.id);

        // do i need to clear ssbo and update every frame????
        // orbit light and spotlight and directional light are never culled,

        // optionally you can include another pass for postprocessing stuff (HDR, Bloom, Blur Effects, etc)
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        static bool sphere_gizmo = false;
        static bool plane_gizmo = false;
        static bool edit_colors = false;
        static bool edit_colorf = false;
        static int power_sphere = 5;
        static int power_plane = 8;

        static ImVec4 colors = ImVec4(0.635f, 0.0f, 1.0f, 0.0f);
        static ImVec4 colorf = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        static ImGuiColorEditFlags color_flags
            = ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_NoPicker;

        ui::LoadInspectorConfig();

        if (ImGui::Begin("Inspector##1", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Checkbox("Show Plane", &show_plane);
            ImGui::Separator();
            ImGui::Checkbox("Point Light Rotation", &orbit);
            ImGui::Separator();
            ImGui::Checkbox("Show Sphere Gizmo", &sphere_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Show Plane Gizmo", &plane_gizmo);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushItemWidth(100.0f);
            ImGui::SliderInt("Sphere Shininess", &power_sphere, 1, 10);
            ImGui::SliderInt("Plane Shininess", &power_plane, 1, 10);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::Checkbox("Edit Sphere Color", &edit_colors);
            if (edit_colors) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Sphere Color", (float*)&colors, color_flags);
                ImGui::Unindent(15.0f);
            }

            ImGui::Checkbox("Edit Flashlight Color", &edit_colorf);
            if (edit_colorf) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Flashlight Color", (float*)&colorf, color_flags);
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

        sphere_shininess = pow(2.0f, (float)power_sphere);
        plane_shininess = pow(2.0f, (float)power_plane);

        auto& m = sphere.GetComponent<Material>();
        m.SetUniform(1, glm::vec3(colors.x, colors.y, colors.z) * 0.2f);  // ambient
        m.SetUniform(2, glm::vec3(colors.x, colors.y, colors.z) * 0.7f);  // diffuse

        camera.GetComponent<Spotlight>().color = glm::vec3(colorf.x, colorf.y, colorf.z);

        //ImGui::ShowDemoWindow();
    }
}
