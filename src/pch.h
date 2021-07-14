#pragma once

// only header files that are external to your project (rarely changed) AND needed
// by most of your source files (often used) should be included in the precompiled
// header, do not blindly include every single external library header file, avoid
// inclusion of headers that are included only in few source files.

// you can also include your own headers if you know they are hardly ever changed.

// as long as you follow this rule, compilation will be significantly faster, at
// the cost of some disk space. The overhead is that some sources will include
// some headers that are not needed (but are inside #include "pch.h" anyway), so
// the compiled binaries of these translation units will be slightly larger in
// size, but this overhead is trivial in most cases (unless your project is huge).

// STL and Boost headers typically belong in the precompiled header file

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <GL/glew.h>

#pragma warning(push)
#pragma warning(disable : 4505)
#include <GL/freeglut.h>
#pragma warning(pop)

#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl3.h"
