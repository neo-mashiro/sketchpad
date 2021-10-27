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

#include "examples/scene_01.h"

using namespace core;
using namespace buffer;
using namespace components;
using namespace utils;

namespace scene {

    static bool show_plane = true;
    static bool draw_depth_buffer = false;
    static bool orbit = true;

    static float orbit_time = 0.0f;
    static float orbit_speed = 2.0f;  // in radians per second
    static float orbit_radius = 4.5f;

    static glm::vec3 sphere_albedo { 0.22f, 0.0f, 1.0f };
    static float sphere_metalness = 0.05f;
    static float sphere_roughness = 0.05f;
    static float sphere_ao = 1.0f;

    static float plane_roughness = 0.1f;
    static float light_cluster_intensity = 10.0f;

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        // name your scene title (will appear in the top menu)
        this->title = "Example Scene";

        auto& model_path   = paths::models;
        auto& shader_path  = paths::shaders;
        auto& texture_path = paths::textures;
        auto& cubemap_path = paths::cubemaps;

        // load shader and texture assets upfront
        this->light_cull_compute_shader   = LoadAsset<ComputeShader>(shader_path + "light_cull.glsl");

        asset_ref<Shader> pbr_shader      = LoadAsset<Shader>(shader_path + "01_pbr.glsl");
        asset_ref<Shader> light_shader    = LoadAsset<Shader>(shader_path + "light_cube.glsl");
        //asset_ref<Shader> skybox_shader   = LoadAsset<Shader>(shader_path + "skybox.glsl");

        //asset_ref<Texture> space_cube     = LoadAsset<Texture>(GL_TEXTURE_CUBE_MAP, cubemap_path + "space");
        asset_ref<Texture> checkerboard   = LoadAsset<Texture>(texture_path + "misc\\checkboard.png");
        asset_ref<Texture> ball_albedo    = LoadAsset<Texture>(texture_path + "meshball4\\albedo.jpg");
        asset_ref<Texture> ball_normal    = LoadAsset<Texture>(texture_path + "meshball4\\normal.jpg");
        asset_ref<Texture> ball_metallic  = LoadAsset<Texture>(texture_path + "meshball4\\metallic.jpg");
        asset_ref<Texture> ball_roughness = LoadAsset<Texture>(texture_path + "meshball4\\roughness.jpg");
        asset_ref<Texture> ball_displace  = LoadAsset<Texture>(texture_path + "meshball4\\displacement.jpg");

        std::vector<asset_ref<Texture>> runestone_pillar {
            LoadAsset<Texture>(model_path + "runestone\\pillars_albedo.png"),
            LoadAsset<Texture>(model_path + "runestone\\pillars_normal.png"),
            LoadAsset<Texture>(model_path + "runestone\\pillars_metallic.png"),
            LoadAsset<Texture>(model_path + "runestone\\pillars_roughness.png")
        };

        std::vector<asset_ref<Texture>> runestone_platform {
            LoadAsset<Texture>(model_path + "runestone\\platform_albedo.png"),
            LoadAsset<Texture>(model_path + "runestone\\platform_normal.png"),
            LoadAsset<Texture>(model_path + "runestone\\platform_metallic.png"),
            LoadAsset<Texture>(model_path + "runestone\\platform_roughness.png"),
            nullptr,  // runestone platform does not have AO map, use a null placeholder
            LoadAsset<Texture>(model_path + "runestone\\platform_emissive.png")
        };

        // create uniform buffer objects (UBO) from shaders
        AddUBO(pbr_shader->GetID());

        // create frame buffer objects (FBO)
        FBO& depth_framebuffer = AddFBO(Window::width, Window::height);
        depth_framebuffer.AddDepStTexture();
        // depth_prepass.GetVirtualMaterial()->SetShader(...);  // set a postprocess shader

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 6.0f, 16.0f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(View::Perspective);
        camera.AddComponent<Spotlight>(glm::vec3(1.0f, 0.553f, 0.0f), 3.8f);  // attach a flashlight
        camera.GetComponent<Spotlight>().SetCutoff(4.0f);
        
        // skybox
        //skybox = CreateEntity("Skybox", ETag::Skybox);
        //skybox.AddComponent<Mesh>(Primitive::Cube);
        //skybox.GetComponent<Material>().SetShader(skybox_shader);
        //skybox.GetComponent<Material>().SetTexture(0, space_cube);

        // create renderable entities...
        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(Primitive::Sphere);
        sphere.GetComponent<Transform>().Translate(world::up * 10.5f);
        sphere.GetComponent<Transform>().Scale(2.0f);

        // a material is automatically attached to the entity when you add a mesh or model
        if (auto& mat = sphere.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            // it's possible to bind a uniform to a variable and observe changes in the shader
            // automatically. In this case, you only need to bind it once inside this `Init()`
            // function, which saves you from having to set uniforms every frame.
            mat.SetUniform(10, sphere_albedo, true);  // albedo (diffuse color)
            mat.SetUniform(11, sphere_metalness, true);
            mat.SetUniform(12, sphere_roughness, true);
            mat.SetUniform(13, sphere_ao, true);
        }

        // for entities whose mesh uses the same type of primitive, you can reuse the previous
        // entity's VAO buffer to create the mesh in order to save memory, in our case, sphere
        // and ball could share the same vertices data, so we'll reuse sphere's vertices array
        Mesh& sphere_mesh = sphere.GetComponent<Mesh>();
        ball = CreateEntity("Ball");
        ball.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        ball.GetComponent<Transform>().Translate(world::up * 6.0f);
        ball.GetComponent<Transform>().Scale(2.0f);

        if (auto& mat = ball.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            mat.SetTexture(0, ball_albedo);
            mat.SetTexture(1, ball_normal);
            mat.SetTexture(2, ball_metallic);
            mat.SetTexture(3, ball_roughness);
            mat.SetTexture(6, ball_displace);
            if (false) {
                mat.SetShader(nullptr);      // this is how you can reset a material's shader
                mat.SetTexture(0, nullptr);  // this is how you can reset a material's texture
            }
        }

        plane = CreateEntity("Plane");
        plane.AddComponent<Mesh>(Primitive::Plane);
        plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        plane.GetComponent<Transform>().Scale(3.0f);
        if (auto& mat = plane.GetComponent<Material>(); true) {
            mat.SetShader(pbr_shader);
            mat.SetUniform(12, plane_roughness, true);
            mat.SetTexture(0, checkerboard);
        }

        runestone = CreateEntity("Runestone");
        runestone.GetComponent<Transform>().Scale(0.02f);
        runestone.GetComponent<Transform>().Translate(world::up * -4.0f);

        auto& model = runestone.AddComponent<Model>(model_path + "runestone\\runestone.fbx", Quality::Auto);
        runestone.GetComponent<Material>().SetShader(pbr_shader);
        model.Import("pillars", runestone_pillar);     // material id 6 (may differ on your PC)
        model.Import("platform", runestone_platform);  // material id 5 (may differ on your PC)
        model.Report();  // a report that helps you learn how to load the model asset

        // create light sources, start from the ambient light (directional light)
        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(glm::radians(-45.0f), world::right);
        direct_light.AddComponent<DirectionLight>(color::white, 0.2f);

        // for static lights, we only need to set the uniform buffer once in `Init()`
        if (auto& ubo = UBOs[1]; true) {
            ubo.Bind();
            ubo.SetData(0, val_ptr(direct_light.GetComponent<DirectionLight>().color));
            ubo.SetData(1, val_ptr(direct_light.GetComponent<Transform>().forward * (-1.0f)));
            ubo.SetData(2, &(direct_light.GetComponent<DirectionLight>().intensity));
            ubo.Unbind();
        }

        // for dynamic lights, the UBO's data will be set in `OnSceneRender()` every frame
        orbit_light = CreateEntity("Orbit Light");
        orbit_light.GetComponent<Transform>().Translate(glm::vec3(0.0f, 8.0f, orbit_radius));
        orbit_light.GetComponent<Transform>().Scale(0.03f);
        orbit_light.AddComponent<PointLight>(color::lime, 0.8f);
        orbit_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);
        orbit_light.AddComponent<Mesh>(Primitive::Cube);
        orbit_light.GetComponent<Material>().SetShader(light_shader);

        // forward+ rendering setup (a.k.a tiled forward rendering)
        nx = (Window::width + tile_size - 1) / tile_size;
        ny = (Window::height + tile_size - 1) / tile_size;
        n_tiles = nx * ny;

        // setup the shader storage buffers for 28 static point lights
        const unsigned int n_pl = 28;
        light_cull_compute_shader->SetUniform(0, n_pl);

        pl_color_ssbo    = LoadBuffer<SSBO<glm::vec4>>(n_pl);
        pl_position_ssbo = LoadBuffer<SSBO<glm::vec4>>(n_pl);
        pl_range_ssbo    = LoadBuffer<SSBO<GLfloat>>(n_pl);
        pl_index_ssbo    = LoadBuffer<SSBO<GLint>>(n_pl * n_tiles);

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

            // translate light positions to the range that is symmetrical about the origin
            auto position = glm::vec3(row - 3.5f, 1.5f, col - 3.5f) * 9.0f;
            auto rand_color = glm::vec3(0.0f);

            // for each point light, generate a random color that is not too dark
            while (glm::length2(rand_color) < 1.5f) {
                rand_color = glm::vec3(math::RandomFloat01(), math::RandomFloat01(), math::RandomFloat01());
            }

            point_lights[index] = CreateEntity("Point Light " + std::to_string(index));
            auto& pl = point_lights[index];
            pl.GetComponent<Transform>().Translate(position - world::origin);
            pl.GetComponent<Transform>().Scale(0.2f);
            pl.AddComponent<PointLight>(rand_color, 1.5f);
            pl.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);
            pl.AddComponent<Mesh>(Primitive::Cube);
            pl.GetComponent<Material>().SetShader(light_shader);

            // the effective range of the light is calculated for you by `SetAttenuation()`
            colors.push_back(glm::vec4(rand_color, 1.0f));
            positions.push_back(glm::vec4(position, 1.0f));
            ranges.push_back(pl.GetComponent<PointLight>().range);

            index++;
        }

        pl_color_ssbo->Write(colors);
        pl_color_ssbo->Bind(0);

        pl_position_ssbo->Write(positions);
        pl_position_ssbo->Bind(1);

        pl_range_ssbo->Write(ranges);
        pl_range_ssbo->Bind(2);

        Renderer::FaceCulling(true);
    }

    // this is called every frame, update your scene here and submit the entities to the renderer
    void Scene01::OnSceneRender() {
        (void)glGetError();
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        // update the scene first: rotate the orbit light
        if (orbit) {
            auto& transform = orbit_light.GetComponent<Transform>();
            float x = orbit_radius * sin(orbit_time * orbit_speed);
            float z = orbit_radius * cos(orbit_time * orbit_speed);
            float y = transform.position.y;
            orbit_time += Clock::delta_time;
            transform.Translate(glm::vec3(x, y, z) - transform.position);
        }

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
            auto& pos = orbit_light.GetComponent<Transform>().position;
            ubo.Bind();
            ubo.SetData(0, val_ptr(ol.color));
            ubo.SetData(1, val_ptr(pos));
            ubo.SetData(2, &(ol.intensity));
            ubo.SetData(3, &(ol.linear));
            ubo.SetData(4, &(ol.quadratic));
            ubo.SetData(5, &(ol.range));
            ubo.Unbind();
        }

        // update uniform buffer for the cluster of 28 point lights
        if (auto& ubo = UBOs[4]; true) {
            auto& pl = point_lights[0].GetComponent<PointLight>();
            ubo.Bind();
            ubo.SetData(0, &(light_cluster_intensity));
            ubo.SetData(1, &(pl.linear));
            ubo.SetData(2, &(pl.quadratic));
            ubo.Unbind();
        }

        // a demo of how to do physically-based shading in the context of tiled forward renderers
        // pass 1: render depth values into the prepass depth buffer
        auto& depth_framebuffer = FBOs[0];
        if (depth_framebuffer.Bind(); true) {
            Renderer::DepthTest(true);
            Renderer::DepthPrepass(true);  // enable early z-test
            Renderer::Clear(color::black);
            Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
            Renderer::Submit(orbit_light.id);
            //Renderer::Submit(skybox.id);
            Renderer::Render();
            depth_framebuffer.Unbind();
        }

        // optionally check if depth values are correctly written to the framebuffer
        if (draw_depth_buffer) {
            Renderer::DepthTest(false);
            Renderer::Clear(color::blue);  // use a non-black clear color to debug the depth buffer
            depth_framebuffer.DebugDraw(-1);
            return;
        }

        // pass 2: dispatch light culling computations on the compute shader.
        // in this pass we only update SSBOs, there's no rendering operations involved...
        if (light_cull_compute_shader->Bind(); true) {
            depth_framebuffer.GetDepthTexture().Bind(0);  // bind the depth buffer
            pl_index_ssbo->Clear();  // recalculate light indices every frame
            pl_index_ssbo->Bind(3);
            light_cull_compute_shader->Dispatch(nx, ny, 1);
        }

        // now let's wait until the compute shader finishes and then unbind the states....
        // ideally, `SyncWait()` should be placed closest to the code that actually uses
        // the SSBO to avoid unnecessary waits, but in this demo we need it right away....

        light_cull_compute_shader->SyncWait();  // sync here to make sure all writes are visible
        light_cull_compute_shader->Unbind();    // unbind the compute shader
        depth_framebuffer.GetDepthTexture().Unbind(0);  // unbind the depth buffer

        // pass 3: render objects as you normally do, but this time using the SSBOs to look up
        // visible lights rather than looping through every light source in the fragment shader

        // first, we need to update uniforms for every entity's material, this is required even
        // if a uniform value stays put, keep in mind that the PBR shader is shared by multiple
        // entities so they can be overwritten by other materials, but setting uniform is cheap

        if (auto& mat = sphere.GetComponent<Material>(); true) {
            for (int i = 3; i <= 9; i++) {
                mat.SetUniform(i, 0);  // sphere doesn't have any PBR textures
            }
        }

        if (auto& mat = ball.GetComponent<Material>(); true) {
            for (int i = 3; i <= 6; i++) {
                mat.SetUniform(i, 1);
            }
            mat.SetUniform(7, 0);      // ball does not have AO map
            mat.SetUniform(8, 0);      // ball does not have emission map
            mat.SetUniform(9, 1);      // ball does have displacement map
            mat.SetUniform(13, 1.0f);  // ambient occlussion
            mat.SetUniform(14, glm::vec2(3.2f, 1.8f));  // uv scale
        }

        if (auto& mat = plane.GetComponent<Material>(); true) {
            mat.SetUniform(3, 1);      // plane only has an albedo_map
            for (int i = 4; i <= 9; i++) {
                mat.SetUniform(i, 0);
            }
            mat.SetUniform(11, 0.1f);  // metalness
            mat.SetUniform(13, 1.0f);  // ambient occlussion
            mat.SetUniform(14, glm::vec2(8.0f));  // uv scale
        }

        if (auto& mat = runestone.GetComponent<Material>(); true) {
            for (int i = 3; i <= 6; i++) {
                mat.SetUniform(i, 1);
            }
            mat.SetUniform(7, 0);      // runestone does not have AO map
            mat.SetUniform(8, 1);      // runestone does have emission map
            mat.SetUniform(9, 0);      // runestone does not have displacement map
            mat.SetUniform(13, 1.0f);  // specify ambient occlussion explicitly
            mat.SetUniform(14, glm::vec2(1.0f));  // uv scale
        }

        if (auto& mat = orbit_light.GetComponent<Material>(); true) {
            mat.SetUniform(3, orbit_light.GetComponent<PointLight>().color);
        }

        for (int i = 0; i < 28; i++) {
            auto& mat = point_lights[i].GetComponent<Material>();
            auto& color = point_lights[i].GetComponent<PointLight>().color;
            mat.SetUniform(3, color);
        }

        Renderer::DepthTest(true);
        Renderer::DepthPrepass(false);
        Renderer::MSAA(true);  // MSAA works fine in forward+ renderers (not true for deferred renderers)
        Renderer::Clear(color::blue);

        // note: when you submit a list of entities to the renderer, they are internally stored in a queue
        // and will be drawn in the order of submission, this order can not only affect alpha blending but
        // can also make a huge difference in performance.

        // tips: it is advised to submit the skybox last to the renderer because it's the farthest object
        // in the scene, this can save us quite a lot fragment invocations as the pixels that already fail
        // the depth test will be instantly discarded.

        // tips: it is advised to submit similar entities as close as possible to each other, especially
        // when you have a large number of entities. To be specific, entities that share the same shader
        // or use the same textures should be grouped together as much as possible, this can help reduce
        // the number of context switching, which is expensive. The shader, texture and uniform class have
        // been optimized to support smart bindings and smart uploads, you should make the most of these
        // features to avoid unnecessary binding operations of shaders and textures, and uniform uploads.

        // tips: if a list of entities use the same shader, textures as well as meshes, you should enable
        // batch rendering on the meshes, submit only one of them and handle batch rendering in the shader

        Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
        Renderer::Submit(orbit_light.id);
        //Renderer::Submit(skybox.id);  // it is advised to submit the skybox last to save performance

        for (int i = 0; i < 28; i++) {
            Renderer::Submit(point_lights[i].id);
        }

        Renderer::Render();

        CheckGLError(5);

        // optionally you can include another pass for postprocessing stuff (HDR, Bloom, Blur Effects, etc)
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        static bool show_sphere_gizmo     = false;
        static bool show_plane_gizmo      = false;
        static bool edit_sphere_albedo    = false;
        static bool edit_flashlight_color = false;

        static ImVec4 sphere_color = ImVec4(0.22f, 0.0f, 1.0f, 0.0f);
        static ImVec4 flashlight_color = ImVec4(1.0f, 0.553f, 0.0f, 1.0f);

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
            ImGui::Checkbox("Show Sphere Gizmo", &show_sphere_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Show Plane Gizmo", &show_plane_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Visualize Depth Buffer", &draw_depth_buffer);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushItemWidth(100.0f);
            ImGui::SliderFloat("Sphere Metalness", &sphere_metalness, 0.05f, 1.0f);
            ImGui::SliderFloat("Sphere Roughness", &sphere_roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Sphere AO", &sphere_ao, 0.0f, 1.0f);
            ImGui::SliderFloat("Plane Roughness", &plane_roughness, 0.1f, 0.3f);
            ImGui::SliderFloat("Light Cluster Intensity", &light_cluster_intensity, 1.0f, 20.0f);
            ImGui::PopItemWidth();
            ImGui::Separator();

            ImGui::Checkbox("Edit Sphere Albedo", &edit_sphere_albedo);
            if (edit_sphere_albedo) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Sphere Albedo", (float*)&sphere_color, color_flags);
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

        if (show_sphere_gizmo) {
            ui::DrawGizmo(camera, sphere);
        }

        if (show_plane_gizmo && show_plane) {
            ui::DrawGizmo(camera, plane);
        }

        sphere_albedo = glm::vec3(sphere_color.x, sphere_color.y, sphere_color.z);

        if (auto& sl = camera.GetComponent<Spotlight>(); true) {
            sl.color = glm::vec3(flashlight_color.x, flashlight_color.y, flashlight_color.z);
        }

        //ImGui::ShowDemoWindow();
    }
}
