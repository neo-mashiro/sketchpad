#pragma once

#include "scene/scene.h"

namespace scene {

    /* this demo scene aims to help you quickly get started by introducing some of the main
       features of this project. Reference it as a template to create your customized scene.

       # demo features
       
       > entity-component system architecture using the open-source `entt` library
       > scene loading and switching (factory pattern with code generated from Lua)
       > external model loading (FBX) and manual import of local textures
       > vertex array object sharing between meshes
       > bufferless rendering of fullscreen quad on custom framebuffers
       > sharing of assets (shaders and textures) among entities
       > simplified Unity-style material system (one material asset for multiple objects)
       > robust debugging tools (detailed console logs + debug callback + debugging by checkpoints)
       > automatic uniforms and textures binding in the material class
       > smart bindings of shaders and textures, smart uploads of uniforms
       > automatic creation of UBOs from shader (requires the `std140` layout)
       > visualization of linearized depth buffer and stencil buffer
       > all types of shaders included in a single GLSL file (except the compute shader)
       > saving and loading of shader binaries (GLSL only, SPIR-V not supported yet)
       > modularized shader system (support C++ style `#include` directives in GLSL files)
       > physically-based shading using the metallic workflow (Cook-Torrance BRDF)
       > tiled forward rendering (with SSBOs and compute shaders for light culling)
       > loading high resolution skybox from HDR equirectangular maps
       > five popular HDR tone mapping operators with gamma correction
       > bloom effect on selected emissive objects using two-pass Gaussian blur filter
       > simple gizmos drawing with ImGui
       > off-screen multisample anti-aliasing (MSAA)
    */

    class Scene01 : public Scene {

        // inherit the base constructor
        using Scene::Scene;

        // override these functions in the cpp file to render your scene
        void Init(void) override;
        void OnSceneRender(void) override;
        void OnImGuiRender(void) override;

        // the class header is a free space where you can declare any variables you need
        // such as entities, universal shaders, asset refs, buffers and so on, you don't
        // have to use a lot of static global variables in the cpp file

        // list all the entities (game objects) in your scene
        Entity camera;
        Entity skybox;
        Entity sphere, ball, plane;
        Entity runestone;  // external models (.fbx)
        Entity direct_light;
        Entity orbit_light;
        Entity point_lights[28];  // static lights that participate in light culling

        // forward+ rendering parameters (a.k.a tiled forward rendering)
        GLuint tile_size = 16;
        GLuint nx, ny, n_tiles;
        GLuint n_point_lights = 28;

        buffer_ref<SSBO<glm::vec4>> pl_color_ssbo;
        buffer_ref<SSBO<glm::vec4>> pl_position_ssbo;
        buffer_ref<SSBO<GLfloat>>   pl_range_ssbo;
        buffer_ref<SSBO<GLint>>     pl_index_ssbo;

        // utility shaders that are directly controlled by the user
        std::unique_ptr<ComputeShader> light_cull_shader;
        std::unique_ptr<Shader> blur_shader;
        std::unique_ptr<Shader> blend_shader;

        // samplers for the Gaussian blur filter
        std::unique_ptr<Sampler> point_sampler;
        std::unique_ptr<Sampler> bilinear_sampler;

        // feel free to define any helper functions here, or you can break down
        // a complex scene into multiple passes to make your code more readable

        glm::vec3 HSL2RGB(const glm::vec3& hsl);

        void UpdatePLColors();
        void UpdateEntities();
        void UpdateUniforms();
        void UpdateUBOs();

        void DepthPrepass();
        void LightCullPass();
        void BloomFilterPass();
        void MSAAResolvePass();
        void GaussianBlurPass();
        void BloomBlendPass();

    };
}
