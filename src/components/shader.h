#pragma once

#include <string>
#include <vector>
#include <type_traits>
#include <GL/glew.h>
#include "components/component.h"

namespace components {

    /* our shader class is a convenient encapsulation of the linked GLSL program in the pipeline,
       there are 2 different flavors of constructors. The first ctor constructs the GLSL program
       by compiling from a user-provided shader source, which is the absolute path of a file with
       the ".glsl" extension, the second ctor does not compile or link anything, but instead load
       from a pre-compiled shader binary file on your local disk. The usages are detailed below.

       many people like to use a separate file for each type of shader and group them in a folder,
       this is the most obvious way of handling shaders. However, the downside of this is that we
       would end up with lots of code duplication, also, beyond a certain point, we would have too
       many files and folders for different objects in the scene, it's not only hard to manage but
       also can be extremely painful in common scenarios, for example, when you modify a shared
       uniform buffer that is declared in multiple files, then you have to modify every file......
       the way we choose to deal with shaders is to write all shaders in a single large ".glsl"
       file: all shared code is placed on the top of the file to avoid duplication, followed by
       code blocks that are specific to each shader type, for each type of shader, its own code is
       enclosed between a pair of "#ifdef" and "#endif" preprocessor guards, when the class reads
       the file, it will loop through all possible shader types and "#define" them before actually
       compiling, so that the compiler can selectively compile shaders one at a time (note that
       compute shaders are handled differently down below)

       as per the GLSL specification, "#version" must be the first line of preprocessor directive
       in the shader code, otherwise it would fallback to the default version 1.1, which is not
       the version we want. Therefore, instead of grabbing the entire buffer all at once, this
       class will read the source file line by line, and then insert a "#define" before the first
       occurrence of "#ifdef", in order to "#define" the current shader type.

       apart from the "#ifdef" and "#endif" guards, we also added another preprocessor directive
       called "#include" to enhance the power of our shader class. This directive works similar to
       the one that is widely used in a C++ script, which simply copies and pastes the contents of
       another file to the shader file. The reason we want to do this is that it further reduces
       code clutter and duplication, which also makes it easier to modify. If you have a bunch of
       data buffer blocks or custom functions that are frequently used by many shaders, it's often
       a nice idea to group them in a single file, and then "#include" it whenever you need it.
       there's only one rule you need to keep in mind, that the file to include must be in the
       same directory as the shader that calls "#include". If the file does not exist or cannot
       open (probably misspelled), the class will ignore the line and continue to compile the rest,
       in this case, the console will print a warning message so that users are likely aware of it.

       in order to use the second ctor, you must first `Save()` an already compiled shader program
       to the local disk and then load from it. On save, the program will be saved into the same
       directory in a ".bin" file, whose filename is a format number (integer) that depends on the
       hardware, such as 3274 on my dedicated AMD card, or 1 on my integrated Intel card. If there
       is no format number supported, the console will log a warning message to notify the user.
       it should be noted that the save function is implementation-dependent, which differs across
       hardware drivers, so you can only load a binary that is saved by yourself, but you may not
       be able to load one that is saved by a different platform or even a previous driver version.
       last but not least, loading from a SPIR-V binary is currently not supported...

       in this demo, our shader class is in the "components" namespace. Despite the fact that it's
       not a component, it's directly used by our "Material" component, so does the texture class.
       in a professional rendering or game engine, shaders and textures are usually classified as
       external asset resources, all of which are managed by a specialized asset manager, which
       takes on the single responsibility to load or free them as appropriate. That said, we won't
       bother with that in this demo, I found it easier to work with this way.

       you may have noticed that our shader class doesn't have any functions for querying uniform
       locations or setting uniform values, this is on purpose! In fact, we would never query the
       locations of uniforms at all, the user must explicitly specify the locations in GLSL (this
       enforcement is also applied to uniform buffer binding points, shader storage buffer indices,
       subroutine uniform indices, ILS image units and so on) to keep the code clean. Doing so has
       the benefit of 0 overhead, it can also help automate certain tasks, as long as you remember
       the locations of uniforms, the template uniform class and material class will handle the
       rest for you, no worries about the different data types or uploading values to the shader!
    */

    class Shader {
      private:
        std::vector<GLuint> shaders;

        void LoadShader(GLenum type);
        void LinkShaders();

      public:
        GLuint id;
        std::string source_path;

        Shader(const std::string& source_path);
        Shader(const std::string& binary_path, GLenum format);
        virtual ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        void Bind() const;
        void Unbind() const;
        void Save() const;

        static void Sync(GLbitfield barriers);
    };

    /* compute shader is a separate class since it must be a standalone program all by itself, the
       underlying data buffer is setup separately from the shader, which can be either SSBO or ILS.
       the usage of compute shader is exactly the same as a normal shader, you can still save and
       load binaries, use the "#ifdef" and "#include" directives, but the shader file can only
       contain the code of a compute shader, not with other types of shaders.
       
       for a compute shader, it's important to keep in mind that the dispatched tasks are fired up
       in parallel on the GPU, and that this parallelism of the threads needs to be synchronized
       by the user manually (this is often only required for tessellation and compute shaders).
       both the SSBO and ILS reads and writes use incoherent memory accesses, so they must call
       the memory barrier to ensure that previous writes are visible.

       the reason that OpenGL gives you this option to manage it yourself is that GPGPU tasks can
       be arbitrarily expensive and time-consuming, you often don't want to sit there and wait for
       the computation to finish. Many people like to call the memory barrier right after the call
       to dispatch, this is safe but also can be very wasteful. For better performance, you should
       put the `SyncWait()` barrier call closest to the code that actually uses the data buffer,
       so that you don't introduce any unnecessary waits, this little trick is easy, but can make
       a huge difference in the framerate especially when your code is large.

       if you're only concerned about thread safety or data integrity, put the calls together:

           compute_shader.Bind();
           compute_shader.Dispatch(nx, ny, nz);
           compute_shader.SyncWait(GL_ALL_BARRIER_BITS);  // wait for everything
           compute_shader.Unbind();
    */

    class ComputeShader : public Shader {
      private:
        static GLint max_workgroup_count[3];
        static GLint max_workgroup_size[3];
        static GLint max_invocations;
        
      public:
        ComputeShader(const std::string& source_path)                : Shader(source_path) {}
        ComputeShader(const std::string& binary_path, GLenum format) : Shader(binary_path, format) {}
        ~ComputeShader() {}  // the base virtual destructor will be automatically called

        ComputeShader(const ComputeShader&) = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;

        ComputeShader(ComputeShader&& other) noexcept : Shader(std::move(other)) {}
        ComputeShader& operator=(ComputeShader&& other) noexcept {
            Shader::operator=(std::move(other));
            return *this;
        }

        void Bind()   const { Shader::Bind(); }
        void Unbind() const { Shader::Unbind(); }
        void Save()   const { Shader::Save(); }

        void Dispatch(GLuint nx, GLuint ny, GLuint nz = 1) const;
        void SyncWait(GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT) const;

        template<typename T>
        void SetUniform(GLuint location, const T& value) const {
            /**/ if constexpr (std::is_same_v<T, bool>)      { glUniform1i(location, static_cast<int>(val)); }
            else if constexpr (std::is_same_v<T, int>)       { glUniform1i(location, val); }
            else if constexpr (std::is_same_v<T, float>)     { glUniform1f(location, val); }
            else if constexpr (std::is_same_v<T, glm::vec2>) { glUniform2fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::vec3>) { glUniform3fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::vec4>) { glUniform4fv(location, 1, &val[0]); }
            else if constexpr (std::is_same_v<T, glm::mat2>) { glUniformMatrix2fv(location, 1, GL_FALSE, &val[0][0]); }
            else if constexpr (std::is_same_v<T, glm::mat3>) { glUniformMatrix3fv(location, 1, GL_FALSE, &val[0][0]); }
            else if constexpr (std::is_same_v<T, glm::mat4>) { glUniformMatrix4fv(location, 1, GL_FALSE, &val[0][0]); }
            else {
                static_assert(const_false<T>, "Unspecified template uniform type T ...");
            }
        }
    };

}
