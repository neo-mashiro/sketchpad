#ifdef geometry_shader

/* this geometry shader passes through the vertex data untouched to the fragment shader
   along the way, it will compute the edge distance of each vertex to define wireframes
   normally, perspective division and viewport transforms are auto done by the hardware
   but since we are directly working in screen space here, we need to do them ourselves
   recall the ISO procedure of transforming a vertex from local space to viewport space

   1. matrix `M` transforms a vertex from local space -> world space (model matrix)
   2. matrix `V` transforms a vertex from world space -> view/camera space (view matrix)
   3. matrix `P` transforms a vertex from camera space -> clip space (projection matrix)
   4. vertex position transformed by `MVP` is assigned to `gl_Position`, thus in clip space
   5. driver performs perspective division /w, which brings clip space -> NDC space [-1, 1]
   6. driver performs viewport transform, which brings NDC space -> screen space (1600x900)
   x. note that in certain context, "screen/window" space is not the same as viewport space
*/

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in _vtx {
    in vec3 _position;
    in vec3 _normal;
    in vec2 _uv;
    in vec2 _uv2;
    in vec3 _tangent;
    in vec3 _binormal;
} vtx[];

layout(location = 0) out _vtx {
    smooth out vec3 _position;
    smooth out vec3 _normal;
    smooth out vec2 _uv;
    smooth out vec2 _uv2;
    smooth out vec3 _tangent;
    smooth out vec3 _binormal;
    noperspective out vec3 _distance;  // distance to the 3 edges of the triangle
};

vec3 ComputeTriangleHeight(const vec2 p0, const vec2 p1, const vec2 p2) {
    // triangle sides
    float a = length(p1 - p2);
    float b = length(p2 - p0);
    float c = length(p1 - p0);

    // triangle angles
    float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
    float beta  = acos((a * a + c * c - b * b) / (2.0 * a * c));

    // triangle height (altitude) on each side
    float ha = abs(c * sin(beta));
    float hb = abs(c * sin(alpha));
    float hc = abs(b * sin(alpha));

    return vec3(ha, hb, hc);
}

const mat4 viewport = mat4(
    vec4(800, 0.0, 0.0, 0.0),  // NDC space x ~ [-1,1] -> scale by viewport w -> x ~ [-800,800]
    vec4(0.0, 450, 0.0, 0.0),  // NDC space y ~ [-1,1] -> scale by viewport h -> y ~ [-450,450]
    vec4(0.0, 0.0, 1.0, 0.0),  // keep scale on the z axis (depth buffer)
    vec4(800, 450, 0.0, 1.0)   // translate by 800/450 on x/y axis -> x ~ [0,1600], y ~ [0,900]
);

void main() {
    // compute the 3 vertices of the triangle in screen space
    vec2 p0 = vec2(viewport * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));
    vec2 p1 = vec2(viewport * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));
    vec2 p2 = vec2(viewport * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));

    vec3 h = ComputeTriangleHeight(p0, p1, p2);

    for (uint i = 0; i < 3; i++) {
        _position = vtx[i]._position;
        _normal   = vtx[i]._normal;
        _uv       = vtx[i]._uv;
        _uv2      = vtx[i]._uv2;
        _tangent  = vtx[i]._tangent;
        _binormal = vtx[i]._binormal;

        switch (i) {
            case 0: _distance = vec3(h.x, 0, 0); break;
            case 1: _distance = vec3(0, h.y, 0); break;
            case 2: _distance = vec3(0, 0, h.z); break;
        }

        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}

#endif