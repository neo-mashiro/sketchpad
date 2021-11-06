#pragma once

#include <filesystem>
#include <string>

namespace utils::paths {

    extern std::filesystem::path solution;

    extern std::string root;
    extern std::string source;
    extern std::string resource;

    extern std::string fonts;
    extern std::string models;
    extern std::string shaders;
    extern std::string textures;

    void SearchPaths();

}