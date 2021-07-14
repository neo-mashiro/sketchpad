--------------------------------------------------------------------------------
-- [ GLOBAL VARIABLES ]
--------------------------------------------------------------------------------
-- Lua variables are global by default unless the local keyword is specified,
-- whenever possible, use local to declare variables to avoid polluting the
-- global namespace and causing bugs.

-- names starting with an underscore followed by uppercase letters (_ACTION,
-- _VERSION) are reserved for internal global variables used by Lua.

-- as a best practice, always prefix a global variable with the letter "g".
--------------------------------------------------------------------------------
g_ide_software = "none"

g_root_dir = "./"
g_source_dir = "./src/"
g_vendor_dir = "./vendor/"

g_object_dir = "./build/intermediates/"
g_target_dir = "./build/bin/"
g_output_dir = "%{cfg.buildcfg}/%{cfg.platform}"

--------------------------------------------------------------------------------
-- [ WORKSPACE / SOLUTION CONFIG ]
--------------------------------------------------------------------------------
-- functions marked as "local" are only meant to be called inside this script.
-- functions not marked as "local" have global scope, which can be called from
-- other build scripts, in that case, be cautious of the working directory as
-- it depends on the caller's relative path, however, pre/post commands are
-- global, they always use the configuration root folder (where .vcxproj files
-- are actually stored) as working directory, regardless of where the function
-- is being called from.

-- why do pre/post build/link commands always have global scope? that's due to
-- the delay execution of shell commands, which only occurs after a visual
-- studio build (c++ sources compilation), rather than now (in premake).
--------------------------------------------------------------------------------
local function setup_solution()
    workspace "sketchpad"
        configurations { "Debug", "Release" }
        platforms      { "x64", "Win32" }

        -- _ACTION is the command line argument following premake5 (e.g. vs2019)
        if _ACTION ~= nil then
            g_ide_software = _ACTION
        else
            error("Please specify an action! (e.g. vs2019)")
        end

        -- where to place the solution files (.sln)
        location(g_root_dir)  -- place under root

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
            location(g_root_dir .. g_ide_software .. "/")

            files {
                g_vendor_dir .. "imgui/" .. "*.h",
                g_vendor_dir .. "imgui/" .. "*.cpp"
            }

            vpaths {
                ["Sources/*"] = {
                    g_vendor_dir .. "imgui/" .. "*.h",
                    g_vendor_dir .. "imgui/" .. "*.cpp"
                }
            }

            -- dependencies for compiling ImGui backends
            includedirs {
                g_vendor_dir .. "GLEW/include",
                g_vendor_dir .. "GLUT/include"
            }

            -- build into the same folder as our application so that it can be linked
            objdir(g_object_dir .. "%{prj.name}/" .. g_output_dir)
            targetdir(g_target_dir .. "%{prj.name}/" .. g_output_dir)

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
        location(g_root_dir .. g_ide_software .. "/")

        -- build all binaries into the build folder
        objdir(g_object_dir .. "%{prj.name}/" .. g_output_dir)
        targetdir(g_target_dir .. "%{prj.name}/" .. g_output_dir)
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
            -- "DEBUG", "_DEBUG", "NDEBUG"

            -- custom macros for custom uses, you name it ......
            "CORE_ENABLE_LOG", "LOG_LEVEL=3"  -- just examples, not really used
        }

        -- precompiled headers
        pchheader "pch.h"
        pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        files {
            -- recursive search in "src/"
            g_source_dir .. "**.h",
            g_source_dir .. "**.hpp",
            g_source_dir .. "**.c",
            g_source_dir .. "**.cpp",
            g_source_dir .. "**.tpp",
            g_source_dir .. "**.glsl"
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
                g_source_dir .. "**.h",
                g_source_dir .. "**.hpp"
            },

            ["Sources/*"] = {
                g_source_dir .. "**.c",
                g_source_dir .. "**.cc",
                g_source_dir .. "**.cpp"
            },

            ["Shaders/*"] = {
                g_source_dir .. "**.glsl"
            },

            ["Templates/*"] = {
                g_source_dir .. "**.ipp",
                g_source_dir .. "**.tpp"
            }
        }

        -- header files include directories
        includedirs {
            g_source_dir,
            g_vendor_dir .. "GLEW/include",
            g_vendor_dir .. "GLFW/include",
            g_vendor_dir .. "GLUT/include",
            g_vendor_dir .. "imgui",

            -- header only libraries
            g_vendor_dir,
            g_vendor_dir .. "GLM/include",
            g_vendor_dir .. "spdlog/include"
        }

        ------------------------------------------------------------------------
        -- [ PROJECT DEPENDENCY CONFIG ]
        ------------------------------------------------------------------------
        -- paths for libraries (libs/dlls/etc) that are required when compiling
        libdirs {
            g_vendor_dir .. "GLEW/lib/%{cfg.platform}",         -- GLEW: use the static library
            g_vendor_dir .. "GLFW/lib-vc2019/%{cfg.platform}",  -- GLFW: use the static library

            -- GLUT v3.0.0 MSVC Package: only the dynamic library is available.
            -- --------------------------------------------------------------------------------
            -- to load a dynamic library, first link to the static import library (a `.lib`
            -- file of the same name as the dynamic library `.dll`), then copy the `.dll` file
            -- to the target build folder using post-build command below.
            -- --------------------------------------------------------------------------------
            -- the import library is used to automate the process of loading routines and
            -- functionality from the `.dll` file at runtime, it's typically small in size.
            -- --------------------------------------------------------------------------------
            g_vendor_dir .. "GLUT/lib/%{cfg.platform}"
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
    ----------------------------------------------------------------------------
    -- scan all the scenes to make sure we don't miss anything
    ----------------------------------------------------------------------------
    local dirs = os.matchdirs(g_source_dir .. "scene_*")  -- non-recursive match

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

    ----------------------------------------------------------------------------
    -- if there are other `.lua` build files nested in "/src", run them 1 by 1.
    -- these files, if present, should be used to setup configuration on a more
    -- detailed level, reserved for future extension.
    -- for example, later we may have a "./src/utils/" folder that groups all
    -- the utility scripts, and contains a `build.lua` file to build them into
    -- our own static library that we can link to.
    ----------------------------------------------------------------------------
    local build_files = os.matchfiles("src/**.lua")  -- recursive match

    for _, build_file in ipairs(build_files) do
        if (os.isfile(build_file)) then
            printf("Running build file: %s", build_file)
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
