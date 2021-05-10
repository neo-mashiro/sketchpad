--------------------------------------------------------------------------------
-- [ GLOBAL CONFIG ]
--------------------------------------------------------------------------------
root_dir = "./"
source_dir = "../../src/"     -- relative to the project path
vendor_dir = "../../vendor/"  -- relative to the project path
project_name = ""

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
                "/NODEFAULTLIB:libcmt.lib"  -- fix LNK4098 warnings
            }

        filter {}  -- clear filter
end

--------------------------------------------------------------------------------
-- [ PROJECT CONFIG ]
--------------------------------------------------------------------------------
-- this function is in global scope because it is called by each project's `build.lua` script.
-- inside this function, the working directory will be the project's folder, so be aware of relative paths.
-- [ CAVEAT: EXCEPTION ] any pre/post command must still use the root dir as the working directory.

function setup_project()
    project(project_name)
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"

        -- project's absolute path (path of the script that calls this function)
        local abs_project_path = os.getcwd()

        -- place the project files (.vcxproj) in the action folder under root dir
        location("../../" .. premake_action .. "/")

        -- this is the final application to ship, build the executable to app dir
        if (project_name == "sketchpad") then
            objdir("../../build/intermediates/" .. project_name .. "/%{cfg.buildcfg}/%{cfg.platform}")
            targetdir("../../app/")
            targetname "z-sketchpad"  -- application name = z-sketchpad.exe

        -- build all other sandbox chapters into the build folder
        else
            objdir("../../build/intermediates/" .. project_name .. "/%{cfg.buildcfg}/%{cfg.platform}")
            targetdir("../../build/bin/" .. project_name .. "/%{cfg.buildcfg}/%{cfg.platform}")
            targetname "Sandbox"
        end

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

            -- custom macros for various uses ......
            "CANVAS_LOG=1", "CANVAS_CORE=1"
        }

        -- precompiled headers
        -- pchheader "pch.h"
        -- pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        files {
            -- non-recursive search (solution level global source files, search depth = 1)
            source_dir .. "*.h",
            source_dir .. "*.hpp",
            source_dir .. "*.c",
            source_dir .. "*.cpp",
            source_dir .. "*.tpp",

            -- recursive search (all source files for the current project)
            abs_project_path .. "/**.h",
            abs_project_path .. "/**.hpp",
            abs_project_path .. "/**.c",
            abs_project_path .. "/**.cpp",
            abs_project_path .. "/**.tpp",
            abs_project_path .. "/**.glsl"
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
                source_dir .. "*.h",
                source_dir .. "*.hpp",
                abs_project_path .. "/**.h",
                abs_project_path .. "/**.hpp"
            },

            ["Sources/*"] = {
                source_dir .. "*.c",
                source_dir .. "*.cc",
                source_dir .. "*.cpp",
                abs_project_path .. "/**.c",
                abs_project_path .. "/**.cc",
                abs_project_path .. "/**.cpp"
            },

            ["Shaders/*"] = {
                abs_project_path .. "/**.glsl"
            },

            ["Templates/*"] = {
                source_dir .. "*.ipp",
                source_dir .. "*.tpp",
                abs_project_path .. "/**.tpp"
            }
        }

        -- header files include directories
        includedirs {
            "../../src",       -- solution level
            abs_project_path,  -- per project level
            vendor_dir .. "GLEW/include",
            vendor_dir .. "GLFW/include",
            vendor_dir .. "GLUT/include",

            -- header only libraries
            "../../vendor",
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
            "glu32", "opengl32", "gdi32", "winmm", "user32"  -- Windows 10
        }

        ------------------------------------------------------------------------
        -- [ POST BUILD ACTIONS ]
        ------------------------------------------------------------------------
        -- [ CAVEAT ] shell commands are executed later in a real build inside Visual
        -- Studio, NOT here in premake, as a result, any pre/post command must use
        -- the `action` folder (where .vcxproj file is stored) as the working directory.

        postbuildmessage "Copying dynamic DLL file to the target path ..."
        postbuildcommands {
            "{COPY} ../vendor/GLUT/bin/%{cfg.platform}/freeglut.dll %{cfg.targetdir}"
        }

        -- keep the intermediate files after a build, we still need the `main.obj` file to debug
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

    local dirs = os.matchdirs("src/*")

    for index, folder in ipairs(dirs) do
        -- folder is relative path: e.g. "src/scene_0"
        local build_file = string.format("%s/build.lua", folder)

        if (os.isfile(build_file)) then
            -- index in Lua starts from 1, not 0 ...
            project_name = string.sub(folder, 5)
            printf("Setting up project %s", project_name)
            dofile(build_file)
        end
    end
end

main()
