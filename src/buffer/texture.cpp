#include "pch.h"

#include "core/log.h"
#include "core/window.h"
#include "buffer/fbo.h"
#include "buffer/sampler.h"
#include "buffer/texture.h"
#include "components/mesh.h"
#include "components/shader.h"
#include "utils/filesystem.h"
#include "utils/math.h"
#include "utils/image.h"

namespace buffer {

    // optimize context switching by avoiding unnecessary binds and unbinds
    static std::vector<GLuint> texture_binding_table(32, 0);  // keep track of textures in each unit

    ///////////////////////////////////////////////////////////////////////////////////////////////

    Texture::Texture(const std::string& img_path, GLuint levels)
        : Buffer(), target(GL_TEXTURE_2D), width(0), height(0), n_levels(levels), format(0), internal_format(0) {

        auto image = utils::Image(img_path);

        width = image.GetWidth();
        height = image.GetHeight();
        format = image.GetFormat();
        internal_format = image.GetInternalFormat();
        
        // if levels is 0, automatic compute the number of mipmap levels needed
        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(floor(std::log2(std::max(width, height))));
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        glTextureStorage2D(id, n_levels, internal_format, width, height);

        if (image.IsHDR()) {
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_FLOAT, image.GetPixels<float>());
        }
        else {
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, image.GetPixels<uint8_t>());
        }

        if (n_levels > 1) {
            glGenerateTextureMipmap(id);
        }

        Sampler::SetDefaultState(*this);
    }

    Texture::Texture(const std::string& img_path, GLuint resolution, GLuint levels)
        : Buffer(), target(GL_TEXTURE_CUBE_MAP), width(resolution), height(resolution),
          format(GL_RGB), internal_format(GL_RGB16F), n_levels(levels) {

        using namespace glm;
        using namespace components;
        using namespace utils;

        // resolution must be a power of 2 in order to achieve high-fidelity visual effects
        if (!math::IsPowerOfTwo(resolution)) {
            CORE_ERROR("Attempting to build a cubemap whose resolution is not a power of 2...");
            return;
        }

        // a cubemap texture should be preferably created from a high dynamic range image
        if (auto path = std::filesystem::path(img_path); path.extension() != ".hdr") {
            CORE_WARN("Attempting to build a cubemap from a non-HDR image...");
            CORE_WARN("Visual quality might drop seriously after tone mapping...");
        }

        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(floor(std::log2(std::max(width, height))));
        }

        // load the equirectangular image into a temporary 2D texture (base level, no mipmaps)
        GLuint equirectangle = 0;
        auto image = utils::Image(img_path, 3, true);
        GLuint im_w = image.GetWidth();
        GLuint im_h = image.GetHeight();

        glCreateTextures(GL_TEXTURE_2D, 1, &equirectangle);
        glTextureParameteri(equirectangle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(equirectangle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(equirectangle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(equirectangle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        if (image.IsHDR()) {
            internal_format = GL_RGB16F;
            glTextureStorage2D(equirectangle, 1, internal_format, im_w, im_h);
            glTextureSubImage2D(equirectangle, 0, 0, 0, im_w, im_h, GL_RGB, GL_FLOAT, image.GetPixels<float>());
        }
        else {
            internal_format = GL_RGB8;
            glTextureStorage2D(equirectangle, 1, internal_format, im_w, im_h);
            glTextureSubImage2D(equirectangle, 0, 0, 0, im_w, im_h, GL_RGB, GL_UNSIGNED_BYTE, image.GetPixels<uint8_t>());
        }

        // create this texture as an empty cubemap to hold the equirectangle
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, n_levels, internal_format, width, height);

        // render the 2D equirectangle into the six faces of the cubemap
        auto virtual_mesh = Mesh(Primitive::Cube);
        auto virtual_shader = Shader(paths::shaders + "equirectangle.glsl");
        auto virtual_framebuffer = FBO(width, height);
        virtual_framebuffer.SetColorTexture(0, id, 0);

        static const mat4 projection = glm::perspective(radians(90.0f), 1.0f, 0.1f, 10.0f);

        static const mat4 views[6] = {
            glm::lookAt(vec3(0.0f), vec3(+1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),  // posx
            glm::lookAt(vec3(0.0f), vec3(-1.0f,  0.0f,  0.0f), vec3(0.0f, -1.0f,  0.0f)),  // negx
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  1.0f,  0.0f), vec3(0.0f,  0.0f,  1.0f)),  // posy
            glm::lookAt(vec3(0.0f), vec3(+0.0f, -1.0f,  0.0f), vec3(0.0f,  0.0f, -1.0f)),  // negy
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  0.0f,  1.0f), vec3(0.0f, -1.0f,  0.0f)),  // posz
            glm::lookAt(vec3(0.0f), vec3(+0.0f,  0.0f, -1.0f), vec3(0.0f, -1.0f,  0.0f))   // negz
        };

        glViewport(0, 0, width, height);
        virtual_framebuffer.Bind();
        virtual_shader.Bind();
        glBindTextureUnit(0, equirectangle);

        for (GLuint face = 0; face < 6; face++) {
            virtual_framebuffer.SetColorTexture(0, id, face);
            virtual_framebuffer.Clear(0);
            virtual_framebuffer.Clear(-1);

            glProgramUniformMatrix4fv(virtual_shader.GetID(), 0, 1, GL_FALSE, &views[face][0][0]);
            glProgramUniformMatrix4fv(virtual_shader.GetID(), 1, 1, GL_FALSE, &projection[0][0]);

            virtual_mesh.Draw();
        }

        glBindTextureUnit(0, 0);
        virtual_shader.Unbind();
        virtual_framebuffer.Unbind();
        glViewport(0, 0, core::Window::width, core::Window::height);

        if (n_levels > 1) {
            glGenerateTextureMipmap(id);
        }

        Sampler::SetDefaultState(*this);

        // manually delete the temporary 2D equirectangle texture
        glDeleteTextures(1, &equirectangle);
    }

    Texture::Texture(const std::string& directory, const std::string& extension, GLuint resolution, GLuint levels)
        : Buffer(), target(GL_TEXTURE_CUBE_MAP), width(resolution), height(resolution),
          format(GL_RGB), internal_format(GL_RGB16F), n_levels(levels) {

        static const std::vector<std::string> faces { "px", "nx", "py", "ny", "pz", "nz" };

        // resolution must be a power of 2 in order to achieve high-fidelity visual effects
        if (!utils::math::IsPowerOfTwo(resolution)) {
            CORE_ERROR("Attempting to build a cubemap whose resolution is not a power of 2...");
            return;
        }

        // this ctor only accepts HDR format faces of the cubemap
        std::string test_face = directory + faces[0] + extension;
        if (!std::filesystem::exists(std::filesystem::path(test_face))) {
            CORE_ERROR("Cannot find cubemap face {0} in the directory...", test_face);
            return;
        }

        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(floor(std::log2(std::max(width, height))));
        }

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
        glTextureStorage2D(id, n_levels, internal_format, width, height);

        for (GLuint face = 0; face < 6; face++) {
            auto image = utils::Image(directory + faces[face] + extension, 3, true);
            CORE_ASERT(image.IsHDR(), "Invalid face image file extension, expected HDR-format images...");
            glTextureSubImage3D(id, 0, 0, 0, face, width, height, 1, format, GL_FLOAT, image.GetPixels<float>());
        }

        if (n_levels > 1) {
            glGenerateTextureMipmap(id);
        }

        Sampler::SetDefaultState(*this);
    }

    Texture::Texture(GLenum target, GLuint width, GLuint height, GLenum internal_format, GLuint levels, bool multisample)
        : Buffer(), target(target), width(width), height(height),
          n_levels(levels), format(0), internal_format(internal_format) {

        // if levels is 0, automatic compute the number of mipmap levels needed
        if (levels == 0) {
            n_levels = 1 + static_cast<GLuint>(floor(std::log2(std::max(width, height))));
        }

        glCreateTextures(target, 1, &id);

        if (multisample) {
            glTextureStorage2DMultisample(id, 4, internal_format, width, height, GL_TRUE);
        }
        else {
            glTextureStorage2D(id, n_levels, internal_format, width, height);
        }
        
        Sampler::SetDefaultState(*this);
    }

    Texture::~Texture() {
        if (id == 0) return;

        CORE_WARN("Destructing texture data (target = {0}, id = {1})...", target, id);
        glDeleteTextures(1, &id);

        for (int i = 0; i < texture_binding_table.size(); i++) {
            if (texture_binding_table[i] = id) {
                texture_binding_table[i] = 0;
            }
        }
    }

    Texture::Texture(Texture&& other) noexcept {
        *this = std::move(other);
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this != &other) {
            this->id = 0;  // it's safe to delete texture 0 (a default fallback texture that is all black)
            std::fill(texture_binding_table.begin(), texture_binding_table.end(), 0);
            std::swap(id, other.id);
            std::swap(target, other.target);
            std::swap(width, other.width);
            std::swap(height, other.height);
            std::swap(n_levels, other.n_levels);
            std::swap(format, other.format);
            std::swap(internal_format, other.internal_format);
        }

        return *this;
    }

    void Texture::Bind(GLuint unit) const {
        // keep track of the texture in each unit to avoid unnecessary binds
        // the DSA call does not alter the global state of active texture unit
        if (id != texture_binding_table[unit]) {
            glBindTextureUnit(unit, id);
            texture_binding_table[unit] = id;
        }
    }

    void Texture::Unbind(GLuint unit) const {
        glBindTextureUnit(unit, 0);
        texture_binding_table[unit] = 0;
    }

    void Texture::Clear() const {
        if (internal_format == GL_RGBA16F) {
            std::vector<GLubyte> zeros(width * height, 0);
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_FLOAT, &zeros[0]);
        }
        else {
            std::vector<GLubyte> zeros(width * height, 0);
            glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, &zeros[0]);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    TexView::TexView(const Texture& texture, GLuint levels) : Buffer() {
        glGenTextures(1, &id);
        glTextureView(id, texture.target, texture.id, texture.internal_format, 0, levels, 0, 1);
    }

    TexView::~TexView() {
        glDeleteTextures(1, &id);

        for (int i = 0; i < texture_binding_table.size(); i++) {
            if (texture_binding_table[i] = id) {
                texture_binding_table[i] = 0;
            }
        }
    }

    void TexView::Bind(GLuint unit) const {
        // keep track of the texture in each unit to avoid unnecessary binds
        if (id != texture_binding_table[unit]) {
            glBindTextureUnit(unit, id);
            texture_binding_table[unit] = id;
        }
    }

    void TexView::Unbind(GLuint unit) const {
        glBindTextureUnit(unit, 0);
        texture_binding_table[unit] = 0;
    }

}
