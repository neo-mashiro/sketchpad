#include "pch.h"

#include "components/tag.h"

namespace components {

    ETag operator|(ETag lhs, ETag rhs) {
        return static_cast<ETag>(
            static_cast<std::underlying_type<ETag>::type>(lhs) |
            static_cast<std::underlying_type<ETag>::type>(rhs)
        );
    }

    ETag operator&(ETag lhs, ETag rhs) {
        return static_cast<ETag>(
            static_cast<std::underlying_type<ETag>::type>(lhs) &
            static_cast<std::underlying_type<ETag>::type>(rhs)
        );
    }

}
