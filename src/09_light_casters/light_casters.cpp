#include "define.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define __DIR__ ParseDir(__FILE__)

Window window{};
Camera camera{};
FrameCounter frame_counter{};
MouseState mouse_state{};
KeyState key_state{};

glm::mat4 M, V, P;

// sphere
struct Material {
    GLuint diffuse_map;
    GLuint specular_map;
    GLuint emission_map;
    float shininess;
} material;

GLuint VAO, VBO, IBO, PO;
std::vector<glm::vec3> positions;
std::vector<glm::vec2> uvs;
std::vector<glm::vec3> normals;
std::vector<float> vertices;
std::vector<unsigned int> indices;

// light cube
struct Light {
    glm::vec3 source;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
} light;

GLuint LVAO, LVBO, LPO;
std::vector<float> light_vertices;

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

void CreateLightCube() {
    light_vertices = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
    };
}

int LoadTexture(const std::string& file_path) {
    std::string file_extension = file_path.substr(file_path.rfind(".") + 1);
    printf("Loading texture %s, detected file extension = %s\n", file_path.c_str(), file_extension.c_str());

    // attempt to read image binary data
    if (file_extension == "png") {
        stbi_set_flip_vertically_on_load(true);
    }

    int width, height, n_channels;
    unsigned char* buffer = stbi_load(file_path.c_str(), &width, &height, &n_channels, 0);

    if (!buffer) {
        std::cerr << "Failed to load texture: " << file_path << std::endl;
        stbi_image_free(buffer);
        return -1;
    }

    // now image data has been loaded into the buffer, let's setup texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);  // bind

    if (n_channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    }
    else if (n_channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);  // unbind

    return (int)texture_id;
}

void SetupWindow() {
    window.title = "Mixture Lights";
    SetupDefaultWindow(window);
}

void Init() {
    srand((unsigned)time(0));  // seed for random numbers

    // setup sphere
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

    // setup light cube
    CreateLightCube();
    glGenVertexArrays(1, &LVAO);
    glBindVertexArray(LVAO);

    glGenBuffers(1, &LVBO);
    glBindBuffer(GL_ARRAY_BUFFER, LVBO);
    glBufferData(GL_ARRAY_BUFFER, light_vertices.size() * sizeof(float), &light_vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);  // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glBindVertexArray(0);
    
    // create shaders
    PO = CreateShader(__DIR__);
    LPO = CreateShader(__DIR__ + "light\\");

    // load texture maps
    material.diffuse_map = (GLuint)LoadTexture(__DIR__ + "textures\\Diffuse.jpg");
    material.specular_map = (GLuint)LoadTexture(__DIR__ + "textures\\Specular.jpg");
    material.emission_map = (GLuint)LoadTexture(__DIR__ + "textures\\Emission.jpg");
    material.shininess = 64.0f;

    // setup material uniforms
    glUseProgram(PO);
    glUniform1i(glGetUniformLocation(PO, "material.diffuse"), 0);   // bind to texture unit 0
    glUniform1i(glGetUniformLocation(PO, "material.specular"), 1);  // bind to texture unit 1
    glUniform1i(glGetUniformLocation(PO, "material.emission"), 2);  // bind to texture unit 2

    // setup light source
    light.source = glm::vec3(0.0f, 1.0f, 1.5f);
    light.ambient = glm::vec3(1.0f);
    light.diffuse = glm::vec3(1.0f);
    light.specular = glm::vec3(1.0f);

    // model view projection
    camera.position = glm::vec3(0.0f, 1.0f, 2.5f);
    P = glm::perspective(glm::radians(camera.fov), window.aspect_ratio, 0.1f, 100.0f);
    V = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
    M = glm::mat4(1.0f);

    // initial mouse position
    mouse_state.last_x = window.width / 2;
    mouse_state.last_y = window.height / 2;

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

void SmoothKeyControl() {
    float movement = camera.speed * frame_counter.delta_time;
    float ground_level = camera.position.y;

    if (key_state.up) {
        camera.position += camera.forward * movement;
    }
    if (key_state.down) {
        camera.position -= camera.forward * movement;
    }
    if (key_state.left) {
        camera.position -= camera.right * movement;
    }
    if (key_state.right) {
        camera.position += camera.right * movement;
    }

    camera.position.y = ground_level;  // snap to the ground
}

void Display() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        frame_counter.this_frame = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
        frame_counter.delta_time = frame_counter.this_frame - frame_counter.last_frame;
        frame_counter.last_frame = frame_counter.this_frame;

        SmoothKeyControl();

        P = glm::perspective(glm::radians(camera.fov), window.aspect_ratio, 0.1f, 100.0f);
        V = glm::lookAt(camera.position, camera.position + camera.forward, camera.up);
        M = glm::mat4(1.0f);

        // rotate the light source each frame
        light.source = glm::vec3(
            1.5f * sin(frame_counter.this_frame * 1.5f), 1.0f, 1.5f * cos(frame_counter.this_frame * 1.5f)
        );
    }

    glUseProgram(PO);
    glBindVertexArray(VAO);
    glEnable(GL_CULL_FACE);

    {
        glUniformMatrix4fv(glGetUniformLocation(PO, "u_model"), 1, GL_FALSE, glm::value_ptr(M));
        glUniformMatrix4fv(glGetUniformLocation(PO, "u_view"), 1, GL_FALSE, glm::value_ptr(V));
        glUniformMatrix4fv(glGetUniformLocation(PO, "u_projection"), 1, GL_FALSE, glm::value_ptr(P));

        glUniform3fv(glGetUniformLocation(PO, "camera_position"), 1, &camera.position[0]);

        glUniform3fv(glGetUniformLocation(PO, "light.source"), 1, &light.source[0]);
        glUniform3fv(glGetUniformLocation(PO, "light.ambient"), 1, &light.ambient[0]);
        glUniform3fv(glGetUniformLocation(PO, "light.diffuse"), 1, &light.diffuse[0]);
        glUniform3fv(glGetUniformLocation(PO, "light.specular"), 1, &light.specular[0]);

        glUniform1f(glGetUniformLocation(PO, "material.shininess"), material.shininess);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuse_map);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.specular_map);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material.emission_map);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glUseProgram(LPO);
    glBindVertexArray(LVAO);
    glDisable(GL_CULL_FACE);

    {
        M = glm::translate(M, light.source);
        M = glm::scale(M, glm::vec3(0.05f));
        glUniformMatrix4fv(glGetUniformLocation(LPO, "u_mvp"), 1, GL_FALSE, glm::value_ptr(P * V * M));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glutSwapBuffers();
    glutPostRedisplay();
}

void Reshape(int width, int height) {
    DefaultReshapeCallback(window, width, height);
}

void Keyboard(unsigned char key, int x, int y) {
    DefaultKeyboardCallback(key, x, y);
}

void Special(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: key_state.up = true; break;
        case GLUT_KEY_DOWN: key_state.down = true; break;
        case GLUT_KEY_LEFT: key_state.left = true; break;
        case GLUT_KEY_RIGHT: key_state.right = true; break;
    }
}

void SpecialUp(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: key_state.up = false; break;
        case GLUT_KEY_DOWN: key_state.down = false; break;
        case GLUT_KEY_LEFT: key_state.left = false; break;
        case GLUT_KEY_RIGHT: key_state.right = false; break;
    }
}

void Mouse(int button, int state, int x, int y) {
    // when the left button is clicked, toggle light mode (weak/strong)
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        light.ambient = glm::length(light.ambient) >= 0.999f ? glm::vec3(0.2f, 0.2f, 0.2f) : glm::vec3(1.0f);
        light.diffuse = glm::length(light.diffuse) >= 0.999f ? glm::vec3(0.5f, 0.5f, 0.5f) : glm::vec3(1.0f);
    }

    // in freeglut, each scroll wheel event is reported as a button click
    if (button == 3 && state == GLUT_DOWN) {  // scroll up
        camera.fov -= 1.0f * mouse_state.zoom_speed;
        camera.fov = glm::clamp(camera.fov, 1.0f, 90.0f);
    }
    else if (button == 4 && state == GLUT_DOWN) {  // scroll down
        camera.fov += 1.0f * mouse_state.zoom_speed;
        camera.fov = glm::clamp(camera.fov, 1.0f, 90.0f);
    }
}

void Idle(void) {}

void Entry(int state) {
    DefaultEntryCallback(state);
}

void Motion(int x, int y) {}

void PassiveMotion(int x, int y) {
    int x_offset = x - mouse_state.last_x;
    int y_offset = mouse_state.last_y - y;  // must invert y coordinate to flip the upside down

    // cache last motion
    mouse_state.last_x = x;
    mouse_state.last_y = y;

    // update camera based on mouse movements
    camera.euler_y += x_offset * mouse_state.sensitivity;
    camera.euler_x += y_offset * mouse_state.sensitivity;
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
    glDeleteTextures(1, &material.diffuse_map);
    glDeleteTextures(1, &material.specular_map);
    glDeleteTextures(1, &material.emission_map);
    glDeleteProgram(PO);
    glDeleteProgram(LPO);
    glDeleteBuffers(1, &IBO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &LVBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &LVAO);
}