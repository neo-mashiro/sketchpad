#pragma once

namespace components {

    // the tag component is intended for filtering entities of specific types, such as the skybox,
    // the main camera, static game objects, or entities that are candidates for occlusion culling,
    // shadow culling, etc. While an entity's name can be any user-defined string, the tag should
    // be restricted to a range of values of a enum bitset because we don't want to rely on a magic
    // string. In the case of a string tag, users are free to name it arbitrarily, so a camera may
    // have a tag called "skybox", and there's no way we can know what it really is, put aside that
    // searching on strings is an expensive operation.

    // without a well maintained tag list, it's really hard for the system in ECS (in our case, the
    // renderer) to distinguish entities from each other, for example, given an entity with a mesh,
    // a transform and a material component, we can't tell if it's just a regular object or a global
    // skybox which needs to be rendered at last to save draw calls. This is where the tag system
    // kicks in to clear up the ambiguity. As a side note, tags are very important in Unity DOTS.
    // another big reason is that, our scene renderer is not a game engine, so I don't want to add
    // native scripting support (which means users can write client code in C# or Lua and attach the
    // script to an entity, like the way we use Unity), so the only way to allow for richer behavior
    // on our entities is to extend the ECS system based on specialized tags. That said, it should
    // be quite easy to embed Lua in C++ using the Lua virtual machine, to make simple AI scripts.

    // feel free to extend the ETag (entity tag) enum class as you create more complex scenes.
    // to extend the system on how to filter and use these tags, see the `Renderer::DrawScene()` API.

    enum class ETag : char {
        Untagged   = 1 << 0,
        Static     = 1 << 1,
        MainCamera = 1 << 2,
        Skybox     = 1 << 3,
        Wireframe  = 1 << 4   // entity to be drawn in wireframe mode (use geometry shader)
    };

    ETag operator|(ETag lhs, ETag rhs);
    ETag operator&(ETag lhs, ETag rhs);

    struct Tag {
        ETag tag = ETag::Untagged;

        Tag() = default;
        Tag(ETag tag) : tag(tag) {}
        Tag(const Tag&) = default;
    };

}
