#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene01 : public Scene {

        using Scene::Scene;  // inherit the base constructor

        Entity camera;
        Entity skybox;
        Entity direct_light;
        Entity point_light;
        Entity sphere;
        Entity ball;
        Entity plane;

        // override these functions in the cpp file to render your scene
        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;
    };
}
