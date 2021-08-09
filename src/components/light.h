//#pragma once
//
//#include <string>
//#include <GL/glew.h>
//
//namespace scene {
//
//
//    
//    light = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) };
//
//    light_shader->Bind();
//    {
//        static const auto initial_transform = light_cube->M;
//        if (rotate_light) {
//            float delta_x = radius * sin(rotation_time * rotate_speed);
//            float delta_z = radius * (cos(rotation_time * rotate_speed) - 1.0f);
//            rotation_time += Clock::delta_time;
//            light_cube->M = glm::translate(initial_transform, glm::vec3(delta_x, 0.0f, delta_z));
//        }
//        light_shader->SetMat4("u_MVP", P * V * light_cube->M);
//        light_cube->Draw(*light_shader);
//    }
//    light_shader->Unbind();
//
//    enum class LightType { Directional, Point, Spotlight };
//
//    class Light {
//      private:
//        void LoadTexture() const;
//        void LoadSkybox() const;
//        void SetWrapMode() const;
//        void SetFilterMode(bool anisotropic) const;
//
//      public:
//        LightType type;
//        Mesh mesh;
//        Shader shader;
//
//        Light(LightType type, TexMap type, const std::string& path, bool anisotropic = false);
//        ~Light();
//
//        // forbid the copying of instances because they encapsulate global OpenGL resources and states.
//        // when that happens, the old instance would probably be destructed and ruin the global states,
//        // so we end up with a copy that points to an OpenGL object that has already been destroyed.
//        // compiler-provided copy constructor and assignment operator only perform shallow copy.
//
//        Light(const Light&) = delete;             // delete the copy constructor
//        Light& operator=(const Light&) = delete;  // delete the assignment operator
//
//        // it may be possible to override the copy constructor and assignment operator to make deep
//        // copies and put certain constraints on the destructor, so as to keep the OpenGL states alive,
//        // but that could be incredibly expensive or even impractical.
//
//        // a better option is to use move semantics:
//        Light(Light&& other) noexcept;             // move constructor
//        Light& operator=(Light&& other) noexcept;  // move assignment operator
//    };
//
//}
