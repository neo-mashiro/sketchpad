#include "pch.h"

#include "core/input.h"
#include "core/log.h"
#include "components/all.h"
#include "components/assist.h"
#include "scene/scene.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/path.h"

using namespace core;
using namespace components;

namespace scene {

    Scene::Scene(const std::string& title) : title(title), directory() {}

    Scene::~Scene() {
        registry.each([this](auto id) {
            CORE_TRACE("Destroying entity: {0}", directory.at(id));
        });
        registry.clear();
    }

    Entity Scene::CreateEntity(const std::string& name, ETag tag) {
        Entity e = { name, registry.create(), &registry };
        e.AddComponent<Transform>();
        e.AddComponent<Tag>(tag);
        directory.emplace(e.id, e.name);
        return e;
    }

    void Scene::DestroyEntity(Entity e) {
        CORE_TRACE("Destroying entity: {0}", e.name);
        directory.erase(e.id);
        registry.destroy(e.id);
    }

    // this base class is being used to render our welcome screen
    static asset_ref<Texture> welcome_screen;
    static ImTextureID welcome_screen_texture_id;

    void Scene::Init() {
        Input::ShowCursor();
        welcome_screen = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "0\\welcome.png");
        welcome_screen_texture_id = (void*)(intptr_t)(welcome_screen->id);
    }

    void Scene::OnSceneRender() {
        Renderer::Clear();
    }

    void Scene::OnImGuiRender() {
        ui::DrawWelcomeScreen(welcome_screen_texture_id);
    }

}
