#include "pch.h"

#include "core/input.h"
#include "core/window.h"
#include "components/all.h"
#include "scene/scene.h"
#include "scene/renderer.h"
#include "scene/ui.h"

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

    // this base class is being used to render our welcome screen
    static std::unique_ptr<Texture> welcome_screen;
    static ImTextureID welcome_screen_texture_id;

    void Scene::Init() {
        Window::Rename("Welcome Screen");
        Window::layer = Layer::ImGui;
        Input::ShowCursor();

        welcome_screen = std::make_unique<Texture>(GL_TEXTURE_2D, IMAGE + "welcome.png");
        welcome_screen_texture_id = (void*)(intptr_t)(welcome_screen->id);
    }

    void Scene::OnSceneRender() {
        Renderer::ClearBuffer();
    }

    void Scene::OnImGuiRender() {
        ui::DrawWelcomeScreen(welcome_screen_texture_id);
    }

}
