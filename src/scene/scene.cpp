#include "pch.h"

#include "core/window.h"
#include "scene/scene.h"
#include "scene/texture.h"

using namespace core;

namespace scene {

    static std::unique_ptr<Texture> welcome_screen;

    // this is called before the first frame, use this function to initialize
    // your scene configuration, setup shaders, textures, lights, models, etc.
    void Scene::Init() {
        glutSetWindowTitle("Welcome Screen");
        glutSetCursor(GLUT_CURSOR_INHERIT);  // show cursor
        Window::layer = Layer::ImGui;
        welcome_screen = std::make_unique<Texture>(GL_TEXTURE_2D, "albedo", RES + "welcome.jpg");
    }

    // this is called every frame, place your scene updates and draw calls here
    void Scene::OnSceneRender() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // this is called every frame, place your ImGui updates and draw calls here
    void Scene::OnImGuiRender() {
        // this base class is being used to render our welcome screen
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        ImTextureID id = (void*)(intptr_t)(welcome_screen->id);
        float win_w = (float)Window::width;
        float win_h = (float)Window::height;
        draw_list->AddImage(id, ImVec2(0.0f, 0.0f), ImVec2(win_w, win_h));
    }
}
