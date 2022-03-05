--------------------------------------------------------------------------------
-- [ GLOBAL VARIABLES ]
--------------------------------------------------------------------------------
-- Lua variables are global by default unless the local keyword is specified,
-- whenever possible, use local to declare variables to avoid polluting the
-- global namespace and causing bugs. Names starting with an "_" followed by
-- uppercase letters (_ACTION, _VERSION, _G) are reserved for internal global
-- variables used by Lua. Global variables are not only global in this file,
-- but also visible to other scripts in the same environment. Lua internally
-- maintains a `_G` table that stores all global variables, we can query them
-- via `_G[key]`, but using `require()` is a generally better option.
--------------------------------------------------------------------------------
WIN_IDE = "none"

ROOT_DIR = "./"
SOURCE_DIR = "./src/"
SHADER_DIR = "./res/shader/"
VENDOR_DIR = "./vendor/"

OBJECT_DIR = "./build/intermediates/"
TARGET_DIR = "./build/bin/"
OUTPUT_DIR = "%{cfg.buildcfg}/%{cfg.platform}"

CONFIG = {}          -- configuration table
CONFIG.files = {}    -- store the file names of all scene scripts
CONFIG.title = {}    -- store the titles of all scenes
CONFIG.class = {}    -- store the class names of all scene classes

USE_LEGACY_BACKEND = false;

--------------------------------------------------------------------------------
-- register premake override for adding solution scope items.
-- [source] https://github.com/premake/premake-core/issues/1061
--------------------------------------------------------------------------------
require('vstudio')
premake.api.register {
    name = "solution_items",
    scope = "workspace",
    kind = "list:string",
}

premake.override(
    premake.vstudio.sln2005, "projects",
    function(base, wks)
        if wks.solution_items and #wks.solution_items > 0 then
            premake.push(
                "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"Solution Items\", "
                .. "\"Solution Items\", \"{" .. os.uuid("Solution Items:" .. wks.name) .. '}"'
            )
            premake.push("ProjectSection(SolutionItems) = preProject")

            for _, file in ipairs(wks.solution_items) do
                file = path.rebase(file, ".", wks.location)
                premake.w(file .. " = " .. file)
            end

            premake.pop("EndProjectSection")
            premake.pop("EndProject")
        end

        base(wks)
    end
)

--------------------------------------------------------------------------------
-- [ WORKSPACE / SOLUTION CONFIG ]
--------------------------------------------------------------------------------
-- functions marked as "local" are only meant to be called inside this script.
-- functions not marked as "local" have global scope, which can be called from
-- other build scripts, in that case, be cautious of the working directory as
-- it depends on the caller's relative path. However, pre/post commands are
-- global, they always use the configuration root folder (where .vcxproj files
-- are actually stored) as working directory, regardless of where the function
-- is being called from. A pre build/link command will be executed before an
-- actual build in visual studio, while a post build/link command will only be
-- executed after a successful build, but both of them are executed afterwards
-- by the visual studio console, not here in premake.
--------------------------------------------------------------------------------
local function setup_solution()
    workspace "sketchpad"
        configurations { "Debug", "Release" }
        platforms      { "x64", "Win32" }

        -- _ACTION is the command line argument following premake5 (e.g. vs2019)
        if _ACTION ~= nil then
            WIN_IDE = _ACTION
        else
            error("Please specify an action! (e.g. vs2019)")
        end

        -- where to place the solution files (.sln)
        location(ROOT_DIR)  -- place under root

        -- add these configuration files to the solution scope
        solution_items { ROOT_DIR .. ".editorconfig" }

        ------------------------------------------------------------------------
        -- [ COMPILER / LINKER CONFIG ]
        ------------------------------------------------------------------------
        flags "FatalLinkWarnings"  -- treat linker warnings as errors
        warnings "Extra"  -- set the number of warnings to maximum level

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
        filter { "system:windows", "action:vs*" }  -- vs2017 ~ vs2019 build on Windows

            systemversion "latest"  -- use the latest version of the SDK available
            flags "MultiProcessorCompile"

            buildoptions {
                "/std:c++17",         -- would update to "/std:c++latest" in the future
                "/utf-8",             -- specify both the source and execution character set as utf-8
                "/diagnostics:caret"  -- place a caret (^) under the line where the issue was detected
            }

            disablewarnings {
                "4018",  -- signed/unsigned mismatch
                "4267",  -- conversion from 'a' to 'b', possible loss of data
                "4456",  -- declaration of '...' hides previous local declaration
                "4458",  -- declaration of '...' hides class member
                "4459",  -- declaration of '...' hides global declaration (glm and spdlog)
                "4505",  -- unreferenced local function has been removed
                "4100",  -- unreferenced formal parameter
                "26495"  -- always initialize a member variable
            }

        filter { "configurations:Debug" }

            -- in debug builds we will always get a 4098 linker warning, because we are using precompiled
            -- binaries for some dependencies like GLFW, which are built in release mode so there will be
            -- a conflict in Visual C++. We can safely ignore this warning in a debug build.

            -- if we choose to manually build GLFW from sources (the preferred way for a game engine) we
            -- should then prepare two builds for it (and for every other library as well), one for debug
            -- mode and one for release mode, but we won't bother doing so.

            linkoptions {
                "/ignore:4099",  -- ignore library pdb warnings when running in debug
                "/ignore:4217",  -- symbol '___glxxx' defined in 'glxx.lib' is imported by 'ImGui.lib'
                "/verbose:lib",  -- list all the libraries being searched
                "/NODEFAULTLIB:msvcrt.lib"   -- resolve precompiled binaries link conflict
            }

        filter { "configurations:Release" }

            linkoptions {
                "/ignore:4099",  -- ignore library pdb warnings when running in debug
                "/ignore:4217"   -- symbol '___glxxx' defined in 'glxx.lib' is imported by 'ImGui.lib'
            }

        filter {}  -- clear filter
end

--------------------------------------------------------------------------------
-- [ LIBRARIES CONFIG ]
--------------------------------------------------------------------------------
-- some vendor libraries are not header-only, and do not have binaries, we have
-- to compile from sources ourselves. Even if binaries are available, sometimes
-- we want to keep the vendor lib up-to-date with its latest release, we still
-- need to compile from source code manually.

-- while it's possible to include vendor's scripts directly into our project,
-- doing so would contaminate our own source tree, making it less obvious to
-- tell which script is external. So instead, we will setup each library as an
-- individual project whose build target is a static `.lib` file, which can
-- then be linked to by our own `.exe` project.
--------------------------------------------------------------------------------
local function setup_vendor_library()
    -- group all external libraries into a virtual folder called "Dependencies"
    group "Dependencies"
        project "ImGui"
            kind "StaticLib"
            language "C++"
            cppdialect "C++17"

            -- place the project files (.vcxproj) in the configuration folder under root
            location(ROOT_DIR .. WIN_IDE .. "/")

            files {
                VENDOR_DIR .. "imgui/include/imgui/" .. "*.h",
                VENDOR_DIR .. "imgui/include/imgui/" .. "*.cpp"
            }

            vpaths {
                ["Sources/*"] = {
                    VENDOR_DIR .. "imgui/include/imgui/" .. "*.h",
                    VENDOR_DIR .. "imgui/include/imgui/" .. "*.cpp"
                }
            }

            -- dependencies for compiling ImGui backends
            includedirs {
                VENDOR_DIR .. "GLAD/include",
                VENDOR_DIR .. "GLUT/include",
                VENDOR_DIR .. "GLFW/include"
            }

            -- currently we don't support Vulkan and DX11 backends
            removefiles {
                VENDOR_DIR .. "imgui/include/imgui/imgui_impl_vulkan.h",
                VENDOR_DIR .. "imgui/include/imgui/imgui_impl_vulkan.cpp",
                VENDOR_DIR .. "imgui/include/imgui/imgui_impl_dx11.h",
                VENDOR_DIR .. "imgui/include/imgui/imgui_impl_dx11.cpp"
            }

            -- build into the same folder as our application so that it can be linked
            objdir(OBJECT_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
            targetdir(TARGET_DIR .. "%{prj.name}/" .. OUTPUT_DIR)

        project "ImGuizmo"
            kind "StaticLib"
            language "C++"
            cppdialect "C++17"

            location(ROOT_DIR .. WIN_IDE .. "/")

            disablewarnings {
                "4389",  -- '==': signed/unsigned mismatch
                "4245",  -- conversion from '.' to '.', signed/unsigned mismatch
            }

            files {
                VENDOR_DIR .. "ImGuizmo/include/ImGuizmo/" .. "*.h",
                VENDOR_DIR .. "ImGuizmo/include/ImGuizmo/" .. "*.cpp"
            }

            vpaths {
                ["Sources/*"] = {
                    VENDOR_DIR .. "ImGuizmo/include/ImGuizmo/" .. "*.h",
                    VENDOR_DIR .. "ImGuizmo/include/ImGuizmo/" .. "*.cpp"
                }
            }

            includedirs {
                VENDOR_DIR .. "imgui/include/imgui"  -- ImGuizmo depends on ImGui
            }

            objdir(OBJECT_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
            targetdir(TARGET_DIR .. "%{prj.name}/" .. OUTPUT_DIR)

        project "Glad"
            kind "StaticLib"
            language "C++"
            cppdialect "C++17"

            location(ROOT_DIR .. WIN_IDE .. "/")

            files {
                VENDOR_DIR .. "GLAD/include/glad/glad.h",
                VENDOR_DIR .. "GLAD/include/KHR/khrplatform.h",
                VENDOR_DIR .. "GLAD/src/glad.c"
            }

            vpaths {
                ["Sources/*"] = {
                    VENDOR_DIR .. "GLAD/include/" .. "**.h",
                    VENDOR_DIR .. "GLAD/src/" .. "**.c"
                }
            }

            includedirs {
                VENDOR_DIR .. "GLAD/include"
            }

            objdir(OBJECT_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
            targetdir(TARGET_DIR .. "%{prj.name}/" .. OUTPUT_DIR)

    group ""  -- clear group
end

--------------------------------------------------------------------------------
-- [ PROJECT CONFIG ]
--------------------------------------------------------------------------------
local function setup_project()
    project "sketchpad"
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++17"

        -- place the project files (.vcxproj) in the configuration folder under root
        location(ROOT_DIR .. WIN_IDE .. "/")

        -- build all binaries into the build folder
        objdir(OBJECT_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
        targetdir(TARGET_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
        targetname "sketchpad"  -- sketchpad.exe

        -- use main() instead of WinMain() as the application entry point
        entrypoint "mainCRTStartup"

        -- define macros (preprocessor definitions)
        defines {
            -- suppress C runtime secure warnings to use unsafe lib functions like scanf()
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",

            -- memory-leak report
            "_CRTDBG_MAP_ALLOC",

            -- "FREEGLUT_STATIC",  -- required if freeglut is linked as a static lib
            -- "GLEW_STATIC",      -- required if GLEW is linked as a static lib

            -- automatically handled by visual studio between debug/release builds
            -- "DEBUG", "_DEBUG", "NDEBUG",

            -- custom macros for custom uses, you name it ......
            "OPENGL", "VULKAN", "MY_AWESOME_MACRO"
        }

        -- by default we use GLAD as our OpenGL loader, and GLFW for window/context/input
        -- management, but we also support the legacy backend freeglut for study purpose...
        if USE_LEGACY_BACKEND == true then
            defines "__FREEGLUT__"
        end

        -- precompiled headers
        pchheader "pch.h"
        pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        files {
            -- recursive search
            SOURCE_DIR .. "**.h",
            SOURCE_DIR .. "**.hpp",
            SOURCE_DIR .. "**.c",
            SOURCE_DIR .. "**.cpp",
            SOURCE_DIR .. "**.tpp",
            SHADER_DIR .. "**.glsl"
        }

        -- exclude template files from build so that they are not compiled
        filter { "files:**.tpp" }
            flags { "ExcludeFromBuild" }

        -- exclude shaders from build, they are compiled by GLSL on the fly
        filter { "files:**.glsl" }
            flags { "ExcludeFromBuild" }

        filter {}  -- clear filter

        -- set up visual studio virtual folders
        vpaths {
            ["Headers/*"] = {
                SOURCE_DIR .. "**.h",
                SOURCE_DIR .. "**.hpp"
            },

            ["Sources/*"] = {
                SOURCE_DIR .. "**.c",
                SOURCE_DIR .. "**.cc",
                SOURCE_DIR .. "**.cpp"
            },

            ["Shaders/*"] = {
                SHADER_DIR .. "**.glsl"
            },

            ["Templates/*"] = {
                SOURCE_DIR .. "**.ipp",
                SOURCE_DIR .. "**.tpp"
            }
        }

        -- header files include directories
        includedirs {
            SOURCE_DIR,
            VENDOR_DIR .. "GLUT/include",
            VENDOR_DIR .. "GLFW/include",
            VENDOR_DIR .. "GLAD/include",
            VENDOR_DIR .. "imgui/include",
            VENDOR_DIR .. "ImGuizmo/include",
            VENDOR_DIR .. "assimp/include",

            -- header only libraries
            VENDOR_DIR,
            VENDOR_DIR .. "stb",
            VENDOR_DIR .. "date/include",
            VENDOR_DIR .. "spdlog/include",
            VENDOR_DIR .. "GLM/include",
            VENDOR_DIR .. "EnTT/include",
            VENDOR_DIR .. "IconFont"
        }

        ------------------------------------------------------------------------
        -- [ PROJECT DEPENDENCY CONFIG ]
        ------------------------------------------------------------------------
        -- if you want to link a dependency as a static library (executable file size will be
        -- larger), specify the path to the static ".lib" file under `libdirs`, then add the
        -- library file name under `links`.

        -- if you want to link a dependency as a dynamic library whose functions are loaded
        -- on the fly, simply provide the path to the import library (a ".lib" file of much
        -- smaller size), you don't need to add its file name under `links`, but make sure to
        -- copy the ".dll" file to the target build folder using post-build commands.

        -- the import library will be used to automate the process of loading routines and
        -- functionality from the ".dll" file at runtime, it's typically small in size.

        -- specify the paths to the library files
        libdirs {
            VENDOR_DIR .. "GLUT/lib/%{cfg.platform}",         -- import library
            VENDOR_DIR .. "GLFW/lib-vc2019/%{cfg.platform}",  -- static library
            VENDOR_DIR .. "assimp/lib/%{cfg.platform}"        -- import library (assimp 5.0.1)
        }

        -- specify the library file names (needed by the linker at link time)
        links {
            "glu32", "opengl32", "gdi32", "winmm", "user32",  -- Windows 10
            "gdiplus.lib",   -- needed for taking screenshots
            "glfw3.lib",     -- static library
            "ImGui", "ImGuizmo", "Glad"  -- link to our own static libs (project name)
        }

        ------------------------------------------------------------------------
        -- [ PRE/POST BUILD/LINK ACTIONS ]
        ------------------------------------------------------------------------
        postbuildmessage "Copying .dll files to the target folder ..."
        postbuildcommands {
            -- the path here is relative to the configuration folder (vs2015, vs2019, etc.)
            "{COPY} ../vendor/GLUT/bin/%{cfg.platform}/freeglut.dll %{cfg.targetdir}",
            "{COPY} ../vendor/assimp/bin/%{cfg.platform}/assimp-vc142-mt.dll %{cfg.targetdir}"
        }

        filter "configurations:Release"
            -- after a release build, clean up all intermediate object files
            -- after a debug build, we should keep the `.obj` files for debugging
            postbuildmessage "Cleaning up intermediate files ..."
            postbuildcommands {
                "{RMDIR} %{cfg.objdir}"
            }

        filter {}
end

--------------------------------------------------------------------------------
-- [ ENTRY POINT ]
--------------------------------------------------------------------------------
local function main()
    ----------------------------------------------------------------------------
    -- scan all scene scripts and store them into the configuration table
    ----------------------------------------------------------------------------
    local scripts = os.matchfiles(SOURCE_DIR .. "example/**.cpp")  -- recursive match
    table.sort(scripts)  -- sort the array so that scene ids are numbered in order

    for index, script in ipairs(scripts) do
        -- script is relative path: e.g. "src/examples/scene_01.cpp"
        -- index in Lua starts from 1, not 0 ...
        local pos_start = #script - script:reverse():find("/", 1) + 1
        local pos_end = #script - script:reverse():find("%.", 1) + 1
        local file = string.sub(script, pos_start + 1, pos_end - 1)  -- e.g. "scene_01"

        if (os.isfile(script)) then
            printf("Parsing script: \"%s\"", script)
            local source_code = io.readfile(script)
            local title = source_code:match('this%->title%s=%s(.-);')
            local class = source_code:match('%svoid%s(.-)::Init%(%)')

            CONFIG.files[index] = file
            CONFIG.title[index] = title
            CONFIG.class[index] = class
        end
    end

    ----------------------------------------------------------------------------
    -- if there are other `.lua` build files nested in "/src", run them 1 by 1.
    -- these files, if exist, should be used to setup configuration on a more
    -- detailed level, reserved for future extension.
    -- for example, we can have subfolders inside "./src/" that are independent
    -- custom modules, each may contain a `build.lua` file that serves to build
    -- the module into our own static library so we can link to it.
    ----------------------------------------------------------------------------
    local build_files = os.matchfiles("src/**.lua")  -- recursive match

    for _, build_file in ipairs(build_files) do
        if (os.isfile(build_file)) then
            printf("Running build file: \"%s\"", build_file)
            dofile(build_file)
        end
    end

    ----------------------------------------------------------------------------
    -- setup configuration files for our solution, vendor library and project
    ----------------------------------------------------------------------------
    setup_solution()
    setup_vendor_library()
    setup_project()
end

--------------------------------------------------------------------------------
-- Great, all done, let's run it!
--------------------------------------------------------------------------------
main()
