#version 460

layout(std140, binding = 0) uniform Camera {
    vec3 position;
    vec3 direction;
    mat4 view;
    mat4 perspective;
} camera;

layout(std140, binding = 1) uniform DLT {
    vec3 color;
    vec3 direction;
    float intensity;
} DL;

layout(std140, binding = 2) uniform PLT {
    vec3 color;
    vec3 position;
    float intensity;
    float linear;
    float quadratic;
    float range;
} PL;

layout(std140, binding = 3) uniform SLT {
    vec3 color;
    vec3 position;
    vec3 direction;
    float intensity;
    float inner_cosine;
    float outer_cosine;
    float range;
} SL;

layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform int material_id;

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 uv2;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout(location = 0) out __ {
    out vec3 _position;
    out vec3 _normal;
    out vec2 _uv;
    out vec2 _uv2;
    out vec3 _tangent;
    out vec3 _bitangent;
};

void main() {
    // model space -> world space
    _position = vec3(transform * vec4(position, 1.0));
    _normal = normalize(vec3(transform * vec4(normal, 0.0)));
    _uv = uv;
    _uv2 = uv2;
    _tangent = tangent;
    _bitangent = bitangent;

    gl_Position = camera.perspective * camera.view * transform * vec4(position, 1.0);
}

# endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in __ {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _bitangent;
};

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D emission;
layout(binding = 3) uniform sampler2D metalness;
layout(binding = 4) uniform sampler2D roughness;
layout(binding = 5) uniform sampler2D AO;

void main() {
    vec3 V = normalize(camera.position - _position);  // view direction vector

    // directional light
    vec3 L1 = normalize(-DL.direction);  // light direction vector
    vec3 R1 = reflect(-L1, _normal);        // reflection vector

    // point light
    vec3 L2 = normalize(PL.position - _position);  // light direction vector
    vec3 R2 = reflect(-L2, _normal);                  // reflection vector

    vec3 diff = max(dot(_normal, L1), 0.0) * DLColor(0) +
                max(dot(_normal, L2), 0.0) * PLColor(0, _position) +
                SLColor(0, _position);

    vec3 spec = pow(max(dot(V, R1), 0.0), 128) * DLColor(0) +
                pow(max(dot(V, R2), 0.0), 128) * PLColor(0, _position);

    if (material_id == 4) {
        color = vec4(texture(albedo, _uv).rgb + diff * texture(metalness, _uv).rgb + spec * texture(roughness, _uv).rgb, 1);
    }
    else if (material_id == 5) {
        color = texture(albedo, _uv);
    }
}

# endif
