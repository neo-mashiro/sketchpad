#pragma once

#include "scene/scene.h"

namespace scene {

    /* this scene demos these new features:

       > static image-based lighting (IBL) (split sum approximation, distant light probes)
       > static planar reflections
       > 
       > 
       > 
       > 
       > 
       > 
    */

    class Scene02 : public Scene {

        using Scene::Scene;

        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;

        Entity camera;
        Entity skybox;
        Entity sphere[10];
        Entity cube[3];
        Entity torus;
        Entity teapot;
        Entity point_light;

        asset_ref<Texture> irradiance_map;
        asset_ref<Texture> prefiltered_map;
        asset_ref<Texture> BRDF_LUT;

        void PrecomputeIBL();
        void UpdateEntities();
        void UpdateUBOs();
    };

}
