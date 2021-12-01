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
        //Entity plane;
        Entity ball[3];
        Entity sphere[7];
        //Entity motor;
        //Entity sculpture;

        asset_ref<Texture> environments[2];
        asset_ref<Texture> irradiance_maps[2];
        asset_ref<Texture> prefiltered_env_maps[2];
        asset_ref<Texture> brdf_luts[2];

        //std::unique_ptr<Shader> irradiance_shader;

        // utility shaders that are directly controlled by the user
        //std::unique_ptr<Shader> blur_shader;
        //std::unique_ptr<Shader> blend_shader;

        void PrecomputeIrradianceMap();

        void ChangeEnvironment();

    };
}
