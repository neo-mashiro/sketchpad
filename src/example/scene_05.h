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
        Entity moonlight;
        Entity lantern;

        Entity floor;
        Entity wall;
        Entity ball[3];
        Entity suzune;
        Entity pillars[8];

        asset_ref<Texture> irradiance_map;
        asset_ref<Texture> prefiltered_map;
        asset_ref<Texture> BRDF_LUT;

        void PrecomputeIBL(const std::string& hdri);
        void SetupMaterial(Material& pbr_mat, int mat_id);
    };

}
