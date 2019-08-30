#pragma once

#include <functional>
#include <type_traits>

namespace stdx {
    template <typename T>
    class ScopeExit;
}

namespace stdx::details {
    using EmptyScope = ScopeExit<void()>;

    template <typename T>
    using TypeIdentity = T;

    template <typename T>
    using RemoveCVRef = std::remove_reference_t<std::remove_cv_t<T>>;

    template <typename... Ts>
    struct TypePack { };

    template <typename Base, typename U, bool = std::is_nothrow_constructible_v<Base, std::in_place_t, U>>
    struct ScopeConstructible {
        using Type = decltype(std::as_const(std::declval<U&>()));

        static constexpr bool Enable = std::is_constructible_v<Base, std::in_place_t, Type>;
        static constexpr bool NoExcept = std::is_nothrow_constructible_v<Base, std::in_place_t, Type>;
    };

    template <typename Base, typename U>
    struct ScopeConstructible<Base, U, true> {
        using Type = TypeIdentity<U>;

        static constexpr bool Enable = true;
        static constexpr bool NoExcept = true;
    };

    template <typename Box, typename U, bool = std::is_nothrow_constructible_v<Box, std::in_place_t, U>>
    struct ResourceConstructible {
        using Type = decltype(std::as_const(std::declval<U&>()));

        static constexpr bool Enable = std::is_constructible_v<Box, std::in_place_t, Type, EmptyScope>;
        static constexpr bool NoExcept = std::is_nothrow_constructible_v<Box, std::in_place_t, Type, EmptyScope>;
    };

    template <typename Base, typename U>
    struct ResourceConstructible<Base, U, true> {
        using Type = TypeIdentity<U>;

        static constexpr bool Enable = true;
        static constexpr bool NoExcept = true;
    };
}