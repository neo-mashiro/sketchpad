#pragma once

#include <string>
#include <GL/glew.h>

namespace scene {

    class FBO {
      private:
        GLuint id { 0 };

      public:
        FBO();
        ~FBO();

    };

}
