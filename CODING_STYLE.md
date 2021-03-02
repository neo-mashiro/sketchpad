# GLSL Template

To be consistent with OpenGL, use `snake_case` for all variables, and `PascalCase` for all custom functions.

```GLSL
#version 460 core
#extension all : disable

in type i_var;           // input variable
in type i_var;           // input variable
in type s_var;           // shared input variable (received from the previous stage)

out type o_var;          // output variable (to be consumed by this stage)
out type s_var;          // shared output variable (pass to the next stage as input)

uniform type u_var;      // uniform
uniform type u_var;      // uniform

const type c_var = 3.5;  // constant

type FunctionInPascalCase() {
    type var = builtinfunction(1, 2) * c_var;  // local variable
}

void main() {
    type var = ...;      // process data
    type var = ...;      // process data

    s_var = ...;         // shared output
    o_var = ...;         // output

    gl_Position = ...;   // built-in output
    gl_FragDepth = ...;  // built-in output
}
```

# GLSL Conventions

- [x] use spaces instead of tabs

- [x] whenever possible, explicitly specify layout locations
```GLSL
layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 0) out vec4 o_color;
```
