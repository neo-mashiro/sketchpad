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

Canvas* canvas = nullptr;  // singleton canvas instance

// global pointers are ok
std::unique_ptr<Camera> camera;
std::unique_ptr<Shader> cube_shader, skybox_shader;
std::unique_ptr<Mesh> cube, skybox;

// global containers are ok
std::vector<Texture> skybox_textures;

// static initialization is ok
const char* scene_title = "Sample Scene 2";  // name your scene here


// event function: this is called right after the OpenGL context has been established
// use this function to initialize your scene configuration, shaders, models, etc.
void Start() {
    glutSetWindowTitle(scene_title);

    // retrieve the global canvas instance
    canvas = Canvas::GetInstance();

    // create main camera
    camera = std::make_unique<Camera>();

    // skybox
    skybox_shader = std::make_unique<Shader>(cwd + "skybox\\");
    skybox_textures.push_back(Texture(GL_TEXTURE_CUBE_MAP, "skybox", cwd + "skybox\\"));  // rvalue
    skybox = std::make_unique<Mesh>(Primitive::Cube, skybox_textures);

    // reflective cube
    cube_shader = std::make_unique<Shader>(cwd + "cube\\");
    cube = std::make_unique<Mesh>(Primitive::Cube);  // no textures, reflect the skybox

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

// event function: this is registered as the OpenGL display callback, which is to be
// invoked every frame, place your draw calls and framebuffer updates here.
void Update() {
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvas->Update();
    camera->Update(canvas->mouse, canvas->window, canvas->keystate, (canvas->frame_counter).delta_time, false);

    glm::mat4 V = camera->GetViewMatrix();
    glm::mat4 P = camera->GetProjectionMatrix((canvas->window).aspect_ratio);

    // draw reflective cube
    cube_shader->Bind();
    {
        cube_shader->SetMat4("u_MVP", P * V * cube->M);
        cube_shader->SetVec3("camera_pos", camera->position);
        cube_shader->SetInt("skybox", 0);  // this line has bug!!! check the shader
        cube->Draw(*cube_shader);
    }
    cube_shader->Unbind();

    // drawing skybox at last can save us many draw calls, because it is the farthest object in
    // the scene, which should be rendered behind all other objects.
    // with depth test enabled, pixels that fail the test (obstructed by other objects) are skipped.
    skybox_shader->Bind();
    glFrontFace(GL_CW);  // skybox has reversed winding order, we only draw the inner faces
    {
        // skybox is stationary, it doesn't move with the camera (fixed position)
        // so we use a rectified view matrix whose translation components are removed
        skybox_shader->SetMat4("u_MVP", P * glm::mat4(glm::mat3(V)) * skybox->M);
        skybox->Draw(*skybox_shader, true);
    }
    glFrontFace(GL_CCW);  // reset the global winding order to draw only outer faces
    skybox_shader->Unbind();

    glutSwapBuffers();
    glutPostRedisplay();
}