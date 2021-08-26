#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <vector>
#include <ECS/entt.hpp>

namespace scene {

    // forward declaration
    class Scene;
    class UBO;
    class FBO;

    class Renderer {
      private:
        static Scene* last_scene;
        static Scene* curr_scene;

        static std::map<GLuint, UBO> UBOs;  // stores a uniform buffer at each binding point
        static std::vector<entt::entity> render_list;

      public:
        // configuration functions
        static void DepthTest(bool on);
        static void StencilTest(bool on);
        static void FaceCulling(bool on);
        static void SetFrontFace(bool ccw);

        // core event functions
        static void Init();
        static void Free();

        static void Attach(const std::string& title);
        static void Detach();

        static void Clear();
        static void Flush();

        static void DrawScene();
        static void DrawImGui();

        static void Submit(std::initializer_list<entt::entity> entities);
    };

}
