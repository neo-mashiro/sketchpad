## Place your GLSL shader sources in this folder

Our renderer and material system are based on a few assumptions of the shader code, make sure to follow these conventions when writing your GLSL shaders (see the sample shader).

### Binding textures

The material API enforces the users to specify a texture unit when pushing a textures asset reference, so we don't need to set sampler uniforms from C++ code. In order for this to work, samplers must be directly bound to texture units in GLSL, this is easier to work with and less error-prone as opposed to setting sampler uniforms manually.

For example, on C++'s side, we know the specular map goes to texture unit 0, emission map goes to 1, etc.
```c++
auto& m = entity.GetComponent<Material>();
m.SetShader(sample);
m.SetTexture(0, specular);
m.SetTexture(1, emission);
```

so in the fragment shader, we need to specify the binding points to match exactly:
```GLSL
#version 460
...
layout(binding = 0) uniform sampler2D specular;
layout(binding = 1) uniform sampler2D emission;
...
```

In the future, I may also add support for bindless textures, but for now we need to follow this rule.

### Loose uniforms

Users can declare as many loose uniforms as they want, but similar to the samplers, they must explicitly specify the location for every uniform. This is the best way to organize uniforms, it not only makes our code cleaner, but also saves us a lot of work. The point is that, we don't want to query uniform locations or use a cache, which can introduce quite a bit of overhead. As a convention, `location = 0` should always be reserved to the model matrix of the transform, which will be automatically uploaded to the shader by the renderer every frame.

### Using uniform blocks

On initialization, the renderer parses the sample shader to determine the structures of all uniform buffers, and is responsible for maintaining the UBOs internally thereafter. These UBOs contain pretty much all the global data we need in our shader code, so we can use them as we need without setting any uniform values. In order for this to work, users need to copy these blocks from the sample shader to their own shader code, so
that every shader is guaranteed to see a standardized list of global data. Users have the option to use only part of that data by copying a subset of them, but then they have to make sure the binding points are correctly mapped to the blocks they need.

For this demo, we are only going to use uniform blocks with the __std140__ layout. Individual uniform members in a block will never be optimized out by the GLSL compiler even if it's not being used, so don't worry about if some of them would ever become inactive. In the sample shader, I have hardcoded 13 uniform blocks upfront: one block for the main camera, followed by 4 directional lights, 4 point lights and 4 spotlights, the binding points of which are numbered from 0, 1-4, 5-8, 9-12, respectively. Within the same type of blocks, each one is listed in the order as they are created in the scene, so if you created 2 entities that has a spotlight component, first A then B, then A's data will be available in the block whose binding point is 9, while B's is in 10. (note that a block index is not the same as a binding point, we won't use block indices whatsoever)

Users are free to add more uniform blocks as they see fit, as long as the total number of blocks does not exceed `GL_MAX_UNIFORM_BUFFER_BINDINGS`. For consistency, the binding points should be numbered from 13 in sequential order, also make sure that they are added to all shaders including the sample. When a new uniform block is declared in GLSL, the renderer will automatically create a corresponding new uniform buffer object (UBO), which you can retrieve from the `std::map UBOs` using the binding point as key. It will also compute the __std140__ aligned offset of each uniform for you, as well as the buffer size in memory, but you must tell the renderer what data should be written into that UBO, which can be done by updating the `Renderer::DrawScene()` API in the renderer script.
