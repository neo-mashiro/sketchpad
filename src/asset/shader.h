/* 
   the shader class is a convenient encapsulation of the linked GLSL program, you can create
   a shader by compiling from a user-provided shader source, or loading a pre-compiled shader
   binary file from local disk.

   # all shaders in one file

   many people like to use a separate file for each type of shader and group them in a folder,
   this is the most common way of handling shaders. However, the downside of this is that we
   would end up having a lot of duplicated code, so beyond a certain point, it becomes really
   hard to manage the mess of shaders in many folders, where you might have to look into each
   file in each folder just to change one thing......

   the way we choose to deal with shaders is to write all shaders in a single ".glsl" file:
   shared code goes on the top of the file, followed by code blocks that are specific to each
   shader stage. The code for each stage is enclosed between a pair of "#ifdef" and "#endif"
   preprocessor guards, so that the compiler can selectively compile shaders of a particular
   stage while filtering others out, and finally link all shaders to make a program.

   as per the GLSL specification, "#version" must be the first line of preprocessor directive
   in the shader code, otherwise it would fallback to the default version 1.1, which is not
   the version we want. Therefore, instead of grabbing the entire buffer all at once, we will
   read code line by line, check if there's a special directive that needs to be resolved.

   # C++ style "#include"

   the "#include" directive is introduced to ease the writing of shaders, it works similar to
   the one that is used in C and C++, which simply copies and pastes the contents of another
   file to the current shader. This feature makes it possible to modulerize GLSL code into a
   bunch of header files that can be installed as needed, which greatly reduces code clutter
   and code duplication.

   due to the lack of build support for GLSL, we don't have the concept of "include directory"
   or a $PATH environment variable to look at when using this feature. As a result, "#include"
   always expects a header file in relative path, which will be concatenated with the path of
   the current shader file to resolve the absolute full path. If the file does not exist or
   cannot open, the line of "#include" will be ignored, and the console will print a warning
   message so that users are likely aware of it.

   > #include "projection.glsl"    // search in current directory
   > #include "./material.glsl"    // search in current directory
   > #include "../utility.glsl"    // search in parent directory
   > #include "..\\../gamma.glsl"  // either separator works fine

   nested "#include" is also supported by recursion so you can include file "A" that includes
   another file "B", but make sure to protect headers by "#ifdef/endif" guards if you want to
   include them multiple times ("#pragma once" is implementation-dependent in GLSL). Also, we
   cannot use "#include" or "ifdef" in any line comment and block comment as they are treated
   as special statements when being parsed, even in the comments!

   # save/load shader binaries

   to build a shader by loading a precompiled binary, that file needs to be already `Save()`
   on local disk. On save, the current shader program will be saved to the source directory
   as a ".bin" file, whose filename is an integer format number that depends on the hardware
   such as 3274 or 1. If the GPU does not support any format number, meaning that we cannot
   `Save()`, the user will be notified by a warning message in the console.

   it should be noted that the format number support differs across hardware and drivers, you
   can only load a binary that's saved by yourself, on the same platform, same card, AND with
   the same driver version, o/w loading could fail. SPIR-V binary is currently not supported.

   # smart bindings

   this class supports smart shader bindings, the previously bound shader id is remembered so
   trying to bind a shader that's already bound has 0 overhead, there's no context switching
   cost in this case (also in the texture and VAO class). Besides, the `SetUniform()` method
   is using DSA calls, so we can upload uniforms anytime, w/o having to bind the shader first.

   # compute shader

   compute shader is a separate class since it must be a standalone program all by itself, the
   underlying data buffer is setup separately from the shader, which can be either SSBO or ILS.

   when using compute shaders, keep in mind that the dispatched tasks are fired up in parallel
   on the GPU, and this parallelism of the threads needs to be synchronized properly by the
   user. Both SSBO and ILS load store operations use incoherent memory accesses, so they must
   call the memory barrier in the right place to ensure that previous writes are visible.

   the reason that OpenGL gives you this option to manage it yourself is that GPGPU tasks can
   be arbitrarily expensive and time-consuming, you often don't want to sit there and wait for
   the computation to finish. Many people like to call the memory barrier right after the call
   to dispatch, this is safe but can also be extremely wasteful. Ideally for performance, you
   should place the `SyncWait()` barrier call closest to the code that actually uses the data
   buffer, so that you don't introduce any unnecessary waits. This little trick is simple, but
   can make a huge difference in framerate especially when the computation is intensive.

   > compute_shader.Bind();
   > compute_shader.Dispatch(nx, ny, nz);
   > compute_shader.SyncWait(GL_ALL_BARRIER_BITS);  // safest but wasteful and slow!
   > compute_shader.Unbind();
*/

#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>
#include "asset/asset.h"

namespace asset {

    class Shader : public IAsset {
      protected:
        std::string source_path;
        std::string source_code;
        std::vector<GLuint> shaders;

        void ReadShader(const std::string& path, const std::string& macro, std::string& output);
        void LoadShader(GLenum type);
        void LinkShaders();

      public:  // rule of five
        Shader() : IAsset() {}
        Shader(const std::string& source_path);
        Shader(const std::string& binary_path, GLenum format);
        virtual ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&& other) = default;
        Shader& operator=(Shader&& other) = default;

      public:
        void Bind() const override;
        void Unbind() const override;

        void Save() const;
        void Inspect() const;

        template<typename T>
        void SetUniform(GLuint location, const T& val) const;

        template<typename T>
        void SetUniformArray(GLuint location, GLuint size, const std::vector<T>& vec) const;
    };
    
    class CShader : public Shader {
      private:
        GLint local_size_x, local_size_y, local_size_z;

      public:  // rule of zero
        CShader(const std::string& source_path);
        CShader(const std::string& binary_path, GLenum format);

      public:
        void Dispatch(GLuint nx, GLuint ny, GLuint nz = 1) const;
        void SyncWait(GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT) const;
    };

}