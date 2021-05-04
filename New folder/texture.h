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
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// https://github.com/SpartanJ/SOIL2
#include "SOIL2.h"  // replace stb_image

// 开始做这个texture！！！
////////////////////////////////////////////////////////////////////////////////
// 做完texture了，不急着做model class，先把smooth camera跑通，并且加上一个plane地板的mesh和texture，并调试编译通过所有9个小项目（合并成几个大的scene）
// 顺便把所有的log信息都替换成spdlog，不用cout cerr和printf。
////////////////////////////////////////////////////////////////////////////////
// https://github.com/if1live/real-stereo-vision-oculus-rift/blob/2dadd318bdbc1e9c9490792225df048ab37993c2/haruna/gl/texture.h
// https://github.com/if1live/real-stereo-vision-oculus-rift/blob/2dadd318bdbc1e9c9490792225df048ab37993c2/haruna/gl/texture.cpp

// 全部跑通了以后，把GLSL整本书过一遍。官方4.5版本的，高亮做上笔记。别看网上其他的参考教程，都是很老的。

// struct Texture {
//     GLuint id;
//     std::string type;
//     std::string path;
// };

class Texture {
  private:
    const static std::unordered_map<GLenum, std::string> cubemap {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, "posx.png" },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "posy.png" },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "posz.png" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "negx.png" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "negy.png" },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "negz.png" }
    };

    void LoadTexture(bool use_soil = false) const {
        stbi_set_flip_vertically_on_load(false);
        int width, height, n_channels;
        unsigned char* buffer = nullptr;

        if (target == GL_TEXTURE_2D) {
            buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);
            if (!buffer) {
                std::cerr << "Failed to load texture: " << path << std::endl;
                stbi_image_free(buffer);
                return;
            }

            GLint format = (n_channels == 4 ? GL_RGBA : GL_RGB);
            glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer);

            glGenerateMipmap(target);
            stbi_image_free(buffer);
        }
        else if (target == GL_TEXTURE_3D) {
            // solid textures, volume simulations, to be implemented...
        }
        else if (target == GL_TEXTURE_CUBE_MAP) {
            // skylight illumination, dynamic reflection, to be implemented...
        }
    }

    void LoadSkybox(bool use_soil = false) const {
        stbi_set_flip_vertically_on_load(false);
        int width, height, n_channels;
        unsigned char* buffer = nullptr;

        for (const auto& face : cubemap) {
            std::string filepath = path + face.second;
            buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

            if (!buffer) {
                std::cerr << "Failed to load skybox texture: " << filepath << std::endl;
                stbi_image_free(buffer);
                return;
            }
            else {
                glTexImage2D(face.first, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            }

            stbi_image_free(buffer);
        }
    }

    void SetWrapMode() const {
        if (target == GL_TEXTURE_2D) {
            // repeat the texture image (recommend to use seamless textures)
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else {
            // repeat the last pixels when s/t/r coordinates fall off the edge
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
    }

    // -------------------------------------------------------------------------------------------
    // texture sampling modes: (from cheap to expensive, from worst to best visual quality)
    // -------------------------------------------------------------------------------------------
    // 1. point filtering produces a blocked pattern (individual pixels more visible)
    // 2. bilinear filtering produces a smoother pattern (texel colors are sampled from neighbors)
    // 3. trilinear filtering linearly interpolates between two bilinearly sampled mipmaps
    // 4. anisotropic filtering samples the texture as a non-square shape to correct blurriness
    // -------------------------------------------------------------------------------------------
    void SetFilterMode(bool anisotropic) const {
        if (target == GL_TEXTURE_2D) {
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // trilinear filtering

            // anisotropic filtering requires OpenGL core 4.6 or EXT_texture_filter_anisotropic
            if (anisotropic) {
                GLfloat param = 1.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &param);

                param = std::clamp(param, 1.0f, 8.0f);  // implementation-defined maximum anisotropy
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, param);
            }
        }
        else if (target == GL_TEXTURE_3D) {
            // solid textures, volume simulations, to be determined...
        }
        else if (!type.compare("skybox")) {
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // skyboxes do not minify, no mipmaps
        }
        else if (target == GL_TEXTURE_CUBE_MAP) {
            // skylight illumination, dynamic reflection, to be determined...
        }
    }

  public:
    GLuint id;
    GLenum target;     // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP

    std::string type;  // albedo, normal, bump (height), displacement, metallic, gloss (roughness), opacity
                       // ambient, diffuse, specular, emission, skybox (3D), skylight illumination (3D)
    std::string path;  // for 1D and 2D texture, this is the path of the image file
                       // for 3D and cubemap, this is the directory that contains images of the 6 faces

    Texture(GLenum target, const std::string& type, const std::string& path, bool anisotropic = false)
        : target(target), type(type), path(path) {

        glGenTextures(1, &id);
        glBindTexture(target, id);

        if (target == GL_TEXTURE_CUBE_MAP && !type.compare("skybox")) {
            LoadSkybox();
        }
        else {
            LoadTexture();
        }

        SetWrapMode();
        SetFilterMode(anisotropic);

        glBindTexture(target, 0);
    }

    ~Texture() {
        glBindTexture(target, 0);
        glDeleteTextures(1, id);
    }
};
