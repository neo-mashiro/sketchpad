#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene03 : public Scene {

        using Scene::Scene;

        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;

        Entity camera;
        Entity skybox;
        Entity direct_light;

        Entity pistol;   // clear coat
        Entity helmet;   // anisotropy
        Entity pyramid;  // refraction (cubic)
        Entity capsule;  // refraction (spherical)

        asset_ref<Texture> irradiance_map;
        asset_ref<Texture> prefiltered_map;
        asset_ref<Texture> BRDF_LUT;

        void PrecomputeIBL();
        void SetupMaterial(Material& mat);

        Entity& GetEntity(int eid);
        void UpdateEntities();
        void UpdateUBOs();
    };

}
