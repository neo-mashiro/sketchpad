#include "pch.h"

#include "core/clock.h"
#include "core/window.h"
#include "scene_02.h"

using namespace core;

namespace scene {

    // this is called before the first frame, use this function to initialize
    // your scene configuration, setup shaders, textures, lights, models, etc.
    void Scene02::Init() {
        glutSetWindowTitle("Skybox Reflection");
        Window::layer = Layer::Scene;

        // main camera
        camera = std::make_unique<Camera>();

        // skybox
        skybox_shader = std::make_unique<Shader>(CWD + "skybox\\");
        skybox_texture.push_back(Texture(GL_TEXTURE_CUBE_MAP, "skybox", CWD + "skybox\\"));  // rvalue
        skybox = std::make_unique<Mesh>(Primitive::Cube, skybox_texture);

        // reflective cube
        cube_shader = std::make_unique<Shader>(CWD + "cube\\");
        cube = std::make_unique<Mesh>(Primitive::Cube);  // no textures, reflect the skybox

        // reflective sphere
        sphere_shader = std::make_unique<Shader>(CWD + "sphere\\");
        sphere = std::make_unique<Mesh>(Primitive::Sphere);  // no textures, reflect the skybox
        sphere->M = glm::translate(sphere->M, glm::vec3(0, -3, -8));
        sphere->M = glm::scale(sphere->M, glm::vec3(3, 3, 3));

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
    void Scene02::OnSceneRender() {
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera->Update();

        glm::mat4 V = camera->GetViewMatrix();
        glm::mat4 P = camera->GetProjectionMatrix();

        // draw reflective cube
        cube_shader->Bind();
        {
            float shift = glm::sin(Clock::time) * 0.005f;
            cube->M = glm::rotate(cube->M, glm::radians(0.2f), glm::vec3(0, 1, 0));
            cube->M = glm::translate(cube->M, glm::mat3(glm::inverse(cube->M)) * glm::vec3(shift, 0, 0));

            cube_shader->SetMat4("u_MVP", P * V * cube->M);
            cube_shader->SetMat4("u_M", cube->M);
            cube_shader->SetVec3("camera_pos", camera->position);
            cube_shader->SetInt("skybox", 0);
            cube->Draw(*cube_shader);
        }
        cube_shader->Unbind();

        // draw reflective sphere
        sphere_shader->Bind();
        {
            sphere_shader->SetMat4("u_MVP", P * V * sphere->M);
            sphere_shader->SetMat4("u_M", sphere->M);
            sphere_shader->SetVec3("camera_pos", camera->position);
            sphere_shader->SetInt("skybox", 0);
            sphere->Draw(*sphere_shader);
        }
        sphere_shader->Unbind();

        // drawing skybox last can save us many draw calls, because it is the farthest
        // object in the scene, which should be rendered behind all other objects.
        // with depth test enabled, pixels that failed the test are skipped over, only
        // those that passed the test (not obstructed by other objects) are drawn.
        skybox_shader->Bind();
        glFrontFace(GL_CW);  // skybox has reversed winding order, we only draw the inner faces
        {
            // skybox is stationary, it doesn't move with the camera (fixed position)
            // so we use a rectified view matrix whose translation components are removed
            skybox_shader->SetMat4("u_MVP", P * glm::mat4(glm::mat3(V)) * skybox->M);
            skybox->Draw(*skybox_shader, true);
        }
        glFrontFace(GL_CCW);  // recover the global winding order
        skybox_shader->Unbind();
    }

    // this is called every frame, place your ImGui updates and draw calls here
    void Scene02::OnImGuiRender() {
        ImGui::ShowDemoWindow();
    }
}
