# GLSL Template

```GLSL
#version 460 core

in type in_variable;          // snake_case
in type in_variable;          // snake_case
in type _sharedInVariable;    // _camelCase (received from the previous stage)

out type OutVariable;         // PascalCase (to be consumed by this stage)
out type _sharedOutVariable;  // _camelCase (pass to the next stage as input)

uniform type uniform_var;     // snake_case
uniform type uniform_var;     // snake_case

const type const_var = 3.5;   // snake_case

type FunctionName() {  // PascalCase
    type local_var = builtinfunction(1, 2) * const_var;  // snake_case
}

void main() {
    type local_var1 = ...;     // process data
    type local_var2 = ...;     // process data

    _sharedOutVariable = ...;  // shared output
    OutVariable = ...;         // local output

    gl_Position = ...;         // built-in output
    gl_FragDepth = ...;        // built-in output
}
```

# GLSL Conventions

- [x] use spaces instead of tabs

- [x] whenever possible, explicitly specify layout locations
```GLSL
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 0) out vec4 Color;
```
