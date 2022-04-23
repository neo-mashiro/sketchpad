#version 460 core
#pragma optimize(off)

// a simple shader for generating omni-directional shadow maps (cubemap)

#include "../core/renderer_input.glsl"

////////////////////////////////////////////////////////////////////////////////

#ifdef vertex_shader

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 uv2;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 binormal;
layout(location = 6) in ivec4 bone_id;
layout(location = 7) in vec4 bone_wt;

layout(location = 100) uniform mat4 bone_transform[150];  // up to 150 bones

mat4 CalcBoneTransform() {
    mat4 T = mat4(0.0);
    for (uint i = 0; i < 4; ++i) {
        if (bone_id[i] >= 0) {
            T += (bone_transform[bone_id[i]] * bone_wt[i]);
        }
    }
    // if (length(T[0]) == 0) { return mat4(1.0); }
    return T;
}

void main() {
    bool suzune = self.material_id == 12 || (self.material_id >= 14 && self.material_id <= 18);
    mat4 BT = suzune ? CalcBoneTransform() : mat4(1.0);
    gl_Position = self.transform * BT * vec4(position, 1.0);  // keep in world space
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef geometry_shader

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) out vec4 world_position;
layout(location = 250) uniform mat4 light_transform[12];

void main() {
    // render target is a cubemap texture, so we render 6 times, once for each face
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face;
        uint index = (rdr_in.shadow_index - 1) * 6 + uint(face);

        for (uint i = 0; i < 3; ++i) {
            world_position = gl_in[i].gl_Position;
            gl_Position = light_transform[index] * world_position;  // project into light frustum
            EmitVertex();
        }

        EndPrimitive();
    }
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef fragment_shader

layout(location = 0) in vec4 world_position;

layout(std140, binding = 1) uniform PL {
    vec4  color[2];
    vec4  position[2];
    float intensity[2];
    float linear[2];
    float quadratic[2];
    float range[2];
} pl;

/* note that if we manually write to the depth buffer, early depth test will no longer work.
   besides, all code paths must write to the depth buffer, so if we write `gl_FragDepth` in
   some `if` branch, we must also update the `else` branch, o/w the behavior is undefined.
   since there is a large performance penalty for updating `gl_FragDepth`, we should try to
   restrict this to a single shader (separate from shading), and update all execution paths.

   early z test is a hardware optimization that can discard pixel before the fragment stage,
   with early z, pixels occluded by others won't go through the fragment shader code at all,
   so we can save a lot of shader invocations. If early z is not possible, depth testing is
   still performed, just after the fragment shader. As long as depth testing is enabled, we
   would never need to sort depth values, so `gl_FragDepth = min(gl_FragDepth, curr_depth)`
   doesn't really make sense since depth values that are not the closest will be dropped.

   for OpenGL 4.2 and above, we can also enforce early z by redeclaring the output variable
   like so: `layout(depth_<condition>) out float gl_FragDepth`, but if the depth comparison
   function `glDepthFunc()` is `GL_LESS` or `GL_LEQUAL`, condition must be `depth_greater`,
   which is not true as we're writing linear depth values (less than the non-linear depth).
   also be aware that the initial value of `gl_FragDepth` is undefined, it is not the same
   as the value used to clear the depth buffer, but can be anything between 0 and 1, so we
   cannot rely on it in our computation, for example, `min(gl_FragDepth, depth)` will cause
   undefined behavior or always return 0.

   due to the drawbacks of updating `gl_FragDepth`, it is sometimes more convenient to just
   write depth into a regular color texture, this is possible by packing 32-bit floats into
   vec4s (RGBA format), each component of which has 8 bits and 1/256 precision, then unpack
   vec4 into 32-bit float when we need the depth. This is mostly used in WebGL to allow for
   high precision data formats (32-bit is often a luxury for browsers and mobiles), but can
   also be used as a solution to work around the side effect of writing to the depth buffer.
   it's not really necessary here in our demo, but is definitely something worth mentioning.

   another thing to watch out for is transparency: depending on the type of objects, we may
   or may not want to cast shadows from non-opaque pixels, for instance, a semi-transparent
   ball should occlude some incoming lights, but a grass quad texture must not cast shadows
   from pixels whose alpha channel is 0. If transparency is involved, we need to access the
   alpha channel using extra uniforms, then the depth values computation should either take
   alpha into account, or simply discard the pixels if alpha is 0. Semi-transparent shadows
   is very hard for a rasterizer, but is much easier to implement in a ray tracer.

   lastly, we should not render any light source in the shadow pass as they are bloomed and
   will not be in shadows, but it's also fine if you have to draw them. However, the skybox
   must not be rendered because it's a unit cube whose geometry has to be invisible, in the
   skybox shader we are using a little hack to fake its depth so that it appears infinitely
   distant, but if we draw it in the shadow pass, our shadow map will see its real geometry,
   so in the rendering pass there will be a box-shaped ghost shadow around the world origin
   on the floor, and nearby objects occluded by the cube will sit in shadows weirdly.
*/

void main() {
    uint pl_index = rdr_in.shadow_index - 1;
    gl_FragDepth = distance(world_position.xyz, pl.position[pl_index].xyz) / rdr_in.far_clip;
}

#endif
