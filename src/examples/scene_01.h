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
        Entity point_lights[28];

        // forward+ rendering (a.k.a tiled forward rendering)
        GLuint tile_size = 16;
        GLuint nx, ny, n_tiles;

        buffer_ref<SSBO<glm::vec4>> pl_color_ssbo;
        buffer_ref<SSBO<glm::vec4>> pl_position_ssbo;
        buffer_ref<SSBO<GLfloat>>   pl_range_ssbo;
        buffer_ref<SSBO<GLint>>     pl_index_ssbo;

        asset_ref<ComputeShader> light_cull_compute_shader;

    };
}
