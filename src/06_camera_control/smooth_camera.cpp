#include "canvas.h"
#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

std::string path = std::string(__FILE__);
std::string cwd = path.substr(0, path.rfind("\\")) + "\\";  // current working directory

// Global class instances must not be defined here directly, declare pointers to them instead.
// If not, they would trigger dynamic initialization at startup, which happens before the main()
// function is entered, so a valid OpenGL context has not yet been created. In this case, the
// code compiles fine but the behaviors are undefined, since these class constructors depend on
// OpenGL APIs, this could cause the program to silently crash at runtime (access violation).

Canvas* canvas = nullptr;  // singleton instance
std::unique_ptr<Camera> camera;
std::unique_ptr<Shader> sphere_shader, plane_shader, skybox_shader;
std::unique_ptr<Mesh> sphere, plane;
std::vector<Texture> sphere_textures, plane_textures;

// static initialization is OK, no constructors will be called
unsigned int scene_id = 2;

void InitScene() {
    // find our canvas instance
    canvas = Canvas::GetInstance();

    // create camera
    camera = std::make_unique<Camera>();

    // create sphere
    sphere_shader = std::make_unique<Shader>(cwd + "sphere\\");
    sphere_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "sphere\\albedo.jpg"));  // rvalue
    sphere_textures.push_back(Texture(GL_TEXTURE_2D, "normal", cwd + "sphere\\normal.jpg"));  // rvalue
    sphere = std::make_unique<Mesh>(Primitive::Sphere, sphere_textures);

    // create plane
    plane_shader = std::make_unique<Shader>(cwd + "floor\\");
    plane_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "floor\\albedo.jpg", true));
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
    camera->Update(canvas->mouse, (canvas->window).zoom, canvas->keystate, (canvas->frame_counter).delta_time);

    // fix black textures first
    // then fix the viewport size

    // group these recover operations into the camera class
    // (need change arguments types to by references &)
    (canvas->window).zoom = 0;  // recover zoom to 0
    (canvas->mouse).delta_x = (canvas->mouse).delta_y = 0;  // recover mouse offset

    glm::mat4 V = camera->GetViewMatrix();
    glm::mat4 P = camera->GetProjectionMatrix((canvas->window).aspect_ratio);

    sphere_shader->Bind();
    {
        sphere->M = glm::rotate(sphere->M, glm::radians(0.1f), glm::vec3(0, 1, 0));
        sphere_shader->SetMat4("u_MVP", P * V * sphere->M);
        sphere->Draw(*sphere_shader);
    }
    sphere_shader->Unbind();

    plane_shader->Bind();
    {
        plane_shader->SetMat4("u_MVP", P * V * plane->M);
        plane->Draw(*plane_shader);
    }
    plane_shader->Unbind();

    glutSwapBuffers();
    glutPostRedisplay();
}
