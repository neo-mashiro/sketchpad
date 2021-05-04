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
// https://github.com/daywee/ZpgProject/blob/3c2885ea89792c218caa41be302ddcb503298d66/ZpgProject/TextureLoader.cpp
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

    void LoadTexture() const {
        stbi_set_flip_vertically_on_load(false);
        int width, height, n_channels;
        unsigned char* buffer = nullptr;

        // 2D texture: load a single image
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

        // 3D textures: load images of the 6 faces
        else if (target == GL_TEXTURE_3D || target == GL_TEXTURE_CUBE_MAP) {
            for (const auto& face : cubemap) {
                std::string filepath = path + face.second;
                buffer = stbi_load(filepath.c_str(), &width, &height, &n_channels, STBI_rgb_alpha);

                if (!buffer) {
                    std::cerr << "Failed to load texture: " << filepath << std::endl;
                    stbi_image_free(buffer);
                    return;
                }
                else {
                    glTexImage2D(face.first, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
                }

                stbi_image_free(buffer);
            }
        }
    }

  public:
    GLuint id;
    GLenum target;     // GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP
    std::string type;  // ambient, diffuse, specular, emission, normal, height, bump, metallic, roughness, opacity
    std::string path;  // for 1D and 2D texture, this is the path of the image file
                       // for 3D and cubemap, this is the directory that contains images of the 6 faces

    Texture(GLenum target, const std::string& type, const std::string& path)
        : target(target), type(type), path(path) {

        glGenTextures(1, &id);
        glBindTexture(target, id);

        // load texture
        LoadTexture();

        // set parameters
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // bilinear filtering
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~Texture() {

    }

};
