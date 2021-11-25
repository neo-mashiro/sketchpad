#include "pch.h"
#include "utils/filesystem.h"

namespace utils::paths {

    std::filesystem::path solution;

    std::string root, source, resource;
    std::string font, model, screenshot, shader, texture;

    /* current working directory could either be the vs2019 project folder, or the target
       folders that contain the executables, thus it can vary depending on how and where
       you are running it from: open a debug exe? a release exe? a win32 exe? a x64 exe?
       or from within the visual studio editor? what if users move it to the root?

       since we know the current path must be inside the root directory "sketchpad/", we
       can iteratively search the parent path in the directory tree until we hit the root,
       so that our application doesn't rely on the current working directory.
    */

    void SearchPaths() {
        solution = std::filesystem::current_path();

        while (!std::filesystem::exists(solution / "sketchpad.sln")) {
            solution = solution.parent_path();
        }

        if (!std::filesystem::is_directory(solution)) {
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

        font       = (res_path / "font"      ).string() + "\\";
        model      = (res_path / "model"     ).string() + "\\";
        screenshot = (res_path / "screenshot").string() + "\\";
        shader     = (res_path / "shader"    ).string() + "\\";
        texture    = (res_path / "texture"   ).string() + "\\";
    }

}