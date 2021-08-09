#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene01 : public Scene {

        using Scene::Scene;  // inherit the base constructor

        struct Material {
            glm::vec3 ambient;
            glm::vec3 diffuse;
            glm::vec3 specular;
            float shininess;
        };

        struct Light {
            glm::vec3 ambient;
            glm::vec3 diffuse;
            glm::vec3 specular;
        };

        Entity camera;
        Entity skybox;
        Entity light;
        Entity sphere;
        Entity ball;
        Entity plane;

        // override these functions in the cpp file to render your scene
        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;
    };
}
