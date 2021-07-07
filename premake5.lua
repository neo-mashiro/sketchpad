--------------------------------------------------------------------------------
-- [ GLOBAL CONFIG ]
--------------------------------------------------------------------------------
root_dir = "./"
premake_action = "none"

--------------------------------------------------------------------------------
-- [ WORKSPACE / SOLUTION CONFIG ]
--------------------------------------------------------------------------------
local function setup_solution()
    workspace "sketchpad"
        configurations { "Debug", "Release" }
        platforms      { "x64", "Win32" }

        -- all variables are global by default unless the local keyword is used
        -- _ACTION is the command line argument following premake5 (e.g. vs2019)
        if _ACTION ~= nil then
            premake_action = _ACTION
        else
            error("Please specify an action! (e.g. vs2019)")
        end

        -- where to place the solution files (.sln)
        location(root_dir)

        ------------------------------------------------------------------------
        -- [ COMPILER / LINKER CONFIG ]
        ------------------------------------------------------------------------
        flags "FatalLinkWarnings"  -- treat linker warnings as errors
        warnings "Extra"           -- set the number of warnings that are shown by the compiler to maximum level

        filter { "platforms:*32" }
            architecture "x86"

        filter { "platforms:*64" }
            architecture "x64"

        filter "configurations:Debug"
            defines { "DEBUG", "_DEBUG" }
            symbols "On"
            runtime "Debug"

        filter "configurations:Release"
            defines { "NDEBUG" }
            optimize "On"
            runtime "Release"

        filter {}  -- clear filter

        ------------------------------------------------------------------------
        -- [ BUILD CONFIG ]
        ------------------------------------------------------------------------
        filter { "system:windows", "action:vs*"}  -- vs2015 ~ vs2019 build on Windows
            systemversion "latest" -- use the latest version of the SDK available
            flags { "MultiProcessorCompile" }
            -- buildoptions { "-std=c++17" }

            linkoptions {
                "/ignore:4099",  -- ignore library pdb warnings when running in debug
                "/ignore:4217",  -- symbol '___glewxxx' defined in 'glew32s.lib' is imported by 'ImGui.lib'
                "/NODEFAULTLIB:libcmt.lib"  -- fix LNK4098 warnings
            }

        filter {}  -- clear filter
end

--------------------------------------------------------------------------------
-- [ STATIC LIBRARY CONFIG ]
--------------------------------------------------------------------------------
-- ImGui is not a header-only library, and does not have binaries, we need to
-- compile from sources ourselves. I don't want to mess up my own source tree
-- by including their scripts directly, so we setup a standalone project that
-- would build the ImGui sources into a static lib file, which can then be
-- linked by our `.exe` project.

-- this function has global scope because it's called from outside the script.
-- inside the function, the working directory will be the caller's relative path
-- so please be cautious, however, pre/post commands are always global, so any
-- pre/post build/link command still uses the root dir as working directory.
--------------------------------------------------------------------------------
function load_static_lib()
    project "ImGui"
        kind "StaticLib"
        language "C++"
        cppdialect "C++17"

        local root_dir = "../"
        local imgui_dir = "../vendor/imgui/"

        -- place the project files (.vcxproj) in the action folder under root
        location(root_dir .. premake_action .. "/")

        files {
            imgui_dir .. "*.h",
            imgui_dir .. "*.cpp"
        }

        vpaths {
            ["Sources/*"] = {
                imgui_dir .. "*.h",
                imgui_dir .. "*.cpp"
            }
        }

        -- dependencies for compiling ImGui backends
        includedirs {
            "../vendor/GLEW/include",
            "../vendor/GLUT/include"
        }

        -- build into the same folder as our application so that it can be linked
        objdir("../build/intermediates/" .. "ImGui" .. "/%{cfg.buildcfg}/%{cfg.platform}")
        targetdir("../build/bin/" .. "ImGui" .. "/%{cfg.buildcfg}/%{cfg.platform}")
end

--------------------------------------------------------------------------------
-- [ PROJECT CONFIG ]
--------------------------------------------------------------------------------
-- this function has global scope because it's called from outside the script.
-- inside the function, the working directory will be the caller's relative path
-- so please be cautious, however, pre/post commands are always global, so any
-- pre/post build/link command still uses the root dir as working directory.
--------------------------------------------------------------------------------
function setup_project()
    project "sketchpad"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"

        -- path of the build file that calls this function
        local build_dir = os.getcwd()  -- absolute path

        -- place the project files (.vcxproj) in the action folder under root
        location("../" .. premake_action .. "/")

        -- build all binaries into the build folder
        objdir("../../build/intermediates/" .. "sketchpad" .. "/%{cfg.buildcfg}/%{cfg.platform}")
        targetdir("../../build/bin/" .. "sketchpad" .. "/%{cfg.buildcfg}/%{cfg.platform}")
        targetname "sketchpad"  -- sketchpad.exe

        -- use main() instead of WinMain() as the application entry point
        entrypoint "mainCRTStartup"

        -- define macros (preprocessor definitions)
        defines {
            -- "FREEGLUT_STATIC",  -- required only when freeglut is linked as a static lib
            "GLEW_STATIC",

            -- suppress C runtime secure warnings to use unsafe lib functions like scanf()
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",

            -- TinyXML parser (read XML and create C++ objects, or vice versa)
            "TIXML_USE_STL",

            -- "DEBUG", "_DEBUG",  -- will be automatically handled by Visual Studio

            -- custom macros for custom uses ......
            "CANVAS_LOG=1", "CANVAS_CORE=1"
        }

        -- precompiled headers
        -- pchheader "pch.h"
        -- pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        files {
            -- recursive search in "src/"
            build_dir .. "/**.h",
            build_dir .. "/**.hpp",
            build_dir .. "/**.c",
            build_dir .. "/**.cpp",
            build_dir .. "/**.tpp",
            build_dir .. "/**.glsl"
        }

        -- exclude template files from build so that they are not compiled
        filter { "files:**.tpp" }
            flags { "ExcludeFromBuild" }

        -- exclude shaders from build
        filter { "files:**.glsl" }
            flags { "ExcludeFromBuild" }

        filter {}

        -- set up visual studio virtual folders
        vpaths {
            ["Headers/*"] = {
                build_dir .. "/**.h",
                build_dir .. "/**.hpp"
            },

            ["Sources/*"] = {
                build_dir .. "/**.c",
                build_dir .. "/**.cc",
                build_dir .. "/**.cpp"
            },

            ["Shaders/*"] = {
                build_dir .. "/**.glsl"
            },

            ["Templates/*"] = {
                build_dir .. "*.ipp",
                build_dir .. "*.tpp",
                build_dir .. "/**.tpp"
            }
        }

        local vendor_dir = "../vendor/"

        -- header files include directories
        includedirs {
            build_dir,
            vendor_dir .. "GLEW/include",
            vendor_dir .. "GLFW/include",
            vendor_dir .. "GLUT/include",
            vendor_dir .. "imgui",

            -- header only libraries
            vendor_dir,
            vendor_dir .. "GLM/include",
            vendor_dir .. "spdlog/include"
        }

        ------------------------------------------------------------------------
        -- [ PROJECT DEPENDENCY CONFIG ]
        ------------------------------------------------------------------------
        -- paths for libraries (libs/dlls/etc) that are required when compiling
        libdirs {
            vendor_dir .. "GLEW/lib/%{cfg.platform}",         -- GLEW: use the static library
            vendor_dir .. "GLFW/lib-vc2019/%{cfg.platform}",  -- GLFW: use the static library
            -- GLUT v3.0.0 MSVC Package: only the dynamic library is available
            vendor_dir .. "GLUT/lib/%{cfg.platform}"
        }

        -- specific library files (.lib .dll) to include
        links {
            "glew32s.lib",
            "glfw3.lib",
            "glu32", "opengl32", "gdi32", "winmm", "user32",  -- Windows 10
            "ImGui"  -- link to the static lib project we built
        }

        ------------------------------------------------------------------------
        -- [ PRE/POST BUILD/LINK ACTIONS ]
        ------------------------------------------------------------------------
        -- [ CAUTION ] shell commands are executed later during a Visual Studio
        -- build, not now (in premake), so they always have global scope, all
        -- pre/post commands use the `action` folder (where .vcxproj files are
        -- stored) as the working directory.

        postbuildmessage "Copying `.dll` files to the target path ..."
        postbuildcommands {
            "{COPY} ../vendor/GLUT/bin/%{cfg.platform}/freeglut.dll %{cfg.targetdir}"
        }

        -- keep the `.obj` files after a build, we still need them for debugging
        -- postbuildmessage "Cleaning up intermediate files ..."
        -- postbuildcommands {
        --     "{RMDIR} %{cfg.objdir}"
        -- }
end

--------------------------------------------------------------------------------
-- [ ENTRY POINT ]
--------------------------------------------------------------------------------
local function main()
    setup_solution()
    print("---------------------------------------------------")

    dirs = os.matchdirs("src/*")

    for index, folder in ipairs(dirs) do
        -- folder is relative path: e.g. "src/scene_01"
        -- index in Lua starts from 1, not 0 ...
        local pos = #folder - folder:reverse():find("/", 1) + 1  -- position of the last slash "/"
        local scene = string.sub(folder, pos + 1)
        local scene_file = string.format("%s/%s.cpp", folder, scene)

        if (os.isfile(scene_file)) then
            printf("Attaching scene: %s", scene_file)
        end
    end

    print("---------------------------------------------------")

    build_files = os.matchfiles("src/*.lua")

    for _, build_file in ipairs(build_files) do
        if (os.isfile(build_file)) then
            printf("Running build file: %s", build_file)
            dofile(build_file)
        end
    end

    print("---------------------------------------------------")
end

main()
