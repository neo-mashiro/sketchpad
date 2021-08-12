#pragma once

#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>
//#include <glm/glm.hpp>

namespace scene {

    class Renderer {
      public:
        static void EnableDepthTest();
        static void DisableDepthTest();

        static void EnableFaceCulling();
        static void DisableFaceCulling();
        static void SetFrontFace(bool ccw = true);

        static void ClearBuffer();
    };

}
