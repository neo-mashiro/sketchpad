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
#include "example/scene_02.h"

using namespace core;
using namespace asset;
using namespace component;
using namespace utils;

namespace scene {

    static int eid = 0;  // 0: sphere, 1: torus, 2: cube, 3: teapot

    static vec3 sphere_color[7] { color::black };
    static float sphere_metalness[7] { 0.05f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f };  // tweak
    static float sphere_roughness[7] { 0.05f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f };  // tweak
    static float sphere_ao = 0.5f;

    static float cube_metalness = 0.5f;
    static float cube_roughness = 0.5f;
    static int cube_rotation = -1;  // -1: no rotation, 0: up, 1: left, 2: right, 3: down
    static int rotation_mode = 1;
    static bool reset_cube = false;

    static vec3 torus_color { color::white };
    static float torus_metalness = 0.5f;
    static float torus_roughness = 0.5f;
    static float torus_ao = 0.5f;
    static bool rotate_torus = false;

    static float teapot_metalness = 0.5f;
    static float teapot_roughness = 0.5f;
    static float teapot_ao = 0.5f;
    static bool rotate_teapot = false;
    static bool teapot_wireframe = false;

    static bool show_grid = false;
    static float grid_cell_size = 2.0f;
    static vec4 thin_line_color { 0.1f, 0.1f, 0.1f, 1.0f };
    static vec4 wide_line_color { 0.2f, 0.2f, 0.2f, 1.0f };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene02::Init() {
        this->title = "Environment Lighting (IBL)";
        PrecomputeIBL();

        resource_manager.Add(-1, MakeAsset<Mesh>(Primitive::Sphere));  // shared sphere mesh
        resource_manager.Add(-2, MakeAsset<Mesh>(Primitive::Cube));  // shared cube mesh

        resource_manager.Add(11, MakeAsset<Shader>(paths::shader + "scene_02\\reflect.glsl"));
        resource_manager.Add(12, MakeAsset<Shader>(paths::shader + "scene_02\\skybox.glsl"));
        resource_manager.Add(13, MakeAsset<Shader>(paths::shader + "scene_01\\light.glsl"));////////////////////////////////////////
        resource_manager.Add(14, MakeAsset<Shader>(paths::shader + "scene_01\\blur.glsl"));/////////////////////////////////////////////////
        resource_manager.Add(15, MakeAsset<Shader>(paths::shader + "scene_01\\blend.glsl"));//////////////////////////////////////////////////////
        resource_manager.Add(16, MakeAsset<Shader>(paths::shader + "core\\infinite_grid.glsl"));

        resource_manager.Add(21, MakeAsset<Material>(resource_manager.Get<Shader>(11)));  // pbr material
        resource_manager.Add(22, MakeAsset<Material>(resource_manager.Get<Shader>(12)));  // skybox material
        resource_manager.Add(23, MakeAsset<Material>(resource_manager.Get<Shader>(13)));  // light material

        resource_manager.Add(30, MakeAsset<Texture>(paths::texture + "aaa\\albedo.jpg"));/////////////////////////////////////
        resource_manager.Add(31, MakeAsset<Texture>(paths::texture + "aaa\\normal.jpg"));//////////////////////////////////////
        resource_manager.Add(32, MakeAsset<Texture>(paths::texture + "aaa\\roughness.jpg"));//////////////////////////////////////
        resource_manager.Add(33, MakeAsset<Texture>(paths::texture + "aaa\\metalness.jpg"));//////////////////////////////////////////
        resource_manager.Add(34, MakeAsset<Texture>(paths::texture + "bbb\\albedo.jpg"));//////////////////////////////////////////
        resource_manager.Add(35, MakeAsset<Texture>(paths::texture + "bbb\\normal.jpg"));//////////////////////////////////////////
        resource_manager.Add(36, MakeAsset<Texture>(paths::texture + "bbb\\roughness.jpg"));//////////////////////////////////////////
        resource_manager.Add(37, MakeAsset<Texture>(paths::texture + "bbb\\ao.jpg"));//////////////////////////////////////////
        resource_manager.Add(38, MakeAsset<Texture>(paths::texture + "ccc\\albedo.jpg"));//////////////////////////////////////////
        resource_manager.Add(39, MakeAsset<Texture>(paths::texture + "ccc\\normal.jpg"));//////////////////////////////////////////
        resource_manager.Add(40, MakeAsset<Texture>(paths::texture + "ccc\\roughness.jpg"));//////////////////////////////////////////

        resource_manager.Add(41, MakeAsset<Sampler>(FilterMode::Point));  // point sampler
        resource_manager.Add(42, MakeAsset<Sampler>(FilterMode::Bilinear));  // bilinear sampler
        
        AddUBO(resource_manager.Get<Shader>(11)->ID());
        AddUBO(resource_manager.Get<Shader>(12)->ID());

        AddFBO(Window::width, Window::height);          // bloom filter pass
        AddFBO(Window::width, Window::height);          // MSAA resolve pass
        AddFBO(Window::width / 2, Window::height / 2);  // Gaussian blur pass

        FBOs[0].AddColorTexture(2, true);    // multisampled textures for MSAA
        FBOs[0].AddDepStRenderBuffer(true);  // multisampled RBO for MSAA
        FBOs[1].AddColorTexture(2);
        FBOs[2].AddColorTexture(2);

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(0.0f, 6.0f, 9.0f);
        camera.AddComponent<Camera>(View::Perspective);
        
        // skybox
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        if (auto& mat = skybox.AddComponent<Material>(resource_manager.Get<Material>(22)); true) {
            mat.SetTexture(0, prefiltered_map);  // environment map is in base level 0
            mat.SetUniform(3, 1.0f);  // skybox brightness
        }

        // create 10 spheres (3 w/ textures + 7 w/o textures)
        const float sphere_posx[] = { 0.0f, -1.5f, 1.5f, -3.0f, 0.0f, 3.0f, -4.5f, -1.5f, 1.5f, 4.5f };
        const float sphere_posy[] = { 10.5f, 7.5f, 7.5f, 4.5f, 4.5f, 4.5f, 1.5f, 1.5f, 1.5f, 1.5f };
        auto sphere_mesh = resource_manager.Get<Mesh>(-1);

        for (int i = 0; i < 10; i++) {
            sphere[i] = CreateEntity("Sphere " + std::to_string(i));
            sphere[i].GetComponent<Transform>().Translate(-world::right * sphere_posx[i]);
            sphere[i].GetComponent<Transform>().Translate(world::up * sphere_posy[i]);
            sphere[i].AddComponent<Mesh>(sphere_mesh);

            auto& mat = sphere[i].AddComponent<Material>(resource_manager.Get<Material>(21));
            mat.SetTexture(0, irradiance_map);
            mat.SetTexture(1, prefiltered_map);
            mat.SetTexture(2, BRDF_LUT);

            if (i == 0) {
                mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(30));
                mat.SetTexture(pbr_t::normal, resource_manager.Get<Texture>(31));
                mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(32));
                mat.SetTexture(pbr_t::metallic, resource_manager.Get<Texture>(33));
                mat.SetUniform(pbr_u::uv_scale, vec2(2.0f, 2.0f));
            }
            else if (i == 1) {
                mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(34));
                mat.SetTexture(pbr_t::normal, resource_manager.Get<Texture>(35));
                mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(36));
                mat.SetTexture(pbr_t::ao, resource_manager.Get<Texture>(37));
                mat.SetUniform(pbr_u::metalness, 0.0f);
                mat.SetUniform(pbr_u::uv_scale, vec2(3.0f, 3.0f));
            }
            else if (i == 2) {
                mat.SetTexture(pbr_t::albedo, resource_manager.Get<Texture>(38));
                mat.SetTexture(pbr_t::normal, resource_manager.Get<Texture>(39));
                mat.SetTexture(pbr_t::roughness, resource_manager.Get<Texture>(40));
                mat.SetUniform(pbr_u::metalness, 0.0f);
                mat.SetUniform(pbr_u::uv_scale, vec2(3.0f, 3.0f));
            }
            else {
                unsigned int offset = i - 3;
                sphere_color[offset] = utils::math::HSV2RGB(offset / 7.0f, 0.9f, 0.9f);

                // set PBR attributes
                mat.BindUniform(pbr_u::albedo, sphere_color + offset);
                mat.BindUniform(pbr_u::metalness, sphere_metalness + offset);
                mat.BindUniform(pbr_u::roughness, sphere_roughness + offset);
                mat.BindUniform(pbr_u::ao, &sphere_ao);
            }
        }

        // create 3 cubes (2 translation + 1 rotation)
        for (int i = 0; i < 3; i++) {
            cube[i] = CreateEntity("Cube " + std::to_string(i));
            cube[i].AddComponent<Mesh>(resource_manager.Get<Mesh>(-2));
            cube[i].GetComponent<Transform>().Translate(-world::right * (-6.0f + 6.0f * i));
            cube[i].GetComponent<Transform>().Translate(world::up * 5.0f);

            auto& mat = cube[i].AddComponent<Material>(resource_manager.Get<Material>(21));
            mat.SetTexture(0, irradiance_map);
            mat.SetTexture(1, prefiltered_map);
            mat.SetTexture(2, BRDF_LUT);

            mat.BindUniform(pbr_u::metalness, &cube_metalness);
            mat.BindUniform(pbr_u::roughness, &cube_roughness);
            mat.SetUniform(pbr_u::ao, 0.5f);
        }

        point_light = CreateEntity("Point Light");
        point_light.AddComponent<Mesh>(sphere_mesh);
        point_light.GetComponent<Transform>().Translate(world::up * 6.0f);
        point_light.GetComponent<Transform>().Translate(world::forward * -4.0f);
        point_light.GetComponent<Transform>().Scale(0.1f);
        point_light.AddComponent<PointLight>(color::orange, 1.8f);
        point_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

        // this is the only analytic light source in the scene, so we setup only once, no need to bind
        if (auto& mat = point_light.AddComponent<Material>(resource_manager.Get<Material>(23)); true) {
            auto& pl = point_light.GetComponent<PointLight>();
            mat.SetUniform(3, pl.color);
            mat.SetUniform(4, pl.intensity);
            mat.SetUniform(5, 2.0f);
        }

        torus = CreateEntity("Torus");
        torus.AddComponent<Mesh>(Primitive::Torus);
        torus.GetComponent<Transform>().Translate(world::up * 5.0f);
        torus.GetComponent<Transform>().Scale(1.0f);////////////////////////////////////////////////////////////////////////////////////

        if (auto& mat = torus.AddComponent<Material>(resource_manager.Get<Material>(21)); true) {
            mat.SetTexture(0, irradiance_map);
            mat.SetTexture(1, prefiltered_map);
            mat.SetTexture(2, BRDF_LUT);
            mat.BindUniform(pbr_u::albedo, &torus_color);
            mat.BindUniform(pbr_u::metalness, &torus_metalness);
            mat.BindUniform(pbr_u::roughness, &torus_roughness);
            mat.BindUniform(pbr_u::ao, &torus_ao);
        }

        teapot = CreateEntity("Teapot");
        teapot.GetComponent<Transform>().Translate(world::up * 4.0f);
        teapot.GetComponent<Transform>().Scale(1.0f);

        if (auto& model = teapot.AddComponent<Model>(paths::model + "teapot.obj", Quality::Auto); true) {
            // import settings (a model may have multiple materials)
            if (auto& mat = model.SetMaterial("DefaultMaterial", resource_manager.Get<Material>(21)); true) {
                mat.SetUniform(pbr_u::albedo, color::lime);
                mat.BindUniform(pbr_u::metalness, &teapot_metalness);  // bind to ImGui float
                mat.BindUniform(pbr_u::roughness, &teapot_roughness);  // bind to ImGui float
                mat.BindUniform(pbr_u::ao, &teapot_ao);
                mat.BindUniform(0, &teapot_wireframe);
                mat.SetTexture(0, irradiance_map);
                mat.SetTexture(1, prefiltered_map);
                mat.SetTexture(2, BRDF_LUT);
            }
        }
    }

    void Scene02::OnSceneRender() {
        UpdateEntities();
        UpdateUBOs();

        //////////// bloom filter pass
        if (FBO& framebuffer = FBOs[0]; true) {
            framebuffer.Clear();
            framebuffer.Bind();
            Renderer::MSAA(true);
            Renderer::DepthTest(true);
            Renderer::FaceCulling(true);
            Renderer::AlphaBlend(true);

            switch (eid) {
                case 0: for (int i = 0; i < 10; i++) Renderer::Submit(sphere[i].id); break;
                case 1: Renderer::Submit(torus.id); break;
                case 2: for (int i = 0; i < 3; i++) Renderer::Submit(cube[i].id); break;
                case 3: {
                    // two-sided shading: disable fc, render right away, restore fc
                    Renderer::FaceCulling(false);
                    Renderer::Submit(teapot.id);
                    Renderer::Render();
                    Renderer::FaceCulling(true);
                }
            }

            Renderer::Submit(point_light.id);
            Renderer::Submit(skybox.id);
            Renderer::Render();

            if (show_grid) {
                auto grid_shader = resource_manager.Get<Shader>(16);
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

        auto blend_shader = resource_manager.Get<Shader>(15);

        if (blend_shader->Bind(); true) {
            blend_shader->SetUniform(0, 3);
            Renderer::Clear();
            Mesh::DrawQuad();
            blend_shader->Unbind();
        }

        bilinear_sampler->Unbind(0);
        bilinear_sampler->Unbind(1);
    }

    void Scene02::OnImGuiRender() {
        using namespace ImGui;
        static bool edit_sphere_metalness = false;
        static bool edit_sphere_roughness = false;

        // 3 x 3 cube rotation panel
        static const bool cell_enabled[9] = { false, true, false, true, false, true, false, true, false };
        static const char* cell_label[4] = { ICON_FK_LONG_ARROW_UP, ICON_FK_LONG_ARROW_LEFT, ICON_FK_LONG_ARROW_RIGHT, ICON_FK_LONG_ARROW_DOWN };
        static const ImVec2 cell_size = ImVec2(40, 40);

        static int z_mode = -1;  // cube gizmo mode

        static ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
        static ImVec4 thin_lc = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        static ImVec4 wide_lc = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

        if (ui::NewInspector()) {
            Indent(5.0f);
            Text("Entity to Render");
            Separator();
            RadioButton("Static Sphere", &eid, 0); SameLine(164);
            RadioButton("Color Torus",   &eid, 1);
            RadioButton("Spinning Cube", &eid, 2); SameLine(164);
            RadioButton("Utah Teapot",   &eid, 3);
            Separator();

            if (CollapsingHeader("Static Sphere", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Ambient Occlusion##1", &sphere_ao, 0.05f, 0.5f);                
                PopItemWidth();

                if (Checkbox("Edit Metalness", &edit_sphere_metalness); edit_sphere_metalness) {
                    PushID("Metalness Sliders");
                    for (int i = 0; i < 7; i++) {
                        float hue = i / 7.0f;
                        PushID(i);
                        PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(hue, 0.5f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(hue, 0.6f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(hue, 0.7f, 0.5f));
                        PushStyleColor(ImGuiCol_SliderGrab,     (ImVec4)ImColor::HSV(hue, 0.9f, 0.9f));

                        VSliderFloat("##m", ImVec2(20, 160), &sphere_metalness[i], 0.0f, 1.0f, "");
                        if (IsItemActive() || IsItemHovered()) {
                            SetTooltip("%.3f", sphere_metalness[i]);
                        }

                        PopStyleColor(4);
                        PopID();
                        SameLine();
                    }
                    PopID();
                    NewLine();
                }

                if (Checkbox("Edit Roughness", &edit_sphere_roughness); edit_sphere_roughness) {
                    PushID("Roughness Sliders");
                    for (int i = 0; i < 7; i++) {
                        float hue = i / 7.0f;
                        PushID(i);
                        PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(hue, 0.5f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(hue, 0.6f, 0.5f));
                        PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(hue, 0.7f, 0.5f));
                        PushStyleColor(ImGuiCol_SliderGrab,     (ImVec4)ImColor::HSV(hue, 0.9f, 0.9f));

                        VSliderFloat("##r", ImVec2(20, 160), &sphere_roughness[i], 0.01f, 1.0f, "");
                        if (IsItemActive() || IsItemHovered()) {
                            SetTooltip("%.3f", sphere_roughness[i]);
                        }

                        PopStyleColor(4);
                        PopID();
                        SameLine();
                    }
                    PopID();
                    NewLine();
                }
            }

            if (CollapsingHeader("Spinning Cube", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Metalness##2", &cube_metalness, 0.0f, 1.0f);
                SliderFloat("Roughness##2", &cube_roughness, 0.0f, 1.0f);
                PopItemWidth();
                Separator();

                Text("Rotation Mode");
                RadioButton("Local Space", &rotation_mode, 1); SameLine();
                RadioButton("World Space", &rotation_mode, 2);
                Separator();

                Text("Gizmo Edit Mode");
                RadioButton("T", &z_mode, 1); SameLine();
                RadioButton("R", &z_mode, 2); SameLine();
                RadioButton("S", &z_mode, 3); SameLine();
                RadioButton("N/A", &z_mode, 0);
                Separator();

                BeginGroup();
                for (int row = 0; row < 3; row++) {
                    for (int col = 0; col < 3; col++) {
                        int index = 3 * row + col;
                        ImVec2 alignment = ImVec2((float)col / 2.0f, (float)row / 2.0f);
                        PushStyleVar(ImGuiStyleVar_SelectableTextAlign, alignment);
                        PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);

                        if (col > 0) SameLine();
                        if (cell_enabled[index] && !reset_cube) {
                            int direction = (index - 1) / 2;
                            bool active = cube_rotation == direction;
                            if (Selectable(cell_label[direction], active, 0, cell_size)) {
                                cube_rotation = active ? -1 : direction;
                            }
                        }
                        else if (row == 1 && col == 1) {
                            ui::PushRotation();
                            Selectable(ICON_FK_REPEAT, false, ImGuiSelectableFlags_Disabled, cell_size);
                            ui::PopRotation(Clock::time * 4.0f, false);
                        }
                        else {
                            Selectable("##empty", false, ImGuiSelectableFlags_Disabled, cell_size);
                        }

                        if (index == 5) {
                            if (SameLine(170.0f); Button("RESET", ImVec2(80.0f, 42.0f))) {
                                reset_cube = true;
                            }
                        }

                        PopStyleVar(2);
                    }
                }
                EndGroup();
            }

            if (CollapsingHeader("Color Torus", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Metalness##3", &torus_metalness, 0.00f, 1.0f);
                SliderFloat("Roughness##3", &torus_roughness, 0.01f, 1.0f);
                SliderFloat("Ambient Occlusion##3", &torus_ao, 0.05f, 0.5f);
                PopItemWidth();
                Checkbox("Torus Rotation", &rotate_torus);
            }

            if (CollapsingHeader("Utah Teapot", ImGuiTreeNodeFlags_None)) {
                PushItemWidth(130.0f);
                SliderFloat("Metalness##4", &teapot_metalness, 0.00f, 1.0f);
                SliderFloat("Roughness##4", &teapot_roughness, 0.01f, 1.0f);
                SliderFloat("Ambient Occlusion##4", &teapot_ao, 0.05f, 0.5f);
                PopItemWidth();
                Checkbox("Teapot Rotation", &rotate_teapot);
                Checkbox("Wireframe Mode", &teapot_wireframe);
            }

            if (CollapsingHeader("Infinite Grid", ImGuiTreeNodeFlags_None)) {
                Checkbox("Show Grid Lines", &show_grid);
                PushItemWidth(130.0f);
                SliderFloat("Grid Cell Size", &grid_cell_size, 0.25f, 8.0f);
                PopItemWidth();

                if (ColorEdit4("Line Color Minor", (float*)&thin_lc, color_flags)) {
                    thin_line_color = vec4(thin_lc.x, thin_lc.y, thin_lc.z, 1.0f);
                }
                if (ColorEdit4("Line Color Main", (float*)&wide_lc, color_flags)) {
                    wide_line_color = vec4(wide_lc.x, wide_lc.y, wide_lc.z, 1.0f);
                }
            }

            Unindent(5.0f);
            ui::EndInspector();
        }

        if (eid == 2 && z_mode > 0) {
            ui::DrawGizmo(camera, cube[1], static_cast<ui::Gizmo>(z_mode));
        }
        else {
            ui::DrawGizmo(camera, point_light, ui::Gizmo::Translate);
        }
    }

    void Scene02::PrecomputeIBL() {
        Renderer::SeamlessCubemap(true);
        Renderer::DepthTest(false);
        Renderer::FaceCulling(true);

        const std::string hdri = "sky_linekotsi_27_HDRI.hdr";

        auto irradiance_shader = CShader(utils::paths::shader + "scene_02\\irradiance_map.glsl");
        auto prefilter_shader  = CShader(utils::paths::shader + "scene_02\\prefilter_envmap.glsl");
        auto envBRDF_shader    = CShader(utils::paths::shader + "scene_02\\environment_BRDF.glsl");

        irradiance_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
        prefiltered_map = MakeAsset<Texture>(utils::paths::texture + "test2\\" + hdri, 1024, 8);
        Texture::Copy(*prefiltered_map, 3, *irradiance_map, 0);
        BRDF_LUT = MakeAsset<Texture>(utils::paths::texture + "common\\checkboard.png", 1);
        Sync::WaitFinish();

        //auto env_map = MakeAsset<Texture>(utils::paths::texture + "test\\" + hdri, 2048, 0);
        //env_map->Bind(0);

        //irradiance_map  = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 128, 128, 6, GL_RGBA16F, 1);
        //prefiltered_map = MakeAsset<Texture>(GL_TEXTURE_CUBE_MAP, 2048, 2048, 6, GL_RGBA16F, 8);
        //BRDF_LUT        = MakeAsset<Texture>(GL_TEXTURE_2D, 1024, 1024, 1, GL_RG16F, 1);

        //CORE_INFO("Precomputing diffuse irradiance map from {0}", hdri);
        //irradiance_map->BindILS(0, 0, GL_WRITE_ONLY);

        //if (irradiance_shader.Bind(); true) {
        //    irradiance_shader.Dispatch(128 / 32, 128 / 32, 6);
        //    irradiance_shader.SyncWait(GL_TEXTURE_FETCH_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        //    auto irradiance_fence = Sync(0);
        //    irradiance_fence.ClientWaitSync();
        //    irradiance_map->UnbindILS(0);
        //}

        //CORE_INFO("Precomputing specular prefiltered envmap from {0}", hdri);
        //Texture::Copy(*env_map, 0, *prefiltered_map, 0);  // copy the base level

        //const GLuint max_level = prefiltered_map->n_levels - 1;
        //GLuint resolution = prefiltered_map->width / 2;
        //prefilter_shader.Bind();

        //for (unsigned int level = 1; level <= max_level; level++, resolution /= 2) {
        //    float roughness = level / static_cast<float>(max_level);
        //    GLuint n_groups = glm::max<GLuint>(resolution / 32, 1);

        //    prefiltered_map->BindILS(level, 1, GL_WRITE_ONLY);
        //    prefilter_shader.SetUniform(0, roughness);
        //    prefilter_shader.Dispatch(n_groups, n_groups, 6);
        //    prefilter_shader.SyncWait(GL_TEXTURE_FETCH_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

        //    auto prefilter_fence = Sync(level);
        //    prefilter_fence.ClientWaitSync();
        //    prefiltered_map->UnbindILS(1);
        //}

        //CORE_INFO("Precomputing specular environment BRDF from {0}", hdri);
        //BRDF_LUT->BindILS(0, 2, GL_WRITE_ONLY);

        //if (envBRDF_shader.Bind(); true) {
        //    envBRDF_shader.Dispatch(1024 / 32, 1024 / 32, 1);
        //    envBRDF_shader.SyncWait(GL_ALL_BARRIER_BITS);
        //    Sync::WaitFinish();
        //    BRDF_LUT->UnbindILS(2);
        //}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    void Scene02::UpdateEntities() {
        auto& main_camera = camera.GetComponent<Camera>();
        main_camera.Update();

        for (int i = 0; i < 3; i++) {
            auto& transform = cube[i].GetComponent<Transform>();

            if (i == 0) {
                transform.Translate(world::up * (cos(Clock::time * 1.5f) * 0.02f));  // 1.5 speed, 0.02 magnitude
            }
            else if (i == 2) {
                transform.Translate(world::up * (-cos(Clock::time * 1.5f) * 0.02f));
            }
            else {
                // middle cube rotation specified by an axis + an angle in degrees
                switch (cube_rotation) {
                    case 0: transform.Rotate(world::right, -0.5f, static_cast<Space>(rotation_mode)); break;
                    case 1: transform.Rotate(world::up,    -0.5f, static_cast<Space>(rotation_mode)); break;
                    case 2: transform.Rotate(world::up,    +0.5f, static_cast<Space>(rotation_mode)); break;
                    case 3: transform.Rotate(world::right, +0.5f, static_cast<Space>(rotation_mode)); break;
                    case -1: default: break;
                }
            }
        }

        if (reset_cube) {
            static const auto cube_origin = vec3(0.0f, 5.0f, 0.0f);
            auto& T = cube[1].GetComponent<Transform>();
            float t = math::EaseFactor(5.0f, Clock::delta_time);
            T.SetPosition(math::Lerp(T.position, cube_origin, t));
            T.SetRotation(math::SlerpRaw(T.rotation, world::eye, t));
            cube_rotation = -1;

            if (math::Equals(T.position, cube_origin) && math::Equals(T.rotation, world::eye)) {
                reset_cube = false;
            }
        }

        torus_color = math::HSV2RGB(vec3(fmodf(Clock::delta_time * 0.05f, 1.0f), 1.0f, 1.0f));

        if (rotate_torus) {
            torus.GetComponent<Transform>().Rotate(world::right, 0.36f, Space::Local);
        }

        if (rotate_teapot) {
            teapot.GetComponent<Transform>().Rotate(world::up, 0.18f, Space::Local);
        }
    }

    void Scene02::UpdateUBOs() {
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
    }

}
