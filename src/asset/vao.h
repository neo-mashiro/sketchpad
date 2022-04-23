/* 
   with the use of DSA, our VAO doesn't care the types of vbo/ido or which target they are
   bound to, as long as the vbo/ibo param is a valid buffer object, `SetVBO()` / `SetIBO()`
   will work properly. For example, for procedural meshes we may have vertices data coming
   from a SSBO that is updated by a compute shader, so data is already on the GPU side, in
   this case, there's no need to make a redundant GPU -> CPU -> GPU round trip, instead we
   can directly pass the SSBO's id to `SetVBO()`, which will be treated as if it was a VBO
   bound to the `GL_ARRAY_BUFFER` target. Buffer objects are essentially just data store.
*/

#pragma once

#include "asset/asset.h"

namespace asset {

    class VAO : public IAsset {
      public:
        VAO();
        ~VAO();

        VAO(const VAO&) = delete;
        VAO& operator=(const VAO&) = delete;
        VAO(VAO&& other) noexcept = default;
        VAO& operator=(VAO&& other) noexcept = default;

        void Bind() const override;
        void Unbind() const override;

        void SetVBO(GLuint vbo, GLuint attr_id, GLint offset, GLint size, GLint stride, GLenum type) const;
        void SetIBO(GLuint ibo) const;
        void Draw(GLenum mode, GLsizei count);
    };

}
