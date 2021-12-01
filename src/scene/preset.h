#pragma once

#include <glm/glm.hpp>

namespace scene {

    namespace world {
        // world space constants (OpenGL adopts a right-handed coordinate system)
        constexpr glm::vec3 origin  { 0.0f };
        constexpr glm::vec3 zero    { 0.0f };
        constexpr glm::vec3 unit    { 1.0f };
        constexpr glm::mat4 eye     { 1.0f };
        constexpr glm::vec3 up      { 0.0f, 1.0f, 0.0f };
        constexpr glm::vec3 forward { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 right   {-1.0f, 0.0f, 0.0f };
    }

    namespace color {
        // some commonly used color presets
        constexpr glm::vec3 white  { 1.0f };
        constexpr glm::vec3 black  { 0.0f };
        constexpr glm::vec3 red    { 1.0f, 0.0f, 0.0f };
        constexpr glm::vec3 green  { 0.0f, 1.0f, 0.0f };
        constexpr glm::vec3 lime   { 0.5f, 1.0f, 0.0f };
        constexpr glm::vec3 blue   { 0.0f, 0.0f, 1.0f };
        constexpr glm::vec3 cyan   { 0.0f, 1.0f, 1.0f };
        constexpr glm::vec3 yellow { 1.0f, 1.0f, 0.0f };
        constexpr glm::vec3 purple { 0.5f, 0.0f, 1.0f };
    }

}