#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "shader.h"
#include "texture.h"

constexpr auto PI = 3.14159265358979323846f;
constexpr auto MAX_TEXTURE_UNITS = 16;  // reset this to your GPU limit (value is queried in `main()`)

// model class先不做，先把smooth camera这章跑通了再说
// 参考那个pr的代码，优化mesh model提高速度
// texture目前先单独做一个class，后面有问题再改，有了FBO以后，很可能要融入FBO作为一个大class。参考那两份代码，分为loadTexture,loadwithAlpha,loadCubeMap三种
// 单独的texture类负责load自定义的texture，model负责load obj中自带的texture
// texture和mesh是独立的，没有任何耦合，mesh只假定textures已经有现成的了，至于怎么来的它不关心

// 什么是material？应该是struct Material { Texture albedo; Texture diffuse; Texture specular; Texture metallic; }这样的
