#include "canvas.h"
#include "mesh.h"
#include "texture.h"

auto CWD = std::string(__FILE__).substr(0, std::string(__FILE__).rfind("\\")) + "\\";

Canvas canvas { "Camera Control" };
Camera camera { };

Shader shader { CWD };
// Shader shader { CWD + "my_model" };  // we can define multiple shaders as global vars here
                                        // one shader per mesh, not per model
// we need a way to organize the hierarchy
// for example, each mesh has a separate folder like `mesh1`, where we have its shaders
// as well as the multiple texture images or even lighting data.
// a skybox can also be regarded as a mesh who has its own folder `skybox`, where we put the skybox shaders
// and the 6 cubemap images.

// every mesh or model should have its own unique model matrix M, which is local to it, for transformations
// but matrix V and P are global, all objects in the scene share V and P.

Mesh sphere, floor;  // declared as global, not yet defined


void Init() {
    // create meshes
    sphere = { Primitive::Sphere,
        std::vector<Texture>(
            { GL_TEXTURE_2D, "albedo", CWD + "textures\\albedo.jpg" },
            { GL_TEXTURE_2D, "normal", CWD + "textures\\normal.jpg" }
        )
    };

    floor = { Primitive::Plane,
        std::vector<Texture>({ GL_TEXTURE_2D, "albedo", CWD + "textures\\floor.jpg", true })
    };

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
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvas.Update();
    camera.Update(canvas.mouse, canvas.window.zoom, canvas.keystate, canvas.frame_counter.delta_time);
    canvas.window.zoom = 0;  // recover zoom to 0

    glm::mat4 V = camera.GetViewMatrix();
    glm::mat4 P = camera.GetProjectionMatrix(canvas.window.aspect_ratio);

    shader.Bind();
    {
        sphere.M = glm::rotate(sphere.M, glm::radians(0.1f), glm::vec3(0, 1, 0));
        shader.SetMat4("MVP", glm::value_ptr(P * V * sphere.M));
        sphere.Draw(shader);
    }
    shader.Unbind();

    shader.Bind();
    {
        shader.SetMat4("MVP", glm::value_ptr(P * V * floor.M));
        sphere.Draw(shader);
    }
    shader.Unbind();

    glutSwapBuffers();
    glutPostRedisplay();
}