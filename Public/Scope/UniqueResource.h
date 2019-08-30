#pragma once

#include "Details/BaseUniqueResource.h"
#include "Details/ResourceBox.h"

#if defined(__GNUC__) && !defined(__clang__)
    #define MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D)                                                                          \
        noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R> && std::is_nothrow_constructible_v<std::decay_t<D>, D>)
#else
    #define MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D)
#endif

namespace stdx {
    template <typename R, typename D>
    class [[nodiscard]] UniqueResource final : private details::UniqueResourceMoveAssign<R, D> {
        using Super = details::UniqueResourceMoveAssign<R, D>;
        using typename Super::D1;
        using typename Super::R1;
        using typename Super::TDestruct;
        using typename Super::TResource;

    public:
        using Super::Release;
        using Super::Reset;

        UniqueResource() = default;

        template <
            typename R2,
            typename D2,
            bool NoExcept = noexcept(UniqueResource(std::declval<R2>(), std::declval<D2>(), true))>
        explicit UniqueResource(R2 && Resource, D2 && Destruct) noexcept(NoExcept) :
            UniqueResource(std::forward<R2>(Resource), std::forward<D2>(Destruct), true) { }

        [[nodiscard]] decltype(auto) Get() const noexcept {
            return Super::Resource().Get();
        }

        [[nodiscard]] decltype(auto) GetDeleter() const noexcept {
            return Super::Destruct().Get();
        }

        template <
            typename T = R1,
            typename std::enable_if_t<std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>, int> = 0>
        [[nodiscard]] auto operator->() const noexcept {
            return Get();
        }

        template <
            typename T = R1,
            typename std::enable_if_t<std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>, int> = 0>
        [[nodiscard]] decltype(auto) operator*() const noexcept {
            return *Get();
        }

    private:
        template <typename R2, typename D2, typename S>
        friend auto MakeUniqueResourceChecked(R2 && Resource, const S& Sentinel, D2&& Destruct)
            MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R2, D2);

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDestruct, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable && !CR::NoExcept && !CD::NoExcept, int> = 0>
        explicit UniqueResource(R2 && Resource, D2 && Destruct, bool bExecuteOnReset) :
            Super(
                std::forward_as_tuple(
                    std::in_place,
                    std::forward<typename CR::Type>(Resource),
                    ScopeExit([&Resource, &Destruct, bExecuteOnReset]() {
                        if (bExecuteOnReset) {
                            std::invoke(Destruct, Resource);
                        }
                    })),
                std::forward_as_tuple(
                    std::in_place,
                    std::forward<typename CD::Type>(Destruct),
                    ScopeExit([this, &Destruct, bExecuteOnReset]() {
                        if (bExecuteOnReset) {
                            std::invoke(Destruct, this->Resource().Get());
                        }
                    })),
                bExecuteOnReset) {
            static_assert(std::is_invocable_v<D2&, R2>);
            static_assert(std::is_invocable_v<D2&, decltype(std::declval<TResource&>().Get())>);
        }

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDestruct, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable && CR::NoExcept && !CD::NoExcept, int> = 0>
        explicit UniqueResource(R2 && Resource, D2 && Destruct, bool bExecuteOnReset) :
            Super(
                std::forward_as_tuple(std::in_place, std::forward<typename CR::Type>(Resource)),
                std::forward_as_tuple(
                    std::in_place,
                    std::forward<typename CD::Type>(Destruct),
                    ScopeExit([this, &Destruct, bExecuteOnReset]() {
                        if (bExecuteOnReset) {
                            std::invoke(Destruct, this->Resource().Get());
                        }
                    })),
                bExecuteOnReset) {
            static_assert(std::is_invocable_v<D2&, decltype(std::declval<TResource&>().Get())>);
        }

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDestruct, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable && !CR::NoExcept && CD::NoExcept, int> = 0>
        explicit UniqueResource(R2 && Resource, D2 && Destruct, bool bExecuteOnReset) :
            Super(
                std::forward_as_tuple(
                    std::in_place,
                    std::forward<typename CR::Type>(Resource),
                    ScopeExit([&Resource, &Destruct, bExecuteOnReset]() {
                        if (bExecuteOnReset) {
                            std::invoke(Destruct, Resource);
                        }
                    })),
                std::forward_as_tuple(std::in_place, std::forward<typename CD::Type>(Destruct)),
                bExecuteOnReset) {
            static_assert(std::is_invocable_v<D2&, R2>);
        }

        template <
            typename R2,
            typename D2,
            typename CR = details::ResourceConstructible<TResource, R2>,
            typename CD = details::ResourceConstructible<TDestruct, D2>,
            typename std::enable_if_t<CR::Enable && CD::Enable && CR::NoExcept && CD::NoExcept, int> = 0>
        explicit UniqueResource(R2 && Resource, D2 && Destruct, bool bExecuteOnReset) noexcept :
            Super(
                std::forward_as_tuple(std::in_place, std::forward<typename CR::Type>(Resource)),
                std::forward_as_tuple(std::in_place, std::forward<typename CD::Type>(Destruct)),
                bExecuteOnReset) { }
    };

    template <typename R, typename D>
    UniqueResource(R, D)->UniqueResource<R, D>;

    template <typename R, typename D>
    UniqueResource(R, D, bool)->UniqueResource<R, D>;

    template <typename R, typename D, typename S = std::decay_t<R>>
    [[nodiscard]] auto MakeUniqueResourceChecked(R&& Resource, const S& Sentinel, D&& Destruct)
        MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT(R, D) {
        return UniqueResource(std::forward<R>(Resource), std::forward<D>(Destruct), !bool(Resource == Sentinel));
    }
}

#undef MAKE_UNIQUE_RESOURCE_CHECKED_NOEXCEPT