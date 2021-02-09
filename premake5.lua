--------------------------------------------------------------------------------
-- [ GLOBAL CONFIG ]
--------------------------------------------------------------------------------
local vendor_dir = "./vendor/"  -- local var is enough for only one project

--------------------------------------------------------------------------------
-- [ WORKSPACE / SOLUTION CONFIG ]
--------------------------------------------------------------------------------
workspace "OpenGL-Canvas"
    configurations { "Debug", "Release" }
    platforms      { "x64", "Win32" }

    -- all variables are global by default unless using the local keyword
    local action = "none"
    -- _ACTION is the command line argument following premake5
    if _ACTION ~= nil then
        action = _ACTION
    end

    -- where to place the project files (.sln .vcxproj)
    location "./"

    ----------------------------------------------------------------------------
    -- [ COMPILER / LINKER CONFIG ]
    ----------------------------------------------------------------------------
    flags "FatalLinkWarnings"  -- treat linker warnings as errors
    warnings "Extra"           -- set the number of warnings that are shown by the compiler to maximum level

    filter { "platforms:*32" }
        architecture "x86"

    filter { "platforms:*64" }
        architecture "x64"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        runtime "Release"

    filter {}  -- clear filter

    ----------------------------------------------------------------------------
    -- [ BUILD CONFIG ]
    ----------------------------------------------------------------------------
    filter { "system:windows", "action:vs*"}  -- vs2015 ~ vs2019 build on Windows
        systemversion "latest" -- use the latest version of the SDK available
        flags { "MultiProcessorCompile" }
        -- buildoptions { "-std=c++17" }

        linkoptions {
            "/ignore:4099",  -- ignore library pdb warnings when running in debug
            "/NODEFAULTLIB:libcmt.lib"  -- fix LNK4098 warnings
        }

    filter {}  -- clear filter

    ----------------------------------------------------------------------------
    -- [ PROJECT CONFIG ]
    ----------------------------------------------------------------------------
    project "OpenGL-Canvas"
        kind "WindowedApp"
        language "C++"
        cppdialect "C++17"

        objdir "build/intermediates/%{cfg.buildcfg}/%{cfg.platform}"
        targetdir "build/bin/%{cfg.buildcfg}/%{cfg.platform}"
        targetname "Canvas"

        -- use main() instead of WinMain() as the application entry point
        entrypoint "mainCRTStartup"

        -- define macros (preprocessor definitions)
        defines {
            "GLEW_STATIC"
        }

        -- precompiled headers
        -- pchheader "pch.h"
        -- pchsource "src/pch.cpp"

        ------------------------------------------------------------------------
        -- [ PROJECT FILES CONFIG ]
        ------------------------------------------------------------------------
        local source_dir = "./src/";

        files {
            source_dir .. "**.h",
            source_dir .. "**.hpp",
            source_dir .. "**.c",
            source_dir .. "**.cpp",
            source_dir .. "**.tpp"
        }

        -- exclude template files from build so that they are not compiled
        filter { "files:**.tpp" }
            flags { "ExcludeFromBuild" }

        filter {}

        -- set up visual studio virtual folders
        vpaths {
            ["Header Files/*"] = {
                source_dir .. "**.h",
                source_dir .. "**.hpp"
            },

            ["Source Files/*"] = {
                source_dir .. "**.c",
                source_dir .. "**.cc",
                source_dir .. "**.cpp"
            },

            ["Template Files/*"] = {
                source_dir .. "**.ipp",
                source_dir .. "**.tpp"
            }
        }

        -- header files include directories
        includedirs {
            "./src",
            vendor_dir .. "GLEW/include",
            vendor_dir .. "GLFW/include",
            vendor_dir .. "GLUT/include"
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
            "opengl32",
            "glew32s.lib",
            "glfw3.lib"
        }

        ------------------------------------------------------------------------
        -- [ POST BUILD ACTIONS ]
        ------------------------------------------------------------------------
        postbuildmessage "Copying dynamic DLL file to the target path ..."
        postbuildcommands {
            "{COPY} vendor/GLUT/bin/%{cfg.platform}/freeglut.dll %{cfg.targetdir}"
        }
