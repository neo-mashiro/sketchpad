#pragma once

#include "scene/scene.h"
#include "scene/camera.h"
#include "scene/shader.h"
#include "scene/texture.h"
#include "scene/mesh.h"
#include "scene/model.h"

namespace scene {

    class Scene02 : public Scene {

        using Scene::Scene;  // inherit the base constructor

        // declare your scene objects here in the header
        // use pointers to avoid dynamic initialization at startup, which happens
        // before main() is entered and so the OpenGL context has not yet been created.
        // most of our class constructors depend on the OpenGL context, without such
        // context, constructor calls will throw an exception to the console.

        // main camera
        std::unique_ptr<Camera> camera;

        // skybox
        std::unique_ptr<Mesh> skybox;
        std::unique_ptr<Shader> skybox_shader;
        std::vector<Texture> skybox_texture;

        // reflective cube
        std::unique_ptr<Mesh> cube;
        std::unique_ptr<Shader> cube_shader;

        // reflective sphere
        std::unique_ptr<Mesh> sphere;
        std::unique_ptr<Shader> sphere_shader;

        // override these functions in the cpp file to render your scene
        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;
    };
}
