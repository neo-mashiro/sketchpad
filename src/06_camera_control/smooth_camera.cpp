#include "define.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Window window{};
Camera camera{};

std::vector<glm::vec3> positions;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;
std::vector<float> vertices;
std::vector<unsigned int> indices;

GLuint VAO, VBO, IBO, PO;
GLuint base, overlay;  // textures

glm::mat4 M, V, P;

// frame counter
float delta_time;
float last_frame;

// mouse movement
float sensitivity = 0.05f;
float zoom_speed = 2.0f;
int last_mouse_x, last_mouse_y;

// direction keys control
bool u_pressed = false;
bool d_pressed = false;
bool l_pressed = false;
bool r_pressed = false;

void CreateSphereMesh() {
    // mesh grid size
    unsigned int n_rows = 500;
    unsigned int n_cols = 500;

    for (unsigned int col = 0; col <= n_cols; ++col) {
        for (unsigned int row = 0; row <= n_rows; ++row) {
            float u = (float)row / (float)n_rows;
            float v = (float)col / (float)n_cols;
            float x = cos(u * PI * 2) * sin(v * PI);
            float y = cos(v * PI);
            float z = sin(u * PI * 2) * sin(v * PI);

            positions.push_back(glm::vec3(x, y, z));
            uvs.push_back(glm::vec2(u, v));
            normals.push_back(glm::vec3(x, y, z));  // sphere centered at the origin, normal = position
        }
    }

    for (unsigned int col = 0; col < n_cols; ++col) {
        for (unsigned int row = 0; row < n_rows; ++row) {
            // counter-clockwise
            indices.push_back((col + 1) * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row + 1);

            // counter-clockwise
            indices.push_back((col + 1) * (n_rows + 1) + row);
            indices.push_back(col * (n_rows + 1) + row + 1);
            indices.push_back((col + 1) * (n_rows + 1) + row + 1);
        }
    }

    for (unsigned int i = 0; i < positions.size(); ++i) {
        vertices.push_back(positions[i].x);
        vertices.push_back(positions[i].y);
        vertices.push_back(positions[i].z);
        vertices.push_back(uvs[i].x);
        vertices.push_back(uvs[i].y);
        vertices.push_back(normals[i].x);
        vertices.push_back(normals[i].y);
        vertices.push_back(normals[i].z);
    }
}

void LoadTexture(const std::string& path) {
    stbi_set_flip_vertically_on_load(false);

    int width, height, n_channels;
    unsigned char* buffer = stbi_load(path.c_str(), &width, &height, &n_channels, 0);

    if (!buffer) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(buffer);
        return;
    }

    if (n_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    }
    else if (n_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
    
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(buffer);
}

void SetupWindow() {
    window.title = "Camera Control";
    SetupDefaultWindow(window);
}

void Init() {
    CreateSphereMesh();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);  // position
    glEnableVertexAttribArray(1);  // uv
    glEnableVertexAttribArray(2);  // normal
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 5));

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    std::string file_path = __FILE__;
    std::string dir = file_path.substr(0, file_path.rfind("\\")) + "\\";
    PO = CreateShader(dir);

    // load base texture, generate mipmaps
    glGenTextures(1, &base);
    glBindTexture(GL_TEXTURE_2D, base);  // now the lines below will be tied to base

    LoadTexture(dir + "textures\\base.jpg");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // bilinear filtering

    glBindTexture(GL_TEXTURE_2D, 0);

    // load normal map
    glGenTextures(1, &overlay);
    glBindTexture(GL_TEXTURE_2D, overlay);  // now the lines below will be tied to normal_map

    LoadTexture(dir + "textures\\normal.jpg");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    // set texture uniforms
    glUseProgram(PO);
    glUniform1i(glGetUniformLocation(PO, "base"), 0);     // bind to texture unit 0
    glUniform1i(glGetUniformLocation(PO, "overlay"), 1);  // bind to texture unit 1

    // init model view projection
    P = glm::perspective(glm::radians(camera.fov), window.aspect_ratio, 0.1f, 100.0f);
    V = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
    M = glm::mat4(1.0f);
    last_mouse_x = window.width / 2;
    last_mouse_y = window.height / 2;

    // face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
}

// update camera positions based on global key pressing states, to be invoked every frame
void SmoothKeyControl() {
    if (u_pressed) {
        camera.position += camera.forward * (camera.speed * delta_time);
    }
    if (d_pressed) {
        camera.position -= camera.forward * (camera.speed * delta_time);
    }
    if (l_pressed) {
        camera.position -= camera.right * (camera.speed * delta_time);
    }
    if (r_pressed) {
        camera.position += camera.right * (camera.speed * delta_time);
    }

    camera.position.y = 0.0f;  // snap to the ground
}

void Display() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(PO);
    glBindVertexArray(VAO);

    {
        float this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        delta_time = this_frame - last_frame;
        last_frame = this_frame;

        SmoothKeyControl();

        P = glm::perspective(glm::radians(camera.fov), window.aspect_ratio, 0.1f, 100.0f);
        V = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
        M = glm::rotate(M, glm::radians(0.1f), glm::vec3(0, 1, 0));
        glUniformMatrix4fv(glGetUniformLocation(PO, "mvp"), 1, GL_FALSE, glm::value_ptr(P * V * M));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, base);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, overlay);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
    glutPostRedisplay();
}

void Reshape(int width, int height) {
    DefaultReshapeCallback(window, width, height);
}

// keyboard callback does not report direction keys or WASD, for that we need the special callback
void Keyboard(unsigned char key, int x, int y) {
    DefaultKeyboardCallback(key, x, y);
}

// special callback does respond to direction keys, but it's not invoked continuously every frame.
// when a key is held down, this is only invoked once every few frames, so if we update camera in
// this function, the updates are not smooth and would result in noticeable jerky movement.
// therefore, here we are only going to set the global key pressing states.
// camera updates are done in the display callback based on these states, which happens every frame.
void Special(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:    u_pressed = true; break;
        case GLUT_KEY_DOWN:  d_pressed = true; break;
        case GLUT_KEY_LEFT:  l_pressed = true; break;
        case GLUT_KEY_RIGHT: r_pressed = true; break;
    }
}

// this callback responds to key releasing events, where we can reset key pressing states to false.
void SpecialUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:    u_pressed = false; break;
        case GLUT_KEY_DOWN:  d_pressed = false; break;
        case GLUT_KEY_LEFT:  l_pressed = false; break;
        case GLUT_KEY_RIGHT: r_pressed = false; break;
    }
}

void Mouse(int button, int state, int x, int y) {
    // in freeglut, each scroll wheel event is reported as a button click
    if (button == 3 && state == GLUT_DOWN) {  // scroll up
        camera.fov -= 1.0f * zoom_speed;
        camera.fov = glm::clamp(camera.fov, 1.0f, 90.0f);
    }
    else if (button == 4 && state == GLUT_DOWN) {  // scroll down
        camera.fov += 1.0f * zoom_speed;
        camera.fov = glm::clamp(camera.fov, 1.0f, 90.0f);
    }
}

void Idle(void) {}

void Entry(int state) {
    DefaultEntryCallback(state);
}

void Motion(int x, int y) {}

void PassiveMotion(int x, int y) {
    // x, y are measured in pixels in screen space, with the origin at the top-left corner
    // but OpenGL uses a world coordinate system with the origin at the bottom-left corner
    int x_offset = x - last_mouse_x;
    int y_offset = last_mouse_y - y;  // must invert y coordinate to flip the upside down

    // cache last motion
    last_mouse_x = x;
    last_mouse_y = y;

    // update camera based on mouse movements
    camera.euler_y += x_offset * sensitivity;
    camera.euler_x += y_offset * sensitivity;
    camera.euler_x = glm::clamp(camera.euler_x, -90.0f, 90.0f);  // clamp vertical rotation

    camera.forward = glm::normalize(
        glm::vec3(
            cos(glm::radians(camera.euler_y)) * cos(glm::radians(camera.euler_x)),
            sin(glm::radians(camera.euler_x)),
            sin(glm::radians(camera.euler_y)) * cos(glm::radians(camera.euler_x))
        )
    );

    camera.right = glm::normalize(glm::cross(camera.forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.up = glm::normalize(glm::cross(camera.right, camera.forward));
}

void Cleanup() {
    glDeleteTextures(1, &base);
    glDeleteTextures(1, &overlay);
    glDeleteProgram(PO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
    glDeleteVertexArrays(1, &VAO);
}