#pragma once

#include <glad/glad.h>

namespace asset {

    class IAsset {
      protected:
        GLuint id;

      public:
        GLuint ID() const { return this->id; }

      protected:
        IAsset();
        virtual ~IAsset() {};  // ensure derived classes are destructed in order

        IAsset(const IAsset&) = delete;
        IAsset& operator=(const IAsset&) = delete;
        IAsset(IAsset&& other) noexcept;
        IAsset& operator=(IAsset&& other) noexcept;

        virtual void Bind() const {}
        virtual void Unbind() const {}

        virtual void Bind(GLuint index) const {}
        virtual void Unbind(GLuint index) const {}
    };

}
