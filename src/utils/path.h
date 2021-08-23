#pragma once

#define CWF std::string(__FILE__)
#define CWD CWF.substr(0, CWF.rfind("\\")) + "\\"
#define SRC CWF.substr(0, CWF.rfind("\\src")) + "\\src\\"
#define RES CWF.substr(0, CWF.rfind("\\src")) + "\\res\\"

#define SHADER  RES + "shader\\"
#define SKYBOX  RES + "skybox\\"
#define TEXTURE RES + "texture\\"