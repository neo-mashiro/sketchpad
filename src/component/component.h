#pragma once

#include "utils/ext.h"
#include "utils/math.h"

namespace component {

    /* this is the base class for all components in the entity-component system, each component
       is associated with a universal unique instance id (UUID), and can be enabled or disabled
       based on the needs. Components are the control modules that define an entity's behavior,
       our concept of "component" is identical to the component class being used in Unity.

       every component class has to be move constructible and move assignable, this is required
       by the ECS system for both safety and robustness. Components are expected to encapsulate
       global objects and states in OpenGL, which are dependent on a valid OpenGL context, thus
       the system needs to ensure that a destructor call will not ruin the global state even if
       components are managing move-only resources directly, such as GPU buffers.

       # guidelines

       > the base class follows the rule of five, derived classes follow the rule of zero
       > a component should not manage OpenGL resources directly (raw pointers, references...)
       > a component should be treated as a wrapper (container) on top of OpenGL resources
       > a component's data members should use trivially copyable types whenever possible
       > a component should ideally be both copyable and move-constructible
       > a component that's fully owned by the ECS registry must not be used in other scope
    */

    class Component {
      protected:
        uint64_t uuid;  // universal unique instance id (in case of collision, go buy a lottery)
        bool enabled;

      public:
        Component() : uuid(utils::math::RandomUInt64()), enabled(true) {}
        virtual ~Component() {}

        Component(const Component&) = default;
        Component& operator=(const Component&) = default;
        Component(Component&& other) noexcept = default;
        Component& operator=(Component&& other) noexcept = default;

        uint64_t UUID()          const { return uuid; }
        explicit operator bool() const { return enabled; }

        void Enable()  { enabled = true; }
        void Disable() { enabled = false; }
    };

    enum class ETag : uint16_t {  // allow up to 16 tags
        Untagged   = 1 << 0,
        Static     = 1 << 1,
        MainCamera = 1 << 2,
        WorldPlane = 1 << 3,
        Skybox     = 1 << 4,
        Water      = 1 << 5,
        Particle   = 1 << 6
    };

    // DEFINE_ENUM_FLAG_OPERATORS(ETag)  // C-style built-in solution for bitfields, don't use

    inline constexpr ETag operator|(ETag a, ETag b) {
        return static_cast<ETag>(utils::to_integral(a) | utils::to_integral(b));
    }

    inline constexpr ETag operator&(ETag a, ETag b) {
        return static_cast<ETag>(utils::to_integral(a) & utils::to_integral(b));
    }

    inline constexpr ETag operator^(ETag a, ETag b) {
        return static_cast<ETag>(utils::to_integral(a) ^ utils::to_integral(b));
    }

    inline constexpr ETag operator~(ETag a) {
        return static_cast<ETag>(~utils::to_integral(a));
    }

    inline ETag& operator|=(ETag& lhs, ETag rhs) { return lhs = lhs | rhs; }
    inline ETag& operator&=(ETag& lhs, ETag rhs) { return lhs = lhs & rhs; }
    inline ETag& operator^=(ETag& lhs, ETag rhs) { return lhs = lhs ^ rhs; }

    /* the tag component is used for identifying entities of certain types, such as the skybox,
       the main camera, static game objects, or those who are candidates for occlusion culling.
       while an entity's name can be any user-defined string, the tag should be restricted to a
       range of enum bitset fields. Users are free to name entities arbitrarily as long as the
       string name is descriptive, but we can't really know what it is, therefore we need a set
       of predefined tags to recognize entities by their properties. Searching on bitset values
       is much cheaper than that on strings, and they allow us to freely overload operators.

       the ETag (entity tag) enum class will be extended as we create more complex scenes, for
       the time being it's primarily used by the renderer to identify entities. ETags represent
       entities' attributes in the form of bit flags, we can test if an entity has an attribute
       using the bitwise "&" operator, or combine them using bitwise "|", "|=" operators, etc.
    */

    class Tag : public Component {
      private:
        ETag tag;

      public:
        explicit Tag(ETag tag) : Component(), tag(tag) {}

        void Add(ETag t) { tag |= t; }
        void Del(ETag t) { tag &= ~t; }

        constexpr bool Contains(ETag t) const {
            return utils::to_integral(tag & t) > 0;
        }
    };

}