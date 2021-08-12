#pragma once

#define CWF std::string(__FILE__)
#define CWD CWF.substr(0, CWF.rfind("\\")) + "\\"
#define SRC CWF.substr(0, CWF.rfind("\\src")) + "\\src\\"
#define RES CWF.substr(0, CWF.rfind("\\src")) + "\\res\\"

#define GLSL    SRC + "GLSL\\"
#define IMAGE   RES + "image\\"
#define SKYBOX  RES + "skybox\\"
#define TEXTURE RES + "texture\\"

#include "components/camera.h"
#include "components/light.h"
#include "components/mesh.h"
#include "components/model.h"
#include "components/shader.h"
#include "components/texture.h"
#include "components/transform.h"