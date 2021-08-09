#include "pch.h"

#include "core/clock.h"
#include "core/input.h"
#include "core/window.h"
#include "components/all.h"
#include "scene/ui.h"
#include "scene_01.h"

using namespace core;
using namespace components;

namespace scene {

    // define your scene-level global variables here, remember to use the `static`
    // keyword to enforce internal linkage, doing so can keep the namespace clean
    // and avoid potential name conflicts across multiple scenes. By making these
    // variables "global", you should be aware that they will survive the entire
    // lifecycle of the application inside this local script. This means that,
    // if you temporarily switched to another scene and then switch back, their
    // values will remain unchanged, so your previous settings are preserved.

    // note: global variables should be POD types such as floats, pointers, simple
    // vectors or structs, they must be independent from each other since the order
    // of static initialization is undefined. Avoid object types that would trigger
    // dynamic initialization, which occurs before the OpenGL context is created.

    glm::mat4 V, P;

    static bool show_plane = true;
    static int ball_shininess = 4;

    static bool rotate_light = true;   // rotate the light cube around the sphere?
    static float rotate_speed = 1.5f;  // rotation speed: radians per second
    static float radius = 2.5f;        // radius of the circle around which the light cube rotates
    static float rotation_time = 0.0f;

    // this is called before the first frame, use this function to initialize
    // your scene configuration, setup shaders, textures, lights, models, etc.
    void Scene01::Init() {
        Window::Rename("Blinn Phong Reflection");
        Window::layer = Layer::Scene;
        Input::ResetCursor();
        Input::HideCursor();

        // main camera
        camera = CreateEntity("Camera");
        camera.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, 4.5f));
        camera.GetComponent<Transform>().Rotate(glm::radians(180.0f), world::up);
        camera.AddComponent<Camera>(&camera.GetComponent<Transform>(), View::Perspective);

        // skybox
        skybox = CreateEntity("Skybox");
        skybox.AddComponent<Mesh>(Primitive::Cube);
        //skybox.AddComponent<Shader>(GLSL + "skybox\\", true);
        skybox.AddComponent<Shader>(GLSL + "skybox\\1.bin", (GLenum)1);
        skybox.GetComponent<Shader>().PushTexture(0, GL_TEXTURE_CUBE_MAP, SKYBOX + "5\\");

        // point light
        light = CreateEntity("Point Light");
        light.AddComponent<Light>(world::unit, world::unit, world::unit);
        light.GetComponent<Transform>().Translate(glm::vec3(0.0f, 2.5f, radius));
        light.GetComponent<Transform>().Scale(0.01f);

        // sphere
        sphere = CreateEntity("Sphere");
        sphere.AddComponent<Mesh>(Primitive::Sphere);
        sphere.AddComponent<Shader>(CWD + "sphere\\");
        sphere.GetComponent<Shader>().Save();
        sphere.GetComponent<Transform>().Translate(world::up * 3.0f);
        sphere.GetComponent<Transform>().Scale(2.0f);
        sphere.AddComponent<Material>(  // these should be shader attributes!!!!!!!!!!!!!!!!
            glm::vec3(0.0215f, 0.1745f, 0.0215f),
            glm::vec3(0.07568f, 0.61424f, 0.07568f),
            glm::vec3(0.633f, 0.727811f, 0.633f), 16.0f
        );

        // metalic ball
        ball = CreateEntity("Ball");
        ball.AddComponent<Mesh>(Primitive::Sphere);
        ball.GetComponent<Transform>().Translate(world::up * -2.0f);
        ball.GetComponent<Transform>().Scale(2.0f);
        ball.AddComponent<Shader>(CWD + "ball\\1.bin", (GLenum)1);
        ball.GetComponent<Shader>().PushTexture(0, GL_TEXTURE_2D, TEXTURE + "3\\diffuse.jpg");
        ball.GetComponent<Shader>().PushTexture(1, GL_TEXTURE_2D, TEXTURE + "3\\specular.jpg");
        ball.GetComponent<Shader>().PushTexture(2, GL_TEXTURE_2D, TEXTURE + "3\\emission.jpg");

        // plane
        //plane = CreateEntity("Plane");
        //plane.AddComponent<Mesh>(Primitive::Plane);
        //plane.GetComponent<Transform>().Translate(world::up * -4.0f);
        //plane.GetComponent<Transform>().Scale(0.75f);
        //plane.AddComponent<Shader>(CWD + "plane\\");
        //plane.GetComponent<Shader>().PushTexture(0, GL_TEXTURE_2D, TEXTURE + "0\\checkboard.png");
        //plane.AddComponent<Material>(world::zero, world::zero, world::unit * 0.51f, 4.0f);

        // enable face culling
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);

        // enable depth test
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthRange(0.0f, 1.0f);
    }

    // this is called every frame, place your scene updates and draw calls here
    void Scene01::OnSceneRender() {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.GetComponent<Camera>().Update();

        V = camera.GetComponent<Camera>().GetViewMatrix();
        P = camera.GetComponent<Camera>().GetProjectionMatrix();

        if (rotate_light) {
            auto& transform = light.GetComponent<Transform>();
            float x = radius * sin(rotation_time * rotate_speed);
            float y = radius * cos(rotation_time * rotate_speed);
            rotation_time += Clock::delta_time;
            transform.Translate(glm::vec3(x, 0.0f, y) - transform.position);
        }
        
        //if (auto& shader = plane.GetComponent<Shader>(); shader.Bind() && show_plane) {
        //    shader.SetMat4("u_M", plane.GetComponent<Transform>().transform);
        //    shader.SetMat4("u_MVP", P * V * plane.GetComponent<Transform>().transform);
        //    shader.SetVec3("u_camera_pos", camera.GetComponent<Transform>().position);
        //    shader.SetVec3("u_light_src",
        //        glm::vec3(light.GetComponent<Transform>().transform * glm::vec4(world::origin, 1.0f)));
        //    shader.SetVec3("u_light.ambient", light.GetComponent<Light>().ambient);
        //    shader.SetVec3("u_light.diffuse", light.GetComponent<Light>().diffuse);
        //    shader.SetVec3("u_light.specular", light.GetComponent<Light>().specular);
        //    shader.SetVec3("u_material.specular", plane.GetComponent<Material>().specular);
        //    shader.SetFloat("u_material.shininess", plane.GetComponent<Material>().shininess);
        //    plane.GetComponent<Mesh>().Draw();
        //    shader.Unbind();
        //}

        if (auto& shader = sphere.GetComponent<Shader>(); shader.Bind()) {
            shader.SetMat4("u_M", sphere.GetComponent<Transform>().transform);
            shader.SetMat4("u_MVP", P * V * sphere.GetComponent<Transform>().transform);
            shader.SetVec3("u_camera_pos", camera.GetComponent<Transform>().position);
            shader.SetVec3("u_light_src",
                glm::vec3(light.GetComponent<Transform>().transform * glm::vec4(world::origin, 1.0f)));
            shader.SetVec3("u_light.ambient", light.GetComponent<Light>().ambient);
            shader.SetVec3("u_light.diffuse", light.GetComponent<Light>().diffuse);
            shader.SetVec3("u_light.specular", light.GetComponent<Light>().specular);
            shader.SetVec3("u_material.ambient", sphere.GetComponent<Material>().ambient);
            shader.SetVec3("u_material.diffuse", sphere.GetComponent<Material>().diffuse);
            shader.SetVec3("u_material.specular", sphere.GetComponent<Material>().specular);
            shader.SetFloat("u_material.shininess", sphere.GetComponent<Material>().shininess);
            sphere.GetComponent<Mesh>().Draw();
            shader.Unbind();
        }

        if (auto& shader = ball.GetComponent<Shader>(); shader.Bind()) {
            shader.SetMat4("u_M", ball.GetComponent<Transform>().transform);
            shader.SetMat4("u_MVP", P * V * ball.GetComponent<Transform>().transform);
            shader.SetVec3("u_camera_pos", camera.GetComponent<Transform>().position);
            shader.SetVec3("u_light_src",
                glm::vec3(light.GetComponent<Transform>().transform * glm::vec4(world::origin, 1.0f)));
            shader.SetVec3("u_light.ambient", light.GetComponent<Light>().ambient);
            shader.SetVec3("u_light.diffuse", light.GetComponent<Light>().diffuse);
            shader.SetVec3("u_light.specular", light.GetComponent<Light>().specular);
            shader.SetFloat("u_shininess", pow(2.0f, (float)ball_shininess));
            ball.GetComponent<Mesh>().Draw();
            shader.Unbind();
        }

        // drawing skybox last can save us many draw calls, because it is the farthest
        // object in the scene, which should be rendered behind all other objects.
        // with depth test enabled, pixels that failed the test are skipped over, only
        // those that passed the test (not obstructed by other objects) are drawn.
        if (auto& shader = skybox.GetComponent<Shader>(); shader.Bind()) {
            glFrontFace(GL_CW);  // skybox has reversed winding order, we only draw the inner faces
            // skybox is stationary, it doesn't move with the camera (fixed position)
            // so we use a rectified view matrix whose translation components are removed
            glm::mat4 M = skybox.GetComponent<Transform>().transform;
            shader.SetMat4("u_MVP", P * glm::mat4(glm::mat3(V)) * M);
            skybox.GetComponent<Mesh>().Draw();
            glFrontFace(GL_CCW);  // recover the global winding order
            shader.Unbind();
        }


    }

    // this is called every frame, place your ImGui updates and draw calls here
    void Scene01::OnImGuiRender() {
        static bool edit_color = false;
        static int shininess_power = 4;
        static ImVec4 color = ImVec4(0.1075f, 0.8725f, 0.1075f, 0.0f);

        static ImGuiColorEditFlags color_flags
            = ImGuiColorEditFlags_NoSidePreview
            | ImGuiColorEditFlags_PickerHueWheel
            | ImGuiColorEditFlags_DisplayRGB
            | ImGuiColorEditFlags_NoPicker;

        ui::LoadInspectorConfig();
        ImGui::Begin("Inspector##1", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Checkbox("Show Plane", &show_plane);
        ImGui::Separator();
        ImGui::Checkbox("Point Light Rotation", &rotate_light);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushItemWidth(100.0f);
        ImGui::SliderInt("Sphere Shininess", &shininess_power, 1, 10);
        ImGui::SliderInt("Metalic Ball Shininess", &ball_shininess, 1, 10);
        ImGui::PopItemWidth();
        ImGui::Separator();
        ImGui::Checkbox("Edit Sphere Color", &edit_color);
        if (edit_color) {
            ImGui::Spacing();
            ImGui::Indent(15.0f);
            ImGui::ColorPicker3("##Sphere Color", (float*)&color, color_flags);
            ImGui::Unindent(15.0f);
        }

        ImGui::End();

        if (true) {
            ui::DrawGizmo(V, P, sphere.GetComponent<Transform>());
        }

        sphere.GetComponent<Material>().shininess = pow(2.0f, (float)shininess_power);
        sphere.GetComponent<Material>().ambient = glm::vec3(color.x, color.y, color.z) * 0.2f;
        sphere.GetComponent<Material>().diffuse = glm::vec3(color.x, color.y, color.z) * 0.7f;
    }
}
