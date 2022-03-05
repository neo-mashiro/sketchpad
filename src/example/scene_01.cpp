#include "pch.h"

#include "core/base.h"
#include "core/clock.h"
#include "core/debug.h"
#include "core/input.h"
#include "core/window.h"
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

    static bool show_plane = true;
    static bool show_light_cluster = true;
    static bool draw_depth_buffer = false;

    static bool orbit = true;
    static float orbit_speed = 0.5f;  // 0.5 degrees per frame

    static vec3 sphere_albedo { 0.22f, 0.0f, 1.0f };
    static float sphere_metalness = 0.05f;
    static float sphere_roughness = 0.05f;
    static float sphere_ao = 1.0f;

    static float plane_roughness = 0.1f;
    static float light_cluster_intensity = 10.0f;
    static float skybox_exposure = 3.5f;

    static int tone_mapping_mode = 3;
    static int bloom_effect = 3;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        // name your scene title (will appear in the top menu)
        this->title = "Demo Scene";

        //// load the string paths to various assets
        //auto& model_path   = utils::paths::model;
        //auto& shader_path  = utils::paths::shader;
        //auto& texture_path = utils::paths::texture;

        //// shaders that apply to entities are managed by material components, they are considered
        //// to be assets like textures, which can be shared across entities, you only need to load
        //// them at local scope and attach to the materials.
        //asset_ref<Shader> pbr_shader    = MakeAsset<Shader>(shader_path + "scene_01\\pbr.glsl");
        //asset_ref<Shader> light_shader  = MakeAsset<Shader>(shader_path + "light_source.glsl");
        //asset_ref<Shader> skybox_shader = MakeAsset<Shader>(shader_path + "scene_01\\skybox.glsl");

        //asset_ref<Material> pbr_material    = MakeAsset<Material>(pbr_shader);
        //asset_ref<Material> light_material  = MakeAsset<Material>(light_shader);
        //asset_ref<Material> skybox_material = MakeAsset<Material>(skybox_shader);

        //// compute shaders and shaders that act upon framebuffers are deemed utility shaders
        //// which are not part of the entity-component system, that's why they are declared
        //// in the header file as class members, users are supposed to control them directly
        //blur_shader       = std::make_unique<Shader>(shader_path + "scene_01\\blur.glsl");
        //blend_shader      = std::make_unique<Shader>(shader_path + "scene_01\\blend.glsl");
        //light_cull_shader = std::make_unique<CShader>(shader_path + "light_culling.glsl");
        //light_cull_shader->SetUniform(0, n_point_lights);

        //// load texture assets upfront
        //asset_ref<Texture> checkerboard   = MakeAsset<Texture>(texture_path + "common\\checkboard.png");
        //asset_ref<Texture> ball_albedo    = MakeAsset<Texture>(texture_path + "meshball\\albedo.jpg");
        //asset_ref<Texture> ball_normal    = MakeAsset<Texture>(texture_path + "meshball\\normal.jpg");
        //asset_ref<Texture> ball_metallic  = MakeAsset<Texture>(texture_path + "meshball\\metallic.jpg");
        //asset_ref<Texture> ball_roughness = MakeAsset<Texture>(texture_path + "meshball\\roughness.jpg");

        //// load skybox cubemap from hdr image
        //asset_ref<Texture> space_cubemap = MakeAsset<Texture>(texture_path + "cosmic\\", ".hdr", 2048, 1);

        //// textures for external models that require manual import are sorted in a vector
        //std::vector<asset_ref<Texture>> runestone_pillar {
        //    MakeAsset<Texture>(model_path + "runestone\\pillars_albedo.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\pillars_normal.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\pillars_metallic.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\pillars_roughness.png")
        //};

        //std::vector<asset_ref<Texture>> runestone_platform {
        //    MakeAsset<Texture>(model_path + "runestone\\platform_albedo.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\platform_normal.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\platform_metallic.png"),
        //    MakeAsset<Texture>(model_path + "runestone\\platform_roughness.png"),
        //    nullptr,  // runestone platform does not have AO map, use a null placeholder
        //    MakeAsset<Texture>(model_path + "runestone\\platform_emissive.png")
        //};

        //// check errors periodically in case the built-in debug message callback fails
        //Debug::CheckGLError(0);  // checkpoint 0

        //// create uniform buffer objects (UBO) from shaders (duplicates will be skipped)
        //AddUBO(pbr_shader->ID());
        //AddUBO(light_shader->ID());

        //// create samplers (sampling state objects) for Gaussian blur filter and bloom
        //point_sampler    = std::make_unique<Sampler>(FilterMode::Point);
        //bilinear_sampler = std::make_unique<Sampler>(FilterMode::Bilinear);

        //Debug::CheckGLError(1);

        //// create intermediate framebuffer objects (FBO)
        //FBO& depth_prepass = AddFBO(Window::width, Window::height);
        //depth_prepass.AddDepStTexture();

        //FBO& bloom_filter_pass = AddFBO(Window::width, Window::height);
        //bloom_filter_pass.AddColorTexture(2, true);    // multisampled textures for MSAA
        //bloom_filter_pass.AddDepStRenderBuffer(true);  // multisampled RBO for MSAA

        //FBO& msaa_resolve_pass = AddFBO(Window::width, Window::height);
        //msaa_resolve_pass.AddColorTexture(2);

        //FBO& blur_pass = AddFBO(Window::width / 2, Window::height / 2);  // two-pass Gaussian blur
        //blur_pass.AddColorTexture(2);

        //Debug::CheckGLError(2);

        //// main camera
        //camera = CreateEntity("Camera", ETag::MainCamera);
        //camera.GetComponent<Transform>().Translate(vec3(0.0f, 6.0f, -16.0f));
        //camera.GetComponent<Transform>().Rotate(world::up, 180.0f);
        //camera.AddComponent<Camera>(View::Perspective);
        //camera.AddComponent<Spotlight>(vec3(1.0f, 0.553f, 0.0f), 3.8f);  // attach a flashlight
        //camera.GetComponent<Spotlight>().SetCutoff(4.0f);
        //
        //// skybox
        //skybox = CreateEntity("Skybox", ETag::Skybox);
        //skybox.AddComponent<Mesh>(Primitive::Cube);
        //if (auto& mat = skybox.AddComponent<Material>(skybox_material); true) {
        //    mat.SetTexture(0, space_cubemap);
        //    mat.SetUniform(3, skybox_exposure, true);
        //}

        //// create renderable entities...
        //sphere = CreateEntity("Sphere");
        //sphere.AddComponent<Mesh>(Primitive::Sphere);
        //sphere.GetComponent<Transform>().Translate(world::up * 10.5f);
        //sphere.GetComponent<Transform>().Scale(2.0f);

        //// it's possible to bind a uniform to a variable and observe changes in the shader
        //// automatically. In this case, you only need to bind it once inside this `Init()`
        //// function, which saves you from having to set uniforms every frame.
        //if (auto& mat = sphere.AddComponent<Material>(pbr_material); true) {
        //    mat.SetUniform(10, sphere_albedo, true);  // albedo (diffuse color)
        //    mat.SetUniform(11, sphere_metalness, true);
        //    mat.SetUniform(12, sphere_roughness, true);
        //    mat.SetUniform(13, sphere_ao, true);
        //}

        //// for entities whose mesh uses the same type of primitive, you can reuse the previous
        //// entity's VAO buffer to create the mesh in order to save memory, in our case, sphere
        //// and ball could share the same vertices data, so we'll reuse sphere's vertices array
        //Mesh& sphere_mesh = sphere.GetComponent<Mesh>();
        //ball = CreateEntity("Ball");
        //ball.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        //ball.GetComponent<Transform>().Translate(world::up * 6.0f);
        //ball.GetComponent<Transform>().Scale(2.0f);

        //if (auto& mat = ball.AddComponent<Material>(pbr_material); true) {
        //    mat.SetTexture(0, ball_albedo);
        //    mat.SetTexture(1, ball_normal);
        //    mat.SetTexture(2, ball_metallic);
        //    mat.SetTexture(3, ball_roughness);
        //    if (false) {
        //        mat.SetShader(nullptr);      // this is how you can reset a material's shader
        //        mat.SetTexture(0, nullptr);  // this is how you can reset a material's texture
        //    }
        //}

        //plane = CreateEntity("Plane");
        //plane.AddComponent<Mesh>(Primitive::Plane);
        //plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        //plane.GetComponent<Transform>().Scale(3.0f);

        //if (auto& mat = plane.AddComponent<Material>(pbr_material); true) {
        //    mat.SetUniform(12, plane_roughness, true);
        //    mat.SetTexture(0, checkerboard);
        //}

        //runestone = CreateEntity("Runestone");
        //runestone.GetComponent<Transform>().Scale(0.02f);
        //runestone.GetComponent<Transform>().Translate(world::up * -4.0f);
        //runestone.AddComponent<Material>(pbr_material);

        //auto& model = runestone.AddComponent<Model>(model_path + "runestone\\runestone.fbx", Quality::Auto);
        //model.Import("pillars", runestone_pillar);
        //model.Import("platform", runestone_platform);

        //Debug::CheckGLError(3);

        //// create light sources, start from the ambient light (directional light)
        //direct_light = CreateEntity("Directional Light");
        //direct_light.GetComponent<Transform>().Rotate(world::right, -45.0f);
        //direct_light.AddComponent<DirectionLight>(color::white, 0.2f);

        //// for static lights, we only need to set the uniform buffer once in `Init()`
        //if (auto& ubo = UBOs[1]; true) {
        //    ubo.Bind();
        //    ubo.SetData(0, val_ptr(direct_light.GetComponent<DirectionLight>().color));
        //    ubo.SetData(1, val_ptr(direct_light.GetComponent<Transform>().forward * (-1.0f)));
        //    ubo.SetData(2, &(direct_light.GetComponent<DirectionLight>().intensity));
        //    ubo.Unbind();
        //}

        //// for dynamic lights, the UBO's data will be set in `OnSceneRender()` every frame
        //orbit_light = CreateEntity("Orbit Light");
        //orbit_light.GetComponent<Transform>().Translate(0.0f, 8.0f, 4.5f);
        //orbit_light.GetComponent<Transform>().Scale(0.3f);
        //orbit_light.AddComponent<PointLight>(color::lime, 0.8f);
        //orbit_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);
        //orbit_light.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        //orbit_light.AddComponent<Material>(light_shader);

        //Debug::CheckGLError(4);

        //// forward+ rendering setup (a.k.a tiled forward rendering)
        //nx = (Window::width + tile_size - 1) / tile_size;
        //ny = (Window::height + tile_size - 1) / tile_size;
        //n_tiles = nx * ny;

        //// setup shader storage buffers for the 28 static point lights
        //pl_color_ssbo    = MakeBuffer<SSBO<vec4>>(n_point_lights);
        //pl_position_ssbo = MakeBuffer<SSBO<vec4>>(n_point_lights);
        //pl_range_ssbo    = MakeBuffer<SSBO<GLfloat>>(n_point_lights);
        //pl_index_ssbo    = MakeBuffer<SSBO<GLint>>(n_point_lights * n_tiles);

        //// light culling in forward+ rendering can be applied to both static and dynamic lights,
        //// in the latter case, it is required that users update the input SSBO buffer data every
        //// frame. In most cases, perform light culling on static lights alone is already enough,
        //// unless you have thousands of lights whose colors or positions are constantly changing.
        //// in this demo, we will only cull the 28 (or even 2800 if you have space) static point
        //// lights, so the input SSBO buffer data only needs to be set up once. The spotlight and
        //// orbit light will always participate in lighting calculations every frame.

        //std::vector<vec4> colors {};
        //std::vector<vec4> positions {};
        //std::vector<GLfloat> ranges {};

        //// make a grid of size 8 x 8 (64 cells), sample each border cell to be a point light.
        //// thus we would have a total of 28 point lights that surrounds our scene, which are
        //// evenly distributed on the boundaries of the plane.

        //for (int i = 0, index = 0; i < 64; i++) {
        //    int row = i / 8;  // ~ [0, 7]
        //    int col = i % 8;  // ~ [0, 7]

        //    if (bool on_boundary = (row == 0 || row == 7 || col == 0 || col == 7); !on_boundary) {
        //        continue;  // skip cells in the middle
        //    }

        //    // translate light positions to the range that is symmetrical about the origin
        //    auto position = vec3(row - 3.5f, 1.5f, col - 3.5f) * 9.0f;

        //    // for each point light, generate a random highly saturated, bright color
        //    // [wikipedia]: fully saturated colors are placed around a circle at a lightness value of 0.5
        //    float random = math::RandomFloat01();
        //    auto hsl_color = vec3(random, 0.7f + random * 0.3f, 0.4f + random * 0.2f);
        //    auto rand_color = math::HSL2RGB(hsl_color);

        //    point_lights[index] = CreateEntity("Point Light " + std::to_string(index));
        //    auto& pl = point_lights[index];
        //    pl.GetComponent<Transform>().Translate(position - world::origin);
        //    pl.GetComponent<Transform>().Scale(0.8f);
        //    pl.AddComponent<PointLight>(rand_color, 1.5f);
        //    pl.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);
        //    pl.AddComponent<Mesh>(sphere_mesh.GetVAO(), sphere_mesh.n_verts);
        //    pl.AddComponent<Material>(light_material);

        //    // the effective range of the light is calculated for you by `SetAttenuation()`
        //    colors.push_back(vec4(rand_color, 1.0f));
        //    positions.push_back(vec4(position, 1.0f));
        //    ranges.push_back(pl.GetComponent<PointLight>().range);

        //    index++;
        //}

        //pl_color_ssbo->Write(colors);
        //pl_color_ssbo->Bind(0);

        //pl_position_ssbo->Write(positions);
        //pl_position_ssbo->Bind(1);

        //pl_range_ssbo->Write(ranges);
        //pl_range_ssbo->Bind(2);

        //Debug::CheckGLError(5);

        //Renderer::FaceCulling(true);
        //Renderer::SeamlessCubemap(true);
    }

    // this is called every frame, update your scene here and submit the entities to the renderer
    void Scene01::OnSceneRender() {
        //UpdateEntities();
        //UpdateUBOs();

        //// pass 1: render depth values into the prepass depth buffer
        //DepthPrepass();

        //// optionally check if depth values are correctly written to the framebuffer
        //if (draw_depth_buffer) {
        //    Renderer::DepthTest(false);
        //    Renderer::Clear();  // a deep blue clear color (easier to debug depth)
        //    auto& depth_prepass_framebuffer = FBOs[0];
        //    depth_prepass_framebuffer.Draw(-1);
        //    return;
        //}

        //// pass 2: dispatch light culling computations on the compute shader
        //LightCullPass();

        //// pass 3: render objects to the multisampled bloom filter buffer (multiple render targets)
        //UpdateUniforms();  // update uniforms before actual shading
        //BloomFilterPass();

        //// pass 4: resolve the previously written multisampled textures to normal textures
        //MSAAResolvePass();

        //// pass 5: apply two-pass Gaussian blur filter on the bloom highlights texture
        //GaussianBlurPass();

        //// pass 6: blend the bloom texture (blurred mipmap) with the original scene texture
        //BloomBlendPass();
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        //using namespace ImGui;

        //static bool show_sphere_gizmo     = false;
        //static bool show_plane_gizmo      = false;
        //static bool edit_sphere_albedo    = false;
        //static bool edit_flashlight_color = false;

        //static ImVec4 sphere_color = ImVec4(0.22f, 0.0f, 1.0f, 0.0f);
        //static ImVec4 flashlight_color = ImVec4(1.0f, 0.553f, 0.0f, 1.0f);
        //static const ImVec4 text_color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);

        //static ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoSidePreview
        //    | ImGuiColorEditFlags_PickerHueWheel
        //    | ImGuiColorEditFlags_DisplayRGB
        //    | ImGuiColorEditFlags_NoPicker;

        //static const char* tone_mappings[] = {
        //    "Simple Reinhard",
        //    "Reinhard-Jodie (Shadertoy)",
        //    "Uncharted 2 Hable Filmic",
        //    "Approximated ACES (Unreal Engine 4)"
        //};

        //static const char tone_mapping_guide[] = "Here is a number of algorithms for high dynamic "
        //    "range tone mapping, select one that looks best on your scene.";

        //static const char tone_mapping_note[] = "Approximated ACES filmic tonemapper might not be "
        //    "the most visually attractive one, but it is the most physically correct.";

        //if (ui::NewInspector() && BeginTabBar("InspectorTab", ImGuiTabBarFlags_None)) {
        //    if (BeginTabItem("Scene")) {
        //        Indent(5.0f);
        //        Checkbox("Show Plane", &show_plane);                    Separator();
        //        Checkbox("Show Light Cluster", &show_light_cluster);    Separator();
        //        Checkbox("Orbit Light", &orbit);                        Separator();
        //        Checkbox("Show Sphere Gizmo", &show_sphere_gizmo);      Separator();
        //        Checkbox("Show Plane Gizmo", &show_plane_gizmo);        Separator();
        //        Checkbox("Visualize Depth Buffer", &draw_depth_buffer); Separator();

        //        PushItemWidth(130.0f);
        //        SliderFloat("Light Cluster Intensity", &light_cluster_intensity, 3.0f, 20.0f); Separator();
        //        SliderFloat("Skybox Exposure", &skybox_exposure, 1.2f, 8.0f); Separator();
        //        PopItemWidth();
        //        Spacing();

        //        if (Button("  New Colors  ")) {
        //            UpdatePLColors();
        //        }
        //        SameLine(0.0f, 10.0f);

        //        Text("Reset Light Cluster Colors");
        //        if (IsItemHovered()) {
        //            SetTooltip("New colors will be generated at random.");
        //        }

        //        Spacing();
        //        Separator();
        //        Unindent(5.0f);
        //        EndTabItem();
        //    }

        //    if (BeginTabItem("Material")) {
        //        Indent(5.0f);
        //        PushItemWidth(100.0f);
        //        SliderFloat("Sphere Metalness", &sphere_metalness, 0.05f, 1.0f);
        //        SliderFloat("Sphere Roughness", &sphere_roughness, 0.05f, 1.0f);
        //        SliderFloat("Sphere AO", &sphere_ao, 0.0f, 1.0f);
        //        SliderFloat("Plane Roughness", &plane_roughness, 0.1f, 0.3f);
        //        PopItemWidth();
        //        Separator();

        //        Checkbox("Edit Sphere Albedo", &edit_sphere_albedo);
        //        if (edit_sphere_albedo) {
        //            Spacing();
        //            Indent(10.0f);
        //            ColorPicker3("##Sphere Albedo", (float*)&sphere_color, color_flags);
        //            Unindent(10.0f);
        //        }

        //        Checkbox("Edit Flashlight Color", &edit_flashlight_color);
        //        if (edit_flashlight_color) {
        //            Spacing();
        //            Indent(10.0f);
        //            ColorPicker3("##Flashlight Color", (float*)&flashlight_color, color_flags);
        //            Unindent(10.0f);
        //        }

        //        Unindent(5.0f);
        //        EndTabItem();
        //    }

        //    if (BeginTabItem("HDR/Bloom")) {
        //        Indent(5.0f);
        //        PushItemWidth(180.0f);
        //        Text("Bloom Effect (Light Cluster)");
        //        SliderInt(" ##Bloom", &bloom_effect, 3, 5);
        //        PopItemWidth();
        //        Separator();
        //        PushStyleColor(ImGuiCol_Text, text_color);
        //        PushTextWrapPos(280.0f);
        //        TextWrapped(tone_mapping_guide);
        //        TextWrapped(tone_mapping_note);
        //        PopTextWrapPos();
        //        PopStyleColor();
        //        Separator();
        //        PushItemWidth(295.0f);
        //        Combo(" ", &tone_mapping_mode, tone_mappings, 4);
        //        PopItemWidth();
        //        Unindent(5.0f);
        //        EndTabItem();
        //    }

        //    EndTabBar();
        //    ui::EndInspector();
        //}

        //if (show_sphere_gizmo) {
        //    ui::DrawGizmo(camera, sphere, ui::Gizmo::Translate);
        //}

        //if (show_plane_gizmo && show_plane) {
        //    ui::DrawGizmo(camera, plane, ui::Gizmo::Translate);
        //}

        //sphere_albedo = vec3(sphere_color.x, sphere_color.y, sphere_color.z);

        //if (auto& sl = camera.GetComponent<Spotlight>(); true) {
        //    sl.color = vec3(flashlight_color.x, flashlight_color.y, flashlight_color.z);
        //}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene01::UpdatePLColors() {
        //std::vector<vec4> colors;

        //for (int i = 0; i < 28; i++) {
        //    auto hue = math::RandomFloat01();
        //    auto rand_color = math::HSL2RGB(hue, 0.7f + hue * 0.3f, 0.4f + hue * 0.2f);
        //    colors.push_back(vec4(rand_color, 1.0f));

        //    auto& pl = point_lights[i].GetComponent<PointLight>();
        //    pl.color = rand_color;  // update the point light color
        //}

        //pl_color_ssbo->Write(colors);  // also update the colors SSBO
        //pl_color_ssbo->Bind(0);
    }

    void Scene01::UpdateEntities() {
        //auto& main_camera = camera.GetComponent<Camera>();
        //main_camera.Update();

        //// rotate the orbit light
        //if (orbit) {
        //    auto& transform = orbit_light.GetComponent<Transform>();
        //    transform.Rotate(world::up, orbit_speed, false);
        //}
    }

    void Scene01::UpdateUBOs() {
        //// update camera's uniform buffer
        //if (auto& ubo = UBOs[0]; true) {
        //    auto& main_camera = camera.GetComponent<Camera>();
        //    ubo.Bind();
        //    ubo.SetData(0, val_ptr(main_camera.T->position));
        //    ubo.SetData(1, val_ptr(main_camera.T->forward));
        //    ubo.SetData(2, val_ptr(main_camera.GetViewMatrix()));
        //    ubo.SetData(3, val_ptr(main_camera.GetProjectionMatrix()));
        //    ubo.Unbind();
        //}

        //// update spotlight's uniform buffer
        //if (auto& ubo = UBOs[2]; true) {
        //    auto& spotlight = camera.GetComponent<Spotlight>();
        //    auto& transform = camera.GetComponent<Transform>();
        //    float inner_cos = spotlight.GetInnerCosine();
        //    float outer_cos = spotlight.GetOuterCosine();
        //    ubo.Bind();
        //    ubo.SetData(0, val_ptr(spotlight.color));
        //    ubo.SetData(1, val_ptr(transform.position));
        //    ubo.SetData(2, val_ptr(-transform.forward));  // L must be from the fragment pointing to the light source
        //    ubo.SetData(3, val_ptr(spotlight.intensity));
        //    ubo.SetData(4, val_ptr(inner_cos));
        //    ubo.SetData(5, val_ptr(outer_cos));
        //    ubo.SetData(6, val_ptr(spotlight.range));
        //    ubo.Unbind();
        //}

        //// update orbit light's uniform buffer
        //if (auto& ubo = UBOs[3]; true) {
        //    auto& ol = orbit_light.GetComponent<PointLight>();
        //    auto& pos = orbit_light.GetComponent<Transform>().position;
        //    ubo.Bind();
        //    ubo.SetData(0, val_ptr(ol.color));
        //    ubo.SetData(1, val_ptr(pos));
        //    ubo.SetData(2, val_ptr(ol.intensity));
        //    ubo.SetData(3, val_ptr(ol.linear));
        //    ubo.SetData(4, val_ptr(ol.quadratic));
        //    ubo.SetData(5, val_ptr(ol.range));
        //    ubo.Unbind();
        //}

        //// update uniform buffer for the cluster of 28 point lights
        //if (auto& ubo = UBOs[4]; true) {
        //    auto& pl = point_lights[0].GetComponent<PointLight>();
        //    ubo.Bind();
        //    ubo.SetData(0, val_ptr(light_cluster_intensity));
        //    ubo.SetData(1, val_ptr(pl.linear));
        //    ubo.SetData(2, val_ptr(pl.quadratic));
        //    ubo.Unbind();
        //}
    }

    void Scene01::UpdateUniforms() {
        //if (auto& mat = sphere.GetComponent<Material>(); true) {
        //    for (int i = 3; i <= 9; i++) {
        //        mat.SetUniform(i, 0);  // sphere doesn't have any PBR textures
        //    }
        //}

        //if (auto& mat = ball.GetComponent<Material>(); true) {
        //    for (int i = 3; i <= 6; i++) {
        //        mat.SetUniform(i, 1);
        //    }
        //    mat.SetUniform(7, 0);
        //    mat.SetUniform(8, 0);
        //    mat.SetUniform(9, 0);
        //    mat.SetUniform(13, 1.0f);  // ambient occlussion
        //    mat.SetUniform(14, vec2(1.6f, 0.9f));  // uv scale
        //}

        //if (auto& mat = plane.GetComponent<Material>(); true) {
        //    mat.SetUniform(3, 1);  // plane only has an albedo map
        //    for (int i = 4; i <= 9; i++) {
        //        mat.SetUniform(i, 0);
        //    }
        //    mat.SetUniform(11, 0.1f);  // metalness
        //    mat.SetUniform(13, 1.0f);  // ambient occlussion
        //    mat.SetUniform(14, vec2(8.0f));  // uv scale
        //}

        //if (auto& mat = runestone.GetComponent<Material>(); true) {
        //    for (int i = 3; i <= 6; i++) {
        //        mat.SetUniform(i, 1);
        //    }
        //    mat.SetUniform(7, 0);      // runestone does not have AO map
        //    mat.SetUniform(8, 1);      // runestone does have an emission map
        //    mat.SetUniform(9, 0);      // runestone does not use displacement map
        //    mat.SetUniform(13, 1.0f);  // specify ambient occlussion explicitly
        //    mat.SetUniform(14, vec2(1.0f));  // uv scale
        //}

        //if (auto& mat = orbit_light.GetComponent<Material>(); true) {
        //    auto& pl = orbit_light.GetComponent<PointLight>();
        //    mat.SetUniform(3, pl.color);
        //    mat.SetUniform(4, pl.intensity);
        //    mat.SetUniform(5, 2.0f);  // bloom multiplier (color saturation)
        //}

        //for (int i = 0; i < 28; i++) {
        //    auto& mat = point_lights[i].GetComponent<Material>();
        //    auto& pl = point_lights[i].GetComponent<PointLight>();
        //    mat.SetUniform(3, pl.color);
        //    mat.SetUniform(4, pl.intensity);
        //    mat.SetUniform(5, 7.0f);  // bloom multiplier (color saturation)
        //}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene01::DepthPrepass() {
        //auto& depth_prepass_framebuffer = FBOs[0];
        //depth_prepass_framebuffer.Bind();
        //depth_prepass_framebuffer.Clear(-1);

        //// the written depth values are only used for light culling in the next pass,
        //// we can skip the 28 light sources here because shading of the static light
        //// cube itself is not affected by any other incoming lights.

        //Renderer::DepthTest(true);
        //Renderer::DepthPrepass(true);  // enable early z-test
        //Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
        //Renderer::Submit(orbit_light.id);
        //Renderer::Submit(skybox.id);
        //Renderer::Render();
        //Renderer::DepthPrepass(false);  // disable early z-test

        //depth_prepass_framebuffer.Unbind();
    }

    void Scene01::LightCullPass() {
        //// in this pass we only update SSBOs, there's no rendering operations involved
        //auto& depth_prepass_framebuffer = FBOs[0];
        //auto& depth_texture = depth_prepass_framebuffer.GetDepthTexture();
        //depth_texture.Bind(0);  // bind the depth buffer

        //pl_index_ssbo->Clear();  // recalculate light indices every frame
        //pl_index_ssbo->Bind(3);
        //light_cull_shader->Bind();
        //light_cull_shader->Dispatch(nx, ny, 1);
        //light_cull_shader->SyncWait();  // sync here to make sure all writes are visible
        //light_cull_shader->Unbind();  // unbind the compute shader

        //// ideally, `SyncWait()` should be placed closest to the code that actually uses
        //// the SSBO to avoid unnecessary waits, but in this demo we need it right away....

        //depth_texture.Unbind(0);  // unbind the depth buffer
    }

    void Scene01::BloomFilterPass() {
        // this is the actual shading pass after the light culling pass, rather than looping through
        // every light in the fragment shader, we look up visible lights in the computed index SSBO.
        // here in this pass, we still have the geometry information of entities in the scene, so we
        // must make sure that MSAA is enabled before actual rendering. After this pass, we will no
        // longer be able to apply MSAA, because we would only have framebuffer textures.

        //FBO& bloom_filter_framebuffer = FBOs[1];
        //bloom_filter_framebuffer.ClearAll();
        //bloom_filter_framebuffer.Bind();

        //Renderer::MSAA(true);  // make sure to enable MSAA before actual rendering
        //Renderer::DepthTest(true);
        //Renderer::Submit(sphere.id, ball.id, show_plane ? plane.id : entt::null, runestone.id);
        //Renderer::Submit(orbit_light.id);
        //Renderer::Submit(skybox.id);

        //if (show_light_cluster) {
        //    for (int i = 0; i < n_point_lights; i++) {
        //        Renderer::Submit(point_lights[i].id);
        //    }
        //}

        //Renderer::Render();

        //bloom_filter_framebuffer.Unbind();
    }

    void Scene01::MSAAResolvePass() {
        //// resolve the multisampled framebuffer `FBOs[1]` to normal framebuffer `FBOs[2]`
        //FBO& source = FBOs[1];
        //FBO& target = FBOs[2];
        //target.ClearAll();

        //FBO::TransferColor(source, 0, target, 0);
        //FBO::TransferColor(source, 1, target, 1);
    }

    void Scene01::GaussianBlurPass() {
        //FBO& blur_framebuffer = FBOs[3];
        //blur_framebuffer.ClearAll();

        //auto& ping_texture = blur_framebuffer.GetColorTexture(0);
        //auto& pong_texture = blur_framebuffer.GetColorTexture(1);
        //auto& source_texture = FBOs[2].GetColorTexture(1);

        //// enable point filtering (nearest neighbor) on the ping-pong textures
        //point_sampler->Bind(0);
        //point_sampler->Bind(1);

        //// make sure the view port is resized to fit the entire texture
        //Renderer::SetViewport(ping_texture.width, ping_texture.height);
        //Renderer::DepthTest(false);

        //if (blur_framebuffer.Bind(); true) {
        //    blur_shader->Bind();
        //    source_texture.Bind(0);
        //    ping_texture.Bind(1);
        //    pong_texture.Bind(2);

        //    bool ping = true;  // read from ping, write to pong

        //    for (int i = 0; i < bloom_effect * 2; i++, ping = !ping) {
        //        blur_framebuffer.SetDrawBuffer(static_cast<GLuint>(ping));
        //        blur_shader->SetUniform(0, i == 0);
        //        blur_shader->SetUniform(1, ping);                
        //        Mesh::DrawQuad();
        //    }

        //    source_texture.Unbind(0);
        //    ping_texture.Unbind(1);
        //    pong_texture.Unbind(2);
        //    blur_shader->Unbind();
        //    blur_framebuffer.Unbind();
        //}

        //// after an even number of iterations, the blurred results are stored in texture "ping"
        //// also don't forget to restore the view port back to normal window size
        //Renderer::SetViewport(Window::width, Window::height);
    }

    void Scene01::BloomBlendPass() {
        //// finally we are back on the default framebuffer
        //auto& original_texture = FBOs[2].GetColorTexture(0);
        //auto& blurred_texture = FBOs[3].GetColorTexture(0);
        //
        //original_texture.Bind(0);
        //blurred_texture.Bind(1);
        //bilinear_sampler->Bind(1);  // bilinear filtering will introduce extra blurred effects

        //if (blend_shader->Bind(); true) {
        //    blend_shader->SetUniform(0, tone_mapping_mode);
        //    Renderer::Clear();
        //    Mesh::DrawQuad();
        //    blend_shader->Unbind();
        //}

        //bilinear_sampler->Unbind(0);
        //bilinear_sampler->Unbind(1);
    }

}
