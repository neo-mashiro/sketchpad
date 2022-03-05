#pragma once

/* only header files that are external to your project (rarely changed) AND needed
   by most of your source files (often used) should be included in the precompiled
   header, do not blindly include every single external library header file, avoid
   inclusion of headers that are included only in few source files.

   C++ standard library headers typically belong in the precompiled header file.
   we can also include our own headers that are quite stable and barely changed.

   as long as you follow this rule, compilation will be significantly faster at the
   cost of some disk space. The overhead is that some sources will include headers
   that are not needed but are inside this file anyway, so the compiled binaries of
   these translation units may be slightly larger in size. That said, such overhead
   is considered trivial in most cases, unless our project is huge.
*/

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <initializer_list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#ifdef APIENTRY
#undef APIENTRY
#endif

#define GLM_ENABLE_EXPERIMENTAL

#pragma warning(push)
#pragma warning(disable : 4201)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/perpendicular.hpp>
#pragma warning(pop)