#include "pch.h"

#include "core/input.h"
#include "core/log.h"
#include "buffer/fbo.h"
#include "buffer/ubo.h"
#include "components/all.h"
#include "components/component.h"
#include "scene/scene.h"
#include "scene/renderer.h"
#include "scene/ui.h"
#include "utils/path.h"

using namespace core;
using namespace buffer;
using namespace components;

namespace scene {

    Scene::Scene(const std::string& title) : title(title), directory() {}

    Scene::~Scene() {
        registry.each([this](auto id) {
            CORE_TRACE("Destroying entity: {0}", directory.at(id));
        });

        registry.clear();
    }

    Entity Scene::CreateEntity(const std::string& name, ETag tag) {
        Entity e = { name, registry.create(), &registry };

        // an entity always has a transform and a tag component
        e.AddComponent<Transform>();
        e.AddComponent<Tag>(tag);
        directory.emplace(e.id, e.name);

        return e;
    }

    void Scene::DestroyEntity(Entity e) {
        CORE_TRACE("Destroying entity: {0}", e.name);

        directory.erase(e.id);
        registry.destroy(e.id);
    }

    void Scene::AddUBO(GLuint shader_id) {
        // parse the shader to determine the structure of uniform blocks
        // a uniform buffer in the map will be keyed by its binding point

        static const std::map<GLenum, GLuint> align140 {
            // base alignment in bytes specified by std140 layout
            { GL_INT, 4 },
            { GL_BOOL, 4 },
            { GL_FLOAT, 4 },
            { GL_FLOAT_VEC2, 8 },
            { GL_FLOAT_VEC3, 16 },
            { GL_FLOAT_VEC4, 16 },
            { GL_FLOAT_MAT2, 16 },
            { GL_FLOAT_MAT3, 16 },
            { GL_FLOAT_MAT4, 16 }
        };

        static const std::map<GLenum, size_t> size140 {
            // size in bytes specified by std140 layout
            { GL_INT, 4 },
            { GL_BOOL, 4 },
            { GL_FLOAT, 4 },
            { GL_FLOAT_VEC2, 8 },
            { GL_FLOAT_VEC3, 12 },
            { GL_FLOAT_VEC4, 16 },
            { GL_FLOAT_MAT2, 16 },  // matrix is stored as an array of column vectors
            { GL_FLOAT_MAT3, 48 },  // element in an array is always padded to a multiple of the size of a vec4
            { GL_FLOAT_MAT4, 64 }
        };

        GLint n_blocks = 0;
        glGetProgramInterfaceiv(shader_id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &n_blocks);

        GLenum block_props1[] = { GL_NUM_ACTIVE_VARIABLES, GL_NAME_LENGTH, GL_BUFFER_BINDING };
        GLenum block_props2[] = { GL_ACTIVE_VARIABLES };

        // iterate over active uniform blocks (iterations are returned in the order of block indices,
        // not in the order of binding points, so binding points may jump between iterations)
        for (int i = 0; i < n_blocks; ++i) {
            GLint block_info[3];
            glGetProgramResourceiv(shader_id, GL_UNIFORM_BLOCK, i, 3, block_props1, 3, NULL, block_info);
            GLuint binding_point = block_info[2];  // now we have the uniform buffer binding point

            if (UBOs.count(binding_point) > 0) {
                continue;  // uniform block bound to this point has already been processed
            }

            char* name = new char[block_info[1]];
            glGetProgramResourceName(shader_id, GL_UNIFORM_BLOCK, i, block_info[1], NULL, name);
            std::string block_name(name);  // now we have the uniform block name
            delete[] name;

            GLint n_uniforms = block_info[0];
            GLint* indices = new GLint[n_uniforms];
            glGetProgramResourceiv(shader_id, GL_UNIFORM_BLOCK, i, 1, block_props2, n_uniforms, NULL, indices);

            // it's unclear if OpenGL would return the indices in order, this isn't mentioned in the
            // documentation, for safety, we must sort the indices to make sure that they appear in
            // the same order as which uniforms are declared in the GLSL uniform block (vital for std140)
            std::vector<GLint> uniform_indices(indices, indices + n_uniforms);
            std::sort(uniform_indices.begin(), uniform_indices.end(), [](int a, int b) { return a < b; });
            delete[] indices;

            CORE_INFO("Computing std140 aligned offset for the uniform block \"{0}\"", block_name);
            GLenum uniform_props[] = { GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX };

            std::vector<GLuint> offset;
            std::vector<size_t> size;
            GLuint curr_offset = 0;

            for (auto& index : uniform_indices) {
                GLint unif_info[3];
                glGetProgramResourceiv(shader_id, GL_UNIFORM, index, 3, uniform_props, 3, NULL, unif_info);
                GLenum data_type = unif_info[1];  // now we have the uniform data type

                GLint name_length = unif_info[0];
                char* name = new char[name_length];
                glGetProgramResourceName(shader_id, GL_UNIFORM, index, name_length, NULL, name);
                std::string unif_name(name);  // now we have the uniform name
                delete[] name;

                // find the base alignment and byte size of the uniform
                GLuint base_align = 0;
                size_t byte_size = 0;

                try {
                    base_align = align140.at(data_type);
                    byte_size = size140.at(data_type);
                }
                catch (const std::out_of_range& oor) {
                    CORE_ERROR("Uniform \"{0}\" is using an unsupported data type...", unif_name);
                    CORE_ASERT(false, "Unable to find the uniform base alignment: {0}", oor.what());
                }

                // the aligned byte offset of a uniform must be equal to a multiple of its base alignment
                GLuint padded_offset = static_cast<GLuint>(ceil(curr_offset / (float)base_align) * base_align);
                offset.push_back(padded_offset);
                size.push_back(byte_size);
                curr_offset = padded_offset + byte_size;
            }

            // after the last iteration, curr_offset = total buffer size in bytes (padding included)
            UBOs.try_emplace(binding_point, binding_point, curr_offset);
            UBOs[binding_point].SetOffset(offset);
            UBOs[binding_point].SetSize(size);
        }
    }

    FBO& Scene::AddFBO(GLuint n_color_buff, GLuint width, GLuint height) {
        // framebuffers are simply indexed by the order of creation
        GLuint key = FBOs.size();
        FBOs.try_emplace(key, n_color_buff, width, height);
        return FBOs[key];
    }

    // this base class is being used to render our welcome screen
    static asset_ref<Texture> welcome_screen;
    static ImTextureID welcome_screen_texture_id;

    void Scene::Init() {
        welcome_screen = LoadAsset<Texture>(GL_TEXTURE_2D, TEXTURE + "misc\\welcome.png");
        welcome_screen_texture_id = (void*)(intptr_t)(welcome_screen->id);
    }

    void Scene::OnSceneRender() {
        Renderer::Clear();
    }

    void Scene::OnImGuiRender() {
        ui::DrawWelcomeScreen(welcome_screen_texture_id);
    }

}
