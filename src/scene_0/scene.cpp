#include "canvas.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "mesh.h"
#include "model.h"

using namespace Sketchpad;  // test

const std::string path = std::string(__FILE__);
const std::string cwd = path.substr(0, path.rfind("\\")) + "\\";  // current working directory

// -------------------------------------------------------------------------------------------
// a word of caution:
// -------------------------------------------------------------------------------------------
// global class instances must not be defined here directly, declare pointers to them instead.
// if not, they would trigger dynamic initialization at startup, which happens before main()
// function is entered, so a valid OpenGL context has not yet been created. In this case, the
// code compiles fine but the behaviors are undefined, since these class constructors depend on
// OpenGL APIs, this could cause the program to silently crash at runtime (access violation).
// -------------------------------------------------------------------------------------------

Canvas* canvas = nullptr;  // singleton canvas instance

// global pointers are ok
std::unique_ptr<Camera> camera;
//std::unique_ptr<Shader> aqua_shader, box_shader, cube_shader, skybox_shader;
std::unique_ptr<Shader> skybox_shader;
//std::unique_ptr<Mesh> aqua, box, cube, skybox;
std::unique_ptr<Mesh> skybox;

// global containers are ok
//std::vector<Texture> aqua_textures, box_textures, skybox_textures;
std::vector<Texture> skybox_textures;

// static initialization is ok
const char* scene_title = "Sample Scene";  // name your scene here


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

    //// aqua
    //aqua_shader = std::make_unique<Shader>(cwd + "aqua\\");
    //aqua_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "aqua\\albedo.jpg"));  // rvalue
    //aqua = std::make_unique<Mesh>(Primitive::Sphere, aqua_textures);

    //// box
    //box_shader = std::make_unique<Shader>(cwd + "box\\");
    //box_textures.push_back(Texture(GL_TEXTURE_2D, "albedo", cwd + "box\\albedo.jpg", true));
    //box = std::make_unique<Mesh>(Primitive::Cube, box_textures);
    //box->M = glm::translate(box->M, glm::vec3(3, 0, 0));
    //box->M = glm::scale(box->M, glm::vec3(1, 0.3f, 1));

    //// cube
    //cube_shader = std::make_unique<Shader>(cwd + "cube\\");
    //cube = std::make_unique<Mesh>(Primitive::Cube);  // no textures, use color interpolation
    //cube->M = glm::translate(cube->M, glm::vec3(3, 2, -2));
    //cube->M = glm::scale(cube->M, glm::vec3(0.8f, 0.8f, 0.8f));

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
// invoked every frame, place your draw calls and framebuffer updates here
void Update() {
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvas->Update();
    camera->Update(canvas->mouse, canvas->window, canvas->keystate, (canvas->frame_counter).delta_time, false);

    glm::mat4 V = camera->GetViewMatrix();
    glm::mat4 P = camera->GetProjectionMatrix((canvas->window).aspect_ratio);

    // draw aqua
    //aqua_shader->Bind();
    //{
    //    aqua->M = glm::rotate(aqua->M, glm::radians(0.1f), glm::vec3(0, 1, 0));
    //    aqua_shader->SetMat4("u_MVP", P * V * aqua->M);
    //    aqua->Draw(*aqua_shader);
    //}
    //aqua_shader->Unbind();

    // draw box
    //box_shader->Bind();
    //{
    //    box_shader->SetMat4("u_MVP", P * V * box->M);
    //    box->Draw(*box_shader);
    //}
    //box_shader->Unbind();

    // draw colorful cube
    //cube_shader->Bind();
    //{
    //    cube->M = glm::rotate(cube->M, glm::radians(0.5f), glm::vec3(1, 0, 0));
    //    cube_shader->SetMat4("u_MVP", P * V * cube->M);
    //    cube->Draw(*cube_shader);
    //}
    //cube_shader->Unbind();

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
