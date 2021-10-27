#include "pch.h"
#include "utils/filesystem.h"

namespace utils::paths {

    std::filesystem::path vs2019;
    std::filesystem::path solution;

    std::string root, source, resource;
    std::string fonts, models, shaders, textures, cubemaps;

    void LoadPathTree() {
        // visual studio's current working directory (.../vs2019)
        vs2019 = std::filesystem::current_path();
        solution = vs2019.parent_path();

        if (!std::filesystem::exists(solution) || !std::filesystem::is_directory(solution)) {
            std::cout << "Solution directory does not exist!" << std::endl;
            std::cin.get();  // pause the console before exiting
            exit(EXIT_FAILURE);
        }

        if (std::filesystem::is_empty(solution)) {
            std::cout << "Solution directory is empty!" << std::endl;
            std::cin.get();  // pause the console before exiting
            exit(EXIT_FAILURE);
        }

        auto src_path = solution / "src";
        auto res_path = solution / "res";

        root     = solution.string() + "\\";
        source   = src_path.string() + "\\";
        resource = res_path.string() + "\\";

        fonts    = (res_path / "font"   ).string() + "\\";
        models   = (res_path / "model"  ).string() + "\\";
        shaders  = (res_path / "shader" ).string() + "\\";
        textures = (res_path / "texture").string() + "\\";
        cubemaps = (res_path / "cubemap").string() + "\\";
    }

}