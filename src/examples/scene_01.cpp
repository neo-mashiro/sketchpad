#include "pch.h"

#include "core/clock.h"
#include "core/input.h"
#include "core/window.h"
#include "components/all.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/path.h"

#include "examples/scene_01.h"

using namespace core;
using namespace components;

namespace scene {

    static bool show_plane = true;
    static float sphere_shininess = 32.0f;
    static float ball_shininess = 256.0f;

    static bool rotate_light = true;   // rotate the point light around the sphere?
    static float rotate_speed = 2.0f;  // rotation speed: radians per second
    static float radius = 3.0f;        // radius of the circle around which the point light rotates
    static float rotation_time = 0.0f;

    // this is called before the first frame, use this function to initialize your scene
    void Scene01::Init() {
        // name your scene title (will appear in the top menu)
        this->title = "Blinn Phong Reflection";

        // load assets upfront
        asset_ref<Shader>  s_skybox   = LoadAsset<Shader>(SHADER + "skybox\\36894.bin", 36894);
        asset_ref<Shader>  s_sphere   = LoadAsset<Shader>(SHADER + "01_sphere");
        asset_ref<Shader>  s_ball     = LoadAsset<Shader>(SHADER + "01_ball");
        asset_ref<Shader>  s_plane    = LoadAsset<Shader>(SHADER + "01_plane");
        asset_ref<Texture> t_skybox   = LoadAsset<Texture>(GL_TEXTURE_CUBE_MAP, SKYBOX + "5");
        asset_ref<Texture> t_plane    = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "0\\checkboard.png");
        asset_ref<Texture> t_diffuse  = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "3\\diffuse.jpg");
        asset_ref<Texture> t_specular = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "3\\specular.jpg");

        // main camera
        camera = CreateEntity("Camera", ETag::MainCamera);
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, 4.5f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(View::Perspective);
        //camera.AddComponent<Spotlight>(color::red, 1.0f);  // a spotlight attached on the camera is a flashlight
        //camera.GetComponent<Spotlight>().SetCutoff(3.0f);

        // skybox
        skybox = CreateEntity("Skybox", ETag::Skybox);
        skybox.AddComponent<Mesh>(Primitive::Cube);
        skybox.GetComponent<Material>().SetShader(s_skybox);
        skybox.GetComponent<Material>().SetTexture(0, t_skybox);

        // directional light
        direct_light = CreateEntity("Directional Light");
        direct_light.GetComponent<Transform>().Rotate(glm::radians(-45.0f), world::right);
        direct_light.AddComponent<DirectionLight>(color::white, 0.4f);

        // point light
        point_light = CreateEntity("Point Light");
        point_light.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, radius));
        point_light.AddComponent<PointLight>(color::white, 0.8f);
        point_light.GetComponent<PointLight>().SetAttenuation(0.09f, 0.032f);

        // sphere
        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(Primitive::Sphere);
        sphere.GetComponent<Transform>().Translate(world::up * 3.0f);
        sphere.GetComponent<Transform>().Scale(2.0f);

        auto& m1 = sphere.GetComponent<Material>();
        m1.SetShader(s_sphere);
        m1.SetUniform(1, glm::vec3(0.0215f, 0.1745f, 0.0215f));
        m1.SetUniform(2, glm::vec3(0.07568f, 0.61424f, 0.07568f));
        m1.SetUniform(3, glm::vec3(0.633f, 0.727811f, 0.633f));
        m1.SetUniform(4, sphere_shininess, true);  // bind the uniform value to the variable (observer)

        // metallic ball
        ball = CreateEntity("Ball");
        ball.AddComponent<Mesh>(Primitive::Sphere);
        ball.GetComponent<Transform>().Translate(world::up * -2.0f);
        ball.GetComponent<Transform>().Scale(2.0f);

        auto& m2 = ball.GetComponent<Material>();
        m2.SetShader(s_ball);
        m2.SetTexture(0, t_diffuse);
        m2.SetTexture(1, t_specular);
        m2.SetUniform(1, ball_shininess, true);
        // m2.SetShader(nullptr);
        // m2.SetTexture(0, nullptr);

        // plane
        plane = CreateEntity("Plane");
        plane.AddComponent<Mesh>(Primitive::Plane);
        plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        plane.GetComponent<Transform>().Scale(1.0f);

        auto& m3 = plane.GetComponent<Material>();
        m3.SetShader(s_plane);
        m3.SetTexture(0, t_plane);
        m3.SetUniform(1, ball_shininess, true);

        Renderer::FaceCulling(true);
        Renderer::DepthTest(true);
    }

    // this is called every frame, update your scene here and submit the entities to the renderer
    void Scene01::OnSceneRender() {
        camera.GetComponent<Camera>().Update();

        if (rotate_light) {
            auto& transform = point_light.GetComponent<Transform>();
            float x = radius * sin(rotation_time * rotate_speed);
            float y = radius * cos(rotation_time * rotate_speed);
            rotation_time += Clock::delta_time;
            transform.Translate(glm::vec3(x, 0.0f, y) - transform.position);
        }

        // submit to the renderer a list of entities that you'd like to draw, the order of entities
        // in the list is important especially for stencil test and transparent blending, etc.
        // placing the skybox at the end of the render list can save us many draw calls since it is
        // the farthest in the scene, depth test will discard all pixels obstructed by other objects
        Renderer::Submit(
            { sphere.id, ball.id, show_plane ? plane.id : entt::null, skybox.id }
        );
    }

    // this is called every frame, update your ImGui widgets here to control entities in the scene
    void Scene01::OnImGuiRender() {
        static bool sphere_gizmo = false;
        static bool plane_gizmo = false;
        static bool edit_color = false;
        static int power_sphere = 5;
        static int power_ball = 8;

        static ImVec4 color = ImVec4(0.1075f, 0.8725f, 0.1075f, 0.0f);

        static ImGuiColorEditFlags color_flags
            = ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_NoPicker;

        ui::LoadInspectorConfig();

        if (ImGui::Begin("Inspector##1", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Checkbox("Show Plane", &show_plane);
            ImGui::Separator();
            ImGui::Checkbox("Point Light Rotation", &rotate_light);
            ImGui::Separator();
            ImGui::Checkbox("Show Sphere Gizmo", &sphere_gizmo);
            ImGui::Separator();
            ImGui::Checkbox("Show Plane Gizmo", &plane_gizmo);
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PushItemWidth(100.0f);
            ImGui::SliderInt("Sphere Shininess", &power_sphere, 1, 10);
            ImGui::SliderInt("Metalic Ball Shininess", &power_ball, 1, 10);
            ImGui::PopItemWidth();
            ImGui::Separator();
            ImGui::Checkbox("Edit Sphere Color", &edit_color);
            if (edit_color) {
                ImGui::Spacing();
                ImGui::Indent(15.0f);
                ImGui::ColorPicker3("##Sphere Color", (float*)&color, color_flags);
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
        ball_shininess = pow(2.0f, (float)power_ball);

        auto& m = sphere.GetComponent<Material>();
        m.SetUniform(1, glm::vec3(color.x, color.y, color.z) * 0.2f);  // ambient
        m.SetUniform(2, glm::vec3(color.x, color.y, color.z) * 0.7f);  // diffuse
    }
}
