#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene05 : public Scene {

        using Scene::Scene;

        void Init() override;
        void OnSceneRender() override;
        void OnImGuiRender() override;

        Entity camera;
        Entity skybox;
        Entity point_light;
        Entity spotlight;
        Entity area_light;

        Entity floor;
        Entity wall;
        Entity ball[3];
        Entity suzune;
        Entity vehicle;

        asset_ref<Texture> irradiance_map;
        asset_ref<Texture> prefiltered_map;
        asset_ref<Texture> BRDF_LUT;

        void PrecomputeIBL();
        void SetupMaterial(Material& pbr_mat, int mat_id);
    };

}
