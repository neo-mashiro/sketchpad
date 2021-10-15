#pragma once

#include "scene/scene.h"

namespace scene {

    class Scene01 : public Scene {

        // inherit the base constructor
        using Scene::Scene;

        // override these functions in the cpp file to render your scene
        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;

        // the class header is a free space where you can declare any variables you
        // need, such as entities, universal shaders, asset refs, buffers and so on
        // so you don't have to use a lot of static global variables in the cpp file

        // list of entities (game objects)
        Entity camera;
        Entity skybox;
        Entity sphere;
        Entity ball;
        Entity plane;
        Entity runestone;
        Entity direct_light;
        Entity orbit_light;
        Entity point_lights[16];

        // forward+ rendering (a.k.a tiled forward rendering)
        GLuint tile_size = 16;
        GLuint nx, ny, n_tiles;

        SSBO<glm::vec4> light_position_ssbo;
        SSBO<GLfloat> light_range_ssbo;
        SSBO<GLint> light_index_ssbo;

        asset_ref<Shader> depth_prepass_shader;
        asset_ref<ComputeShader> light_cull_compute_shader;


    };
}
