/* 
   adapted and refactored based on https://learnopengl.com/ by Joey de Vries

   for humanoid models, animations are stored in the hierarchical node graph. An animation is
   represented by an array of channels, each contains the keyframes data for a unique bone.
   for this part we'll keep using the Assimp library, but there tends to be lots of confusion
   regarding the data logic behind it, for a detailed explanation, see the official docs:
   https://assimp-docs.readthedocs.io/en/master/usage/use_the_lib.html#data-structures

   # terminology

   a node is a basic unit in the hierarchical tree, it contains the local transform matrix, a
   reference to its parent and children nodes, as well as some optional bones/animation data.
   a bone node is a node for which Assimp defines a pair of vertex id and weight, the vertex
   position is then subject to weighted changes by this node. A bone is a node, but a node is
   not necessarily a bone, some nodes do not affect vertices.

   note that bones are defined by Assimp, not by an animation clip, even if Assimp interprets
   a node as a bone, a clip may not have animation data for it. Therefore, a bone can further
   be divided into either dead or alive, dead bones do not have animation data in the clip,
   while living bones are indeed influenced by the clip so that they can affect the vertices.

   a keyframe holds the value of a certain key at a specific timestamp on a timeline, the key
   is either one of position, rotation or scale. A channel contains all keyframes data for a
   unique bone node, every key must have at least 2 slices so that the starting and ending of
   the transition is well-defined. By our definition, it follows that the number of channels
   is less than or equal to the number of bones, which in turn is less than or equal to the
   total number of nodes. A bone does not have to be referenced by a channel (dead bone), but
   a channel can only reference a node that Assimp recognizes as a bone.

   # change of basis

   for clarity, we will not use the term "local" or "global" as they are only meaningful in a
   certain frame of reference. Instead we'll name all matrices as `u2v`, meaning that it will
   transform a vertex from space u to space v.

   model space refers to the space in which vertices are defined when Assimp imports them, by
   default this is the first frame of the animation, which is often the bind pose or T pose.
   node space refers to the local space of that node relative to its parent. For a bone node,
   this is also called the bone space. For a parent, it's the parent's node space.

   matrix `m2n` brings model space (bind pose) -> some node space (could be bone space)
   matrix `n2p` brings some node space -> its parent node space
   matrix `p2m` brings a parent node space -> model space (bind pose)
   matrix `m2w` brings model space -> world space (not used in the model's local scope)

   note that Assimp calls `m2n` the offset matrix, which is confusing.

   there's another: matrix `n2m` converts a vertex from its node space directly to the model
   space, skipping the parent space transform, it is used by bones that are animated (alive).
   for animated bones, keyframes data is absolute and local to the node's parent space, hence
   we should replace the `n2p` matrix with the matrix interpolated from keyframes at runtime.
   
   # transform example

   consider a node C which is a child of B which is a child of A which is the root node.

   - `A.n2p` is the identity matrix since A is root, so node space A = model space
   - `C.n2p` transforms a vertex v in node space C to node space B
   - `B.n2p` transforms a vertex v in node space B to node space A
   - `B.n2p * C.n2p * v` brings v from node space C directly to node space A, the model space
   - `C.n2m = A.n2p * B.n2p * C.n2p = B.n2p * C.n2p` if C is a dead bone, o/w interpolate
   - `C.m2n = inverse(C.n2m)`, this is what Assimp calls the "offset matrix"
   
   # prepare animation

   an animation holds all keyframes data in a vector of channels, indexed by bone id, it's an
   optional asset owned by the model class, all data is static. As mentioned in `model.h`, we
   recommend that animation is baked into the model, this can be done manually using external
   tools such as Unity, Blender or Adobe Mixamo, the purpose of which is to match the list of
   bones in the animation to the model. Due to the variety of animation formats, it's hard to
   reconcile bones in code, users must make sure that the animation and model are compatible.
   in case of multiple animation clips in the file, only the first one will be loaded.

   the preparation of animation asset requires a bit knowledge of character rigging, renaming
   and remapping of bones, inverse kinematics (IK) and so on, we'll not support these as they
   are outside the scope of this demo. There are plenty of tools and plugins relevant on the
   internet, just note that some motions are specific to certain models and cannot be applied
   to other models, and it's not always possible to remap bones to bones when the underlying
   structures are totally different. For example, MMD motions often use a bone structure that
   consists of multiple layers, some bones can overlap, and some are special because there is
   no counterparts in other formats, so it's very hard to retarget them on a FBX model.

   # preprocess animation

   as we defined earlier, a channel can only correspond to a node that Assimp recognizes as a
   bone, and the number of channels must <= the number of bones, unfortunately this is often
   not satisfied when the clip gets imported, we need to clean up the data first.
   
   for some reason, some software forces the artists to create dummy channels for otherwise
   static nodes, on the other hand, Assimp can remove bones as a result of optimization, so
   the 2 sides rarely agree. Assimp knows that fully weight-based skinning is very expensive
   so it won't identify all channels as bones, if a bone is empty or has too little impact on
   vertices, it's often ruled out. When we apply motions to a model, the model is seen as the
   ground truth, channels that don't match a node or bone should be dropped.

   # FK (forward-kinematics) vs IK (inverse-kinematics)

   our animator does not have an IK solver, so it can only handle FK bones. If we try to play
   animations that make use of IK, it will appear very weird as the hip bone may be rotating.
   most 3D software like Blender, Unity and even the Mixamo website have built-in IK support,
   these are implemented using the FABRIK algorithm or alike, but for this demo, we will not
   dive into the IK system since it's outside the scope of rendering.

   to import and use an animation, make sure it's driven by FK only. If the original armature
   also contains IK bones, we must convert it to a new armature that uses FK bones only. This
   can be done using a Blender addon called `Landon`, the convertion is as easy as running a
   line of code in the Blender Python Console: `bpy.ops.rigging.iktofk()` (import bpy first)

   # animator controller

   the animator component is an actor role that acts upon a model, at runtime, it updates the
   bone transforms every frame so the vertex shader can use them to do animation. As an actor
   it does not hold any data on its own, it's just a brain that directs the traffic.
   
   just like how the camera is tied to the transform, the animator is also tied to the model
   component, but since ECS pool fully owns the model component, pointer stability cannot be
   ensured. As a result, we can't rely on a pointer to model (which is volatile) for the sake
   of establishing this connection, instead, we need to pass `const Model&` as a parameter at
   runtime to access the model's correct hierarchy data and animation.

   on update, the animator iterates over the nodes vector to calculate the bone transform for
   each node, since it's sorted in the order of hierarchy, matrices can be easily chained as
   we loop through the vector. A bone transform is simply `p2m * n2p * m2n`, for bones that
   are alive, `n2p` is replaced by a matrix interpolated from keyframes at runtime, which is
   already local to the parent space.

   one thing to be aware of is that 3D software tend to use different units of measurement,
   for example, Blender by default uses centimeter as the metric for positions and scales, so
   the `n2p` matrix of the root node is in fact not identity, but identity scaled by 0.01. As
   implied here, although the root node is already in model space from the very beginning, it
   is measured in units of the original software, so we must scale it to match our own units.
   this is done by multiplying the inverse of root node's `n2p` matrix, which only converts
   the units, without change of basis, some people also call it the global inverse matrix.
*/

#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <glm/glm.hpp>
#include "component/component.h"
#include "component/model.h"

namespace component {

    class Channel {
      private:
        template<typename TKey, int Key>  // strong typedefs
        struct Frame {
            Frame(TKey value, float time) : value(value), timestamp(time) {}
            TKey value;
            float timestamp;
        };

        using FT = Frame<glm::vec3, 1>;  // position frames
        using FR = Frame<glm::quat, 2>;  // rotation frames
        using FS = Frame<glm::vec3, 3>;  // scale frames

      private:
        std::vector<FT> positions;
        std::vector<FR> rotations;
        std::vector<FS> scales;

        template<typename TFrame>
        std::tuple<int, int> GetFrameIndex(const std::vector<TFrame>& frames, float time) const;

      public:
        std::string name;
        int bone_id = -1;

        Channel() : bone_id(-1) {}
        Channel(aiNodeAnim* ai_channel, const std::string& name, int id, float duration);
        Channel(Channel&& other) = default;
        Channel& operator=(Channel&& other) = default;

        glm::mat4 Interpolate(float time) const;
    };

    class Model;

    class Animation {
      private:
        unsigned int n_channels;
        std::vector<Channel> channels;  // indexed by bone id
        friend class Animator;

      public:
        std::string name;
        float duration;
        float speed;

        Animation(const aiScene* ai_scene, Model* model);
    };

    class Animator : public Component {
      public:
        float current_time;
        std::vector<glm::mat4> bone_transforms;

        Animator(Model* model);

        void Update(Model& model, float deltatime);
        void Reset(Model* model);
    };

}
