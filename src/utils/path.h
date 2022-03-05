#pragma once

#include <filesystem>
#include <string>

namespace utils::paths {

    extern std::filesystem::path solution;

    extern std::string root;
    extern std::string source;
    extern std::string resource;

    extern std::string font;
    extern std::string model;
    extern std::string screenshot;
    extern std::string shader;
    extern std::string texture;

    void SearchPaths();

}