#include "pch.h"

#include "core/clock.h"
#include "core/window.h"
#include "scene_01.h"
#include "utils/ui.h"

using namespace core;

namespace scene {

    // this is called before the first frame, use this function to initialize
    // your scene configuration, setup shaders, textures, lights, models, etc.
    void Scene01::Init() {
        glutSetWindowTitle("Colorful Cubes");
        glutSetCursor(GLUT_CURSOR_NONE);  // hide cursor
        Window::layer = Layer::Scene;

        // main camera
        camera = std::make_unique<Camera>();

        // skybox
        skybox_shader = std::make_unique<Shader>(CWD + "skybox\\");
        skybox_texture.push_back(Texture(GL_TEXTURE_CUBE_MAP, "skybox", CWD + "skybox\\"));  // rvalue
        skybox = std::make_unique<Mesh>(Primitive::Cube, skybox_texture);

        // cubes
        cube_shader = std::make_unique<Shader>(CWD + "cube\\");
        for (int x = -3; x <= 3; x++) {
            for (int y = -3; y <= 3; y++) {
                for (int z = -3; z <= 3; z++) {
                    Mesh cube = Mesh(Primitive::Cube);
                    cube.M = glm::translate(cube.M, glm::vec3(x, y, z) * 0.7f);
                    cube.M = glm::scale(cube.M, glm::vec3(0.1f));
                    cubes.push_back(std::move(cube));
                }
            }
        }

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
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera->Update();

        glm::mat4 V = camera->GetViewMatrix();
        glm::mat4 P = camera->GetProjectionMatrix();

        // draw cubes
        cube_shader->Bind();
        {
            for (unsigned int i = 0; i < cubes.size(); i++) {
                cube_shader->SetMat4("u_MVP", P * V * cubes[i].M);
                cube_shader->SetVec4("u_color", cubes[i].M[3] / 1.6f + 0.5f);
                cubes[i].Draw(*cube_shader);
            }
        }
        cube_shader->Unbind();

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
    void Scene01::OnImGuiRender() {
        ImGui::ShowDemoWindow();
    }
}
