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
VENDOR_DIR = "./vendor/"

OBJECT_DIR = "./build/intermediates/"
TARGET_DIR = "./build/bin/"
OUTPUT_DIR = "%{cfg.buildcfg}/%{cfg.platform}"

CONFIG = {}          -- configuration table
CONFIG.files = {}    -- store the file names of all scene scripts
CONFIG.title = {}    -- store the titles of all scenes
CONFIG.class = {}    -- store the class names of all scene classes

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
        filter { "system:windows", "action:vs*"}  -- vs2015 ~ vs2019 build on Windows

            systemversion "latest" -- use the latest version of the SDK available
            flags "MultiProcessorCompile"
            -- buildoptions { "-pedantic", "-std=c++17" }

            linkoptions {
                "/ignore:4099",  -- ignore library pdb warnings when running in debug
                "/ignore:4217",  -- symbol '___glewxxx' defined in 'glew32s.lib' is imported by 'ImGui.lib'
                "/NODEFAULTLIB:libcmt.lib"  -- fix LNK4098 warnings
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
        -- a graphical user interface library for c++
        project "ImGui"
            kind "StaticLib"
            language "C++"
            cppdialect "C++17"

            -- place the project files (.vcxproj) in the configuration folder under root
            location(ROOT_DIR .. WIN_IDE .. "/")

            files {
                VENDOR_DIR .. "imgui/" .. "*.h",
                VENDOR_DIR .. "imgui/" .. "*.cpp"
            }

            vpaths {
                ["Sources/*"] = {
                    VENDOR_DIR .. "imgui/" .. "*.h",
                    VENDOR_DIR .. "imgui/" .. "*.cpp"
                }
            }

            -- dependencies for compiling ImGui backends
            includedirs {
                VENDOR_DIR .. "GLEW/include",
                VENDOR_DIR .. "GLUT/include"
            }

            -- build into the same folder as our application so that it can be linked
            objdir(OBJECT_DIR .. "%{prj.name}/" .. OUTPUT_DIR)
            targetdir(TARGET_DIR .. "%{prj.name}/" .. OUTPUT_DIR)

        -- include more vendor libraries that you want to compile from source code
        --[[
        project "tinyxml2"
            kind "StaticLib"
            language "C++"
            cppdialect "C++17"
            ...
        --]]

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
            -- "FREEGLUT_STATIC",  -- required only when freeglut is linked as a static lib
            "GLEW_STATIC",

            -- suppress C runtime secure warnings to use unsafe lib functions like scanf()
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",

            -- TinyXML parser (read XML and create C++ objects, or vice versa)
            "TIXML_USE_STL",

            -- automatically handled by visual studio between debug/release builds
            -- "DEBUG", "_DEBUG", "NDEBUG",

            -- custom macros for custom uses, you name it ......
            "CORE_ENABLE_LOG", "LOG_TRACE",
            -- "MULTI_THREAD"  -- if defined, consider using DirectX instead
        }

        -- precompiled headers
        pchheader "pch.h"
        pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        files {
            -- recursive search in "src/"
            SOURCE_DIR .. "**.h",
            SOURCE_DIR .. "**.hpp",
            SOURCE_DIR .. "**.c",
            SOURCE_DIR .. "**.cpp",
            SOURCE_DIR .. "**.tpp",
            SOURCE_DIR .. "**.glsl"
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
                SOURCE_DIR .. "**.glsl"
            },

            ["Templates/*"] = {
                SOURCE_DIR .. "**.ipp",
                SOURCE_DIR .. "**.tpp"
            }
        }

        -- header files include directories
        includedirs {
            SOURCE_DIR,
            VENDOR_DIR .. "GLEW/include",
            VENDOR_DIR .. "GLFW/include",
            VENDOR_DIR .. "GLUT/include",
            VENDOR_DIR .. "imgui",

            -- header only libraries
            VENDOR_DIR,
            VENDOR_DIR .. "GLM/include",
            VENDOR_DIR .. "spdlog/include"
        }

        ------------------------------------------------------------------------
        -- [ PROJECT DEPENDENCY CONFIG ]
        ------------------------------------------------------------------------
        -- paths for libraries (libs/dlls/etc) that are required when compiling
        libdirs {
            VENDOR_DIR .. "GLEW/lib/%{cfg.platform}",         -- GLEW: use the static library
            VENDOR_DIR .. "GLFW/lib-vc2019/%{cfg.platform}",  -- GLFW: use the static library

            -- GLUT v3.0.0 MSVC Package: only the dynamic library is available.
            -- --------------------------------------------------------------------------------
            -- to load a dynamic library, first link to the static import library (a `.lib`
            -- file of the same name as the dynamic library `.dll`), then copy the `.dll` file
            -- to the target build folder using post-build commands.
            -- --------------------------------------------------------------------------------
            -- the import library is used to automate the process of loading routines and
            -- functionality from the `.dll` file at runtime, it's typically small in size.
            -- --------------------------------------------------------------------------------
            VENDOR_DIR .. "GLUT/lib/%{cfg.platform}"
        }

        -- specific library files (.lib .dll) to include
        links {
            "glew32s.lib",  -- glew32s is the static version, glew32 is the import library for .dll
            "glfw3.lib",  -- you can easily tell the static version from file size (the larger one)
            "glu32", "opengl32", "gdi32", "winmm", "user32",  -- Windows 10

            "ImGui"  -- link to our own static library build
        }

        ------------------------------------------------------------------------
        -- [ PRE/POST BUILD/LINK ACTIONS ]
        ------------------------------------------------------------------------
        postbuildmessage "Copying .dll files to the target folder ..."
        postbuildcommands {
            -- the path here is relative to the configuration folder (vs2015, vs2019, etc.)
            "{COPY} ../vendor/GLUT/bin/%{cfg.platform}/freeglut.dll %{cfg.targetdir}"
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
    -- scan all scene files and store them into the configuration table
    ----------------------------------------------------------------------------
    local dirs = os.matchdirs(SOURCE_DIR .. "scene_*")  -- non-recursive match
    table.sort(dirs)  -- sort the array so that scene ids are numbered in order

    for index, folder in ipairs(dirs) do
        -- folder is relative path: e.g. "src/scene_01"
        -- index in Lua starts from 1, not 0 ...
        local pos = #folder - folder:reverse():find("/", 1) + 1  -- position of the last slash "/"
        local file = string.sub(folder, pos + 1)
        local path = string.format("%s/%s.cpp", folder, file)

        if (os.isfile(path)) then
            printf("Attaching scene: \"%s\"", path)
            local source_code = io.readfile(path)
            local title = source_code:match('Window::Rename%((.-)%)')
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
