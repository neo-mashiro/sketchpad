#pragma once

#include "buffer/buffer.h"

namespace buffer {

    class VAO : public Buffer {
      public:
        VAO();
        ~VAO();

        void Bind() const override;
        void Unbind() const override;

        void SetLayout() const;
    };

}
