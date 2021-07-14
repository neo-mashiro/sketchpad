#include "pch.h"

#include "core/window.h"
#include "scene/scene.h"

using namespace core;

namespace scene {
    // this is called before the first frame, use this function to initialize
    // your scene configuration, setup shaders, textures, lights, models, etc.
    void Scene::Init() {
        glutSetWindowTitle("Welcome to sketchpad");
        Window::layer = Layer::ImGui;
        glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
    }

    // this is called every frame, place your scene updates and draw calls here
    void Scene::OnSceneRender() {
        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // this is called every frame, place your ImGui updates and draw calls here
    void Scene::OnImGuiRender() {
        ImGui::ShowDemoWindow();
    }
}
