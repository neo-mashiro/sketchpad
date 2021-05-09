#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "canvas.h"
#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

const std::string path = std::string(__FILE__);
const std::string cwd = path.substr(0, path.rfind("\\")) + "\\";  // current working directory

// -------------------------------------------------------------------------------------------
// A word of caution:
// -------------------------------------------------------------------------------------------
// Global class instances must not be defined here directly, declare pointers to them instead.
// If not, they would trigger dynamic initialization at startup, which happens before main()
// function is entered, so a valid OpenGL context has not yet been created. In this case, the
// code compiles fine but the behaviors are undefined, since these class constructors depend on
// OpenGL APIs, this could cause the program to silently crash at runtime (access violation).
// -------------------------------------------------------------------------------------------

Canvas* canvas = nullptr;  // singleton instance

std::unique_ptr<Camera> camera;
std::unique_ptr<Shader> aqua_shader, plane_shader, skybox_shader;
std::unique_ptr<Mesh> aqua, plane;
std::vector<Texture> aqua_textures, plane_textures;

// static initialization is ok
const char* scene_title = "Sample Scene";  // name your scene here


void InitScene() {
    glutSetWindowTitle(scene_title);

    // retrieve the global canvas instance
    canvas = Canvas::GetInstance();

    // create main camera
    camera = std::make_unique<Camera>();

    // aqua
    aqua_shader = std::make_unique<Shader>(cwd + "aqua\\");
    aqua_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "aqua\\albedo.jpg"));  // rvalue
    aqua = std::make_unique<Mesh>(Primitive::Sphere, aqua_textures);

    // plane
    plane_shader = std::make_unique<Shader>(cwd + "plane\\");
    plane_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "plane\\albedo.jpg", true));
    plane = std::make_unique<Mesh>(Primitive::Plane, plane_textures);

    // enable face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

void Display() {
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvas->Update();
    camera->Update(canvas->mouse, canvas->window, canvas->keystate, (canvas->frame_counter).delta_time);

    glm::mat4 V = camera->GetViewMatrix();
    glm::mat4 P = camera->GetProjectionMatrix((canvas->window).aspect_ratio);

    aqua_shader->Bind();
    aqua->M = glm::rotate(aqua->M, glm::radians(0.1f), glm::vec3(0, 1, 0));
    aqua_shader->SetMat4("u_MVP", P * V * aqua->M);
    aqua->Draw(*aqua_shader);
    aqua_shader->Unbind();

    plane_shader->Bind();
    plane_shader->SetMat4("u_MVP", P * V * plane->M);
    plane->Draw(*plane_shader);
    plane_shader->Unbind();

    glutSwapBuffers();
    glutPostRedisplay();
}
