#pragma once

#include <string>
#include <vector>
#include "scene/scene.h"

using namespace scene;

namespace factory {

    extern const std::vector<std::string> titles;

    Scene* LoadScene(const std::string& title);

}