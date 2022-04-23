#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene06 : public Scene {

        using Scene::Scene;

        void Init() override;
        void OnSceneRender() override;
        void OnImGuiRender() override;

        Entity camera;
        Entity skybox;
        Entity cathedral;
        Entity light[4];

        asset_ref<Texture> irradiance_map;
        asset_ref<Texture> prefiltered_map;
        asset_ref<Texture> BRDF_LUT;

        void PrecomputeIBL(const std::string& hdri);
        void SetupMaterial(Material& pbr_mat, int mat_id);
    };

}
