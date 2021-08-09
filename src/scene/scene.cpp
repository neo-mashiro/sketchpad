#include "pch.h"

#include "core/input.h"
#include "core/window.h"
#include "components/all.h"
#include "scene/scene.h"

using namespace core;
using namespace components;

namespace scene {

    Entity Scene::CreateEntity(const std::string& name) {
        Entity e = { name, registry.create(), &registry };
        e.AddComponent<Transform>();
        return e;
    }

    void Scene::DestroyEntity(Entity entity) {
        registry.destroy(entity.id);
    }

    static std::unique_ptr<Texture> welcome_screen;

    void Scene::Init() {
        Window::Rename("Welcome Screen");
        Window::layer = Layer::ImGui;
        Input::ShowCursor();
        welcome_screen = std::make_unique<Texture>(GL_TEXTURE_2D, TEXTURE + "0\\welcome.jpg");
    }

    void Scene::OnSceneRender() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Scene::OnImGuiRender() {
        // this base class is being used to render our welcome screen
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        ImTextureID id = (void*)(intptr_t)(welcome_screen->id);
        float win_w = (float)Window::width;
        float win_h = (float)Window::height;
        draw_list->AddImage(id, ImVec2(0.0f, 0.0f), ImVec2(win_w, win_h));
    }

}
