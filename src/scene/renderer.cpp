#include "pch.h"

#include "scene/renderer.h"

namespace scene {

    void Renderer::EnableDepthTest() {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthRange(0.0f, 1.0f);
    }

    void Renderer::DisableDepthTest() {
        glDisable(GL_DEPTH_TEST);
    }

    void Renderer::EnableFaceCulling() {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
    }

    void Renderer::DisableFaceCulling() {
        glDisable(GL_CULL_FACE);
    }

    void Renderer::SetFrontFace(bool ccw) {
        glFrontFace(ccw ? GL_CCW : GL_CW);
    }

    void Renderer::ClearBuffer() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

}
