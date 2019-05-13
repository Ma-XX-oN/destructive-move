// destructive_move.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <utility>
#include <type_traits>
#include <new>
#include <tuple>
#include <assert.h>

#ifdef _MSC_VER
# define FUNCSIG __FUNCSIG__
# define NO_VTABLE __declspec(novtable)
# ifndef _CPPRTTI
#  error Must have RTTI on
# endif
#else
# define FUNCSIG __PRETTY_FUNCTION__
# define NO_VTABLE 
#endif

// Generate an error to see what the type that is being dealt with.
#define INTEROGATE_TYPE(x) static_cast<decltype(x)>(nullptr)

#define OUTPUT_FUNC (std::cout << (void*)this << " " << FUNCSIG << "\n")

//=============================================================================
template <typename...T>
constexpr bool always_false = false;

//=============================================================================
namespace detail {
    template <typename T0, typename T1> struct is_same_cv_impl                                       : std::true_type  {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0               , T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0               , T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0               , T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const         , T1               > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const         , T1 const         > : std::true_type  {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const         , T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const         , T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0       volatile, T1               > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0       volatile, T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0       volatile, T1       volatile> : std::true_type  {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0       volatile, T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const volatile, T1               > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const volatile, T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const volatile, T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_same_cv_impl<T0 const volatile, T1 const volatile> : std::true_type  {};
}

template <typename T0, typename T1>
constexpr bool is_same_cv_v = detail::is_same_cv_impl<
    std::remove_pointer_t<std::remove_reference_t<T0>>
    , std::remove_pointer_t<std::remove_reference_t<T1>>
>::value;

//-----------------------------------------------------------------------------

namespace detail {
    template <typename T0, typename T1> struct is_stronger_cv_impl                                       : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0               , T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0               , T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0               , T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const         , T1               > : std::true_type  {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const         , T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const         , T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const         , T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0       volatile, T1               > : std::true_type  {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0       volatile, T1 const         > : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0       volatile, T1       volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0       volatile, T1 const volatile> : std::false_type {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const volatile, T1               > : std::true_type  {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const volatile, T1 const         > : std::true_type  {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const volatile, T1       volatile> : std::true_type  {};
    template <typename T0, typename T1> struct is_stronger_cv_impl<T0 const volatile, T1 const volatile> : std::false_type {};
}

template <typename T0, typename T1>
constexpr bool is_stronger_cv_v = detail::is_stronger_cv_impl<
    std::remove_pointer_t<std::remove_reference_t<T0>>
    , std::remove_pointer_t<std::remove_reference_t<T1>>
>::value;

// This is the function you most likely want.
template <typename T0, typename T1>
constexpr bool is_stronger_or_same_cv_v = is_stronger_cv_v<T0, T1> || is_same_cv_v<T0, T1>;

//=============================================================================
// Remove reference and any cv qualifiers. (like in C++20)
template <typename T>
using  remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

//=============================================================================
// Remove reference, first level pointer and any cv qualifiers.
template <typename T>
using  strip_t = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;

//=============================================================================
namespace detail {
    template <typename From, typename To> struct copy_rp               { using type = std::remove_reference_t<To>    ; };
    template <typename From, typename To> struct copy_rp<From &  , To> { using type = std::remove_reference_t<To> &  ; };
    template <typename From, typename To> struct copy_rp<From && , To> { using type = std::remove_reference_t<To> && ; };
    template <typename From, typename To> struct copy_rp<From *  , To> { using type = std::remove_reference_t<To> *  ; };
    template <typename From, typename To> struct copy_rp<From *& , To> { using type = std::remove_reference_t<To> *& ; };
    template <typename From, typename To> struct copy_rp<From *&&, To> { using type = std::remove_reference_t<To> *&&; };
}
// Copies reference/pointer type from From to To
template <typename From, typename To> using  copy_rp_t = typename detail::copy_rp<From, To>::type;

//=============================================================================
namespace detail {
    template <typename From, typename To> struct copy_cv                          { using type = To                ; };
    template <typename From, typename To> struct copy_cv<From const         , To> { using type = To  const         ; };
    template <typename From, typename To> struct copy_cv<From       volatile, To> { using type = To        volatile; };
    template <typename From, typename To> struct copy_cv<From const volatile, To> { using type = To  const volatile; };
}
// Copies const/volatile from From to To behind reference and first level pointer if any.
//
// Any reference is first removed from From.
template <typename From, typename To>
using  copy_cv_t = copy_rp_t<To, typename detail::copy_cv<std::remove_reference_t<From>, strip_t<To>>::type>;

//=============================================================================
// Used for std::forward to take a type From and copy its cv and rp over to
// type To.
template <typename From, typename To>
using  fwd_type_t = copy_rp_t<From, copy_cv_t<From, To>>;

template <typename From, typename To>
decltype(auto) fwd_like(To&& obj_ref)
{
    return std::forward<fwd_type_t<From, To>>(obj_ref);
}

static_assert(std::is_same_v<fwd_type_t<int const, float&>, float const>, "");
//=============================================================================

template <typename T>     using  to_const = const T;

//=============================================================================
namespace detail {
    template <typename T> struct to_const_rvalue_impl      { using type = T; };
    template <typename T> struct to_const_rvalue_impl<T&&> { using type = T const&&; };
}
template <typename T>     using  to_const_rvalue = typename detail::to_const_rvalue_impl<T>::type;

//=============================================================================
namespace detail {
    template <typename T> struct to_const_lvalue_impl      { using type = T; };
    template <typename T> struct to_const_lvalue_impl<T&&> { using type = T const&&; };
}
template <typename T>     using  to_const_lvalue = typename detail::to_const_lvalue_impl<T>::type;

//=============================================================================
template <template <typename> class Op, typename T>
auto&& cast(T&& item) {
    return std::forward<fwd_type_t<T, Op<T>>>(const_cast<remove_cvref_t<T>&&>(item));
}

template <template <typename> class Op, typename T>
auto&& cast(T&  item) {
    return std::forward<fwd_type_t<T, Op<T>>>(const_cast<remove_cvref_t<T>& >(item));
}

//=============================================================================
template<typename Derived, typename...Ts>
struct emplace_params;

template <typename...Ts>
auto&& get_first(emplace_params<Ts...>& params);

template<typename Derived, typename...Ts>
struct emplace_params final
{
    template <typename...Us>
    friend auto&& get_first(emplace_params<Us...>& params);

    emplace_params(Ts&&...args)
        : ref_storage(std::forward<Ts>(args)...)
    {}

    void uninitialized_construct(void* storage)
        noexcept(noexcept(Derived(std::forward<Ts>(std::declval<Ts>())...)))
    {
        std::apply([storage](auto &&... args) {
            new (storage) Derived(std::forward<Ts>(args)...);
        }, ref_storage);
    }

    // If "this" is const, then passed the parameters as not modifiable (const)
    void uninitialized_construct(void* storage) const
        noexcept(noexcept(Derived(std::forward<to_const_lvalue<Ts>>(std::declval<Ts>())...)))
    {
        std::apply([storage](auto &&... args) {
            new (storage) Derived(std::forward<to_const_lvalue<Ts>>(args)...);
        }, ref_storage);
    }

private:
    std::tuple<decltype(std::forward<Ts>(std::declval<Ts>()))...> ref_storage;
};

template <typename T, typename...Ts>
auto emplace(Ts&& ...args)
{
    return emplace_params<T, Ts...>(std::forward<Ts>(args)...);
}

template <typename...Ts>
auto&& get_first(emplace_params<Ts...>& params)
{
    return std::get<0>(params.ref_storage);
}

struct default_empty {};

namespace detail
{
    template <typename T, typename Derived>
    class destructively_movable_impl;
}

template <typename Contained, typename Is_valid = void>
class destructively_movable
    : public detail::destructively_movable_impl<Contained, destructively_movable<Contained, Is_valid>>
{
    using base = detail::destructively_movable_impl<Contained, destructively_movable<Contained, Is_valid>>;
public:
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (destructively_movable const& over destructively_movable_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    destructively_movable(destructively_movable     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    destructively_movable(destructively_movable const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    destructively_movable&  operator=(destructively_movable     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }

    destructively_movable&& operator=(destructively_movable     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }

    void is_valid(bool value)                noexcept {        Is_valid(this, value); }
    void is_valid(bool value)       volatile noexcept {        Is_valid(this, value); }
    bool is_valid(          ) const          noexcept { return Is_valid(this); }
    bool is_valid(          ) const volatile noexcept { return Is_valid(this); }

    using base::base;
};

template <typename Contained>
class destructively_movable<Contained, void>
    : public detail::destructively_movable_impl<Contained, destructively_movable<Contained, void>>
{
    using base = detail::destructively_movable_impl<Contained, destructively_movable<Contained, void>>;
    bool m_isValid;
public:
    // This copy/move constructor is needed because on copying/moving,
    // m_isValid will be updated after is was already set by the
    // base copy/move constructor.  This either invalidates the value
    // or does an extra copy.
    //
    // Note: By defining the copy/move constructor/assignment operator members
    //       explicitly like this, the base copy constructor/assignmnt
    //       operator members are ignored since the templated constructor/
    //       assigment that take a universal reference is a better match.
    //       (destructively_movable const& over destructively_movable_impl
    //       const&).  The only way to call that copy constructor is to
    //       instantiate the base class directly, or explicitly call the
    //       assignment operator, if they existed.  However they've been
    //       deleted those anyway if someone were so inclided.
    destructively_movable(destructively_movable     && obj) noexcept(noexcept(base(std::move(obj)))) : base(std::move(obj)) {}
    destructively_movable(destructively_movable const& obj) noexcept(noexcept(base(          obj ))) : base(          obj ) {}

    destructively_movable&  operator=(destructively_movable const& obj)          &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj)          &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }
    destructively_movable&  operator=(destructively_movable const& obj) volatile &  noexcept(noexcept(                 base::operator=(          obj ))) { return                  base::operator=(          obj ); }
    destructively_movable&  operator=(destructively_movable     && obj) volatile &  noexcept(noexcept(                 base::operator=(std::move(obj)))) { return                  base::operator=(std::move(obj)); }

    destructively_movable&& operator=(destructively_movable const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }

    void is_valid(bool value)                noexcept {        m_isValid = value; }
    void is_valid(bool value)       volatile noexcept {        m_isValid = value; }
    bool is_valid(          ) const          noexcept { return m_isValid; }
    bool is_valid(          ) const volatile noexcept { return m_isValid; }

    using base::base;
};

template <typename T>
struct is_destructively_movable : std::false_type {};

template <typename T, typename I>
struct is_destructively_movable<destructively_movable<T, I>> : std::true_type {};

namespace detail {
template <typename Contained, typename Derived>
class destructively_movable_impl
{
    Derived                * derived()                noexcept { return static_cast<Derived                *>(this); }
    Derived       volatile * derived()       volatile noexcept { return static_cast<Derived       volatile *>(this); }
    Derived const          * derived() const          noexcept { return static_cast<Derived const          *>(this); }
    Derived const volatile * derived() const volatile noexcept { return static_cast<Derived const volatile *>(this); }

    alignas(Contained) char m_storage[std::is_empty<Contained>::value ? 0 : sizeof(Contained)];

    template <typename U>
    static auto&& object(U&& this_ref) noexcept
    {
        assert(this_ref.is_valid());
        return fwd_like<U>(
            reinterpret_cast<copy_cv_t<U, Contained>&>(
                *this_ref.m_storage
            )
        );
    }

    template <typename T> struct bare_type_impl                           { using type = T; };
    template <typename T> struct bare_type_impl<destructively_movable<T>> { using type = T; };

    // Strips and extract the wrapped type or if not wrapped, just return the type.
    template <typename T> using  bare_t = typename bare_type_impl<strip_t<T>>::type;
    
    template <typename T> using  fwd_to_bare_type_t = fwd_type_t<T, bare_t<T>>;

    template <typename T>
    static void clear_is_valid_if_moving_distructively_movable(T&& value) noexcept
    {
        if constexpr (is_destructively_movable<T>::value)
        {
            // NOTE: is_destructively_movable<first_element>::value implies
            //       that first_element is an rvalue, otherwise it would be
            //       an lvalue reference, which would fail to match.
            if constexpr (std::is_same<Contained, typename T::type>::value)
            {
                // Operation was a move on a destructively_movable object.
                value.is_valid(false);
            }
        }
    }
    destructively_movable_impl(destructively_movable_impl const&) = delete;
    destructively_movable_impl& operator=(destructively_movable_impl const&) = delete;
public:

    using type = Contained;
    destructively_movable_impl(default_empty const& obj) noexcept {
        is_valid(false);
    }

    // Does emplace construction of Contained, including "move/copy
    // construction".
    //
    // NOTE: Neither move or copy constructors would actually be called because
    //       this class is never passed a destructively_movable_impl const& or
    //       destructively_movable_impl&& directly anyway, since the derived
    //       class explicitly passes a destructively_movable const& or
    //       destructively_movable&& respectively.
    template <typename...Ts>
    destructively_movable_impl(Ts&&...construct) noexcept(noexcept(
        destructively_movable_impl(emplace<Contained>(std::forward<Ts>(construct)...))
    ))
        : destructively_movable_impl(emplace<Contained>(std::forward<Ts>(construct)...))
    {
        assert(is_valid());
    }

    // Does emplace construction of T
    // If going to accept derived types, then the size of objects must be the same.
    template <typename T, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    destructively_movable_impl(emplace_params<T, Ts...>&& construct) noexcept(noexcept(
        construct.uninitialized_construct(m_storage)
    ))
    {
        //OUTPUT_FUNC;
        construct.uninitialized_construct(m_storage);
        // HMMMMM. Need more than this I think as this can leave the object
        // potentially hanging on to resources.  Or should that be on the
        // onus of the user who implements the move constructor?
        if constexpr (sizeof...(Ts) == 1)
        {
            using first_element = std::tuple_element_t<0, std::tuple<Ts...>>;
            clear_is_valid_if_moving_distructively_movable(
                std::forward<first_element>(get_first(construct))
            );
        }
        is_valid(true);
    }

    // Does emplace construction of T.
    //
    // A const emplace_params implies that all params passed to constructor
    // will be const lvalues, regardless of what they actually were.  This
    // allows for safe reuse of an emplace_params.
    //
    // If going to accept derived types, then the size of objects must be the same.
    template <typename T, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    destructively_movable_impl(emplace_params<T, Ts...> const& construct) noexcept(noexcept(
        construct.uninitialized_construct(m_storage)
    ))
    {
        OUTPUT_FUNC;
        construct.uninitialized_construct(m_storage);
        is_valid(true);
    }

    ~destructively_movable_impl()
    {
        if constexpr (!std::is_trivially_destructible<Contained>::value) {
            if (is_valid()) {
                object().~Contained();
            }
        }
    }

    // Forward the heavy lifting to the constructors
    template <typename...Ts>
    Derived& construct(Ts&&...construct) noexcept(noexcept(
            Derived(std::forward<Ts>(construct)...)
        ))
    {
        OUTPUT_FUNC;
        assert(!is_valid());
        return *new (this) Derived(std::forward<Ts>(construct)...);
    }
private:
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs)
        noexcept(
            noexcept(std::forward<T>(lhs).object().operator=(std::forward<U>(rhs)))
            // Weird.  Enabling the next line results in the error msg:
            //
            //   1>destructive_move.cpp(437): error : exception specification of 'destructively_movable' uses itself
            //
            //This sounds like there is a call cycle which I don't see.

            //&& noexcept(lhs.construct(std::forward<U>(rhs)))
        )
    {
        if constexpr (is_destructively_movable<U>::value)
        {
            // Only assign something if there is something to assign.
            if (rhs.is_valid()) {
                std::forward<T>(lhs).object().operator=(std::forward<U>(rhs));
            }
        }
        else {
            if (lhs.is_valid()) {
                std::forward<T>(lhs).object().operator=(std::forward<U>(rhs));
            }
            else {
                lhs.construct(std::forward<U>(rhs));
            }
        }
        assert(!is_destructively_movable<U>::value || lhs.is_valid());
        //INTEROGATE_TYPE(                (lhs));
        //INTEROGATE_TYPE( std::forward<T>(lhs));
        clear_is_valid_if_moving_distructively_movable(std::forward<U>(rhs));
        return fwd_like<T>(static_cast<Derived&>(lhs));
    }

public:
    // Wasted hours trying to do:
    //   return assign(static_cast<Derived&>(*this), std::forward<T>(rhs));
    // Seems that the compiler gets confused and thinks that assign() returns
    // Contained for some reason.  Tried to use following signatures:
    //
    //    template <typename T, typename U> auto&&         assign(T&& lhs, U&& rhs);
    //    template <typename T, typename U> decltype(auto) assign(T&& lhs, U&& rhs);
    //    template <typename T, typename U> auto           assign(T&& lhs, U&& rhs) -> Derived&&;
    //
    // While returning std::forward<T>(lhs).  Interogating the type inside the
    // function revaled that it was what was expected (equivilant to Derived).
    // Interogating the actual returned type gave the equivilant of Contained.
    // Even when using the debugger, auto shows return type as
    // destructively_movable_impl reference not destructively_movable reference.
    //
    // Gave up and returned a static_cast to Derived& instead, which worked.
    template <typename T> auto&  operator=(T&& rhs)          &  { assign(         (*this), std::forward<T>(rhs)); return static_cast<Derived& >(*this); }
    template <typename T> auto&  operator=(T&& rhs) volatile &  { assign(         (*this), std::forward<T>(rhs)); return static_cast<Derived& >(*this); }
    // Does it even make sense to assign to a rvalue?  Limited value?
    template <typename T> auto&& operator=(T&& rhs)          && { assign(std::move(*this), std::forward<T>(rhs)); return static_cast<Derived&&>(*this); }
    template <typename T> auto&& operator=(T&& rhs) volatile && { assign(std::move(*this), std::forward<T>(rhs)); return static_cast<Derived&&>(*this); }

    void destruct()
    {
        assert(is_valid());
        object().~Contained();
        is_valid(false);
    }

    void is_valid(bool value)                noexcept {        derived()->is_valid(value); }
    void is_valid(bool value)       volatile noexcept {        derived()->is_valid(value); }
    bool is_valid(          ) const          noexcept { return derived()->is_valid(); }
    bool is_valid(          ) const volatile noexcept { return derived()->is_valid(); }


    // Access object as lvalue reference
    auto&& object()                &  noexcept { return object(          *this ); }
    auto&& object()       volatile &  noexcept { return object(          *this ); }
    auto&& object() const          &  noexcept { return object(          *this ); }
    auto&& object() const volatile &  noexcept { return object(          *this ); }

    // Access object as rvalue reference
    auto&& object()                && noexcept { return object(std::move(*this)); }
    auto&& object()       volatile && noexcept { return object(std::move(*this)); }
    auto&& object() const          && noexcept { return object(std::move(*this)); }
    auto&& object() const volatile && noexcept { return object(std::move(*this)); }

    // Member of operators
    auto operator->()                noexcept { return &object(); }
    auto operator->()       volatile noexcept { return &object(); }
    auto operator->() const          noexcept { return &object(); }
    auto operator->() const volatile noexcept { return &object(); }

    // Address of operators
    auto operator& ()                noexcept { return &object(); }
    auto operator& ()       volatile noexcept { return &object(); }
    auto operator& () const          noexcept { return &object(); }
    auto operator& () const volatile noexcept { return &object(); }

    // Conversion operators
    //
    // NOTE: Only implicit conversion casts to weaker types and base
    //       classes with the same reference type are allowed.  However,
    //       becasue keyword "this" cannot be interrogated external to the
    //       body of the function, each function has to be manually writen.
    //
    //                     | downcast   | is_stronger_or_same  ||           
    //    lrvalue transfer | or upcast  | <T, decltype(*this)> || Cast is   
    //   ==================+============+======================++===========
    //     T&& from &&     | downcast   | true                 ||  implicit 
    //     T&  from &      | downcast   | true                 ||  implicit 
    //     T&& from &&     | upcast     | true                 ||  explicit  // This will not throw. Need a special
    //     T&  from &      | upcast     | true                 ||  explicit  // X_dynamic_cast for that.
    //   ------------------+------------+----------------------++-----------
    //     T&& from &&     | don't care | false                ||  explicit 
    //     T&  from &      | don't care | false                ||  explicit 
    //   ------------------+------------+----------------------++-----------
    //     T&& from &      | don't care | don't care           ||  explicit 
    //     T&  from &&     | don't care | don't care           ||  explicit 
    //
    // There is no exception when downcasting, hence noexcept(IS_DOWNCAST).
    //
    // There will be 4 functions (no cv, c, v, and cv) for each line in the
    // chart except.  That's 4*8 = 32 functions.  That's a lot of code,
    // with a lot of potential for error.  Because the main body does
    // exactly the same thing (and also happens to be very simple as they
    // just hand off to other functions), using a macro is the safest and
    // clearest way to do this.
    //
    // If what type of "this" could be deduced, with all its qualifiers,
    // then this would colapse to two functions, or a single function if
    // template <typename U> operator U&&() was a universal reference
    // conversion function. 

#define IS_DOWNCAST (std::is_base_of<strip_t<U>, Contained>::value)
#define IS_UPCAST   (std::is_base_of<Contained, strip_t<U>>::value && !std::is_same<Contained, strip_t<U>>::value)
// Using ... instead of a named parameter because when enabler(const_volatile) is called with
// const_volatile being blank (no cv), VC++ complains.
#define IS_CV_STRONGER_OR_SAME(...) (is_stronger_or_same_cv_v<U, int __VA_ARGS__>)
#define IS_ALWAYS_ENABLED(...)      (!always_false<U>)

#define CONVERSION(const_volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        template <typename U, std::enable_if_t< enabler(const_volatile) , int> = 0>             \
        explicit operator U convert_to_lrvalue () const_volatile convert_from_lrvalue noexcept  \
        {                                                                                       \
            return fwd_like<U convert_to_lrvalue>(object());                                    \
        }

#define CONVERSION_ALL_CVS(        convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(              , convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(      volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(const         , convert_to_lrvalue, convert_from_lrvalue, enabler, explicit) \
        CONVERSION(const volatile, convert_to_lrvalue, convert_from_lrvalue, enabler, explicit)

    CONVERSION_ALL_CVS(&&, &&,  IS_DOWNCAST && IS_CV_STRONGER_OR_SAME,         );
    CONVERSION_ALL_CVS(& , & ,  IS_DOWNCAST && IS_CV_STRONGER_OR_SAME,         );
    CONVERSION_ALL_CVS(&&, &&,  IS_UPCAST   && IS_CV_STRONGER_OR_SAME, explicit);
    CONVERSION_ALL_CVS(& , & ,  IS_UPCAST   && IS_CV_STRONGER_OR_SAME, explicit);

    CONVERSION_ALL_CVS(&&, &&,                !IS_CV_STRONGER_OR_SAME, explicit);
    CONVERSION_ALL_CVS(& , & ,                !IS_CV_STRONGER_OR_SAME, explicit);

    CONVERSION_ALL_CVS(&&, & ,                      IS_ALWAYS_ENABLED, explicit);
    CONVERSION_ALL_CVS(& , &&,                      IS_ALWAYS_ENABLED, explicit);

#undef CONVERSION_ALL_CVS
#undef CONVERSION
#undef IS_ALWAYS_ENABLED
#undef IS_CV_STRONGER_OR_SAME
#undef IS_UPCAST
#undef IS_DOWNCAST
};

} //namespace detail {

static int i = 0;
struct X {
    int m_i = ++i;
    X()  { std::cout << "X constructed\n"; }
    ~X() { std::cout << "X destructed\n"; }

    void test()                && { std::cout << "               rvalue\n"; }
    void test()       volatile && { std::cout << "      volatile rvalue\n"; }
    void test() const          && { std::cout << "const          rvalue\n"; }
    void test() const volatile && { std::cout << "const volatile rvalue\n"; }

    void test()                &  { std::cout << "               lvalue\n"; }
    void test()       volatile &  { std::cout << "      volatile lvalue\n"; }
    void test() const          &  { std::cout << "const          lvalue\n"; }
    void test() const volatile &  { std::cout << "const volatile lvalue\n"; }
};

#include <vector>
#include <algorithm>
#include <boost/circular_buffer.hpp>
template <typename T, typename Allocator   = std::allocator<T>>
struct augmented_vector
{
    struct is_valid;

    using objects_t              = std::vector<destructively_movable<T, is_valid>, Allocator>;

    using value_type             = typename objects_t::value_type;
    using allocator_type         = Allocator;
    using size_type              = typename objects_t::size_type;
    using difference_type        = typename objects_t::difference_type;
    using reference              = typename objects_t::reference;
    using const_reference        = typename objects_t::const_reference;
    using pointer                = typename objects_t::pointer;
    using const_pointer          = typename objects_t::const_pointer;
    using iterator               = typename objects_t::iterator;
    using const_iterator         = typename objects_t::const_iterator;
    using reverse_iterator       = typename objects_t::reverse_iterator;
    using const_reverse_iterator = typename objects_t::const_reverse_iterator;

    std::vector<bool> external_valid_flags;
    objects_t objects;

    static constexpr int max_cached = 8;
    struct local_vector_info
    {
        augmented_vector* vector;
        iterator begin, end;
        bool allow_write;
    };

    static std::vector<local_vector_info> all_augmented_vectors;
    static boost::circular_buffer<local_vector_info> cached_augmented_vectors;
    static int cached_augmented_vectors_stored;
    struct is_valid
    {
        std::pair<augmented_vector*, bool> get_this(destructively_movable<T>* pObject)
        {
            auto match = [](local_vector_info& lvi) {
                return lvi.vector->objects.data() < pObject
                    && pObject < lvi.vector->objects.data() + lvi.vector->objects.size();
            };
            auto found = std::find_if(
                augmented_vector::cached_augmented_vectors.rbegin()
                , augmented_vector::cached_augmented_vectors.end()
                , match
            );
            if (found == augmented_vector::cached_augmented_vectors.rend())
            {
                auto found = std::lower_bound(
                    augmented_vector::all_augmented_vectors.begin()
                    , augmented_vector::all_augmented_vectors.end()
                    , pObject
                    , match
                );
                assert(found != augmented_vector::all_augmented_vectors.end());
                return { found->vector, found->allow_write };
            }
            assert(found != augmented_vector::cached_augmented_vectors.end());
            augmented_vector::cached_augmented_vectors.push_back(*found);
            return { found->vector, found->allow_write };
        }

        bool operator()(destructively_movable<T>* pObject) const
        {
            auto[this_, allow_write] = get_this(pObject);
            return external_valid_flags[std::distance(this_->objects, pObject)];
        }
        void operator()(destructively_movable<T>* pObject, bool value)
        {
            auto[this_, allow_write] = get_this(pObject);
            if (allow_write) {
                external_valid_flags[std::distance(this_->objects, pObject)] = value;
            }
        }
    };

    template <typename C>
    auto find_in_list(C& container)
    {
        auto found = std::find_if(
            container.begin()
            , container.end()
            , [this](local_vector_info& lvi) {
                return lvi.vector == this;
            });
        return std::make_pair(found, (found != container.end()));
    }

    template <typename C>
    void remove_from_list(C& container)
    {
        auto[it, found] = find_in_list(container);
        if (found) {
            container.erase(found);
        }
    }

    void remove_from_list()
    {
        remove_from_list(all_augmented_vectors);
        remove_from_list(cached_augmented_vectors);
    }

    template <typename C>
    void adjust_in_list(C& container)
    {
        auto[it, found] = find_in_list(container);
        if (found) {
            it->begin = begin();
            it->end   = begin() + capacity();
        }
    }

    void adjust_in_list()
    {
        adjust_in_list(all_augmented_vectors);
        adjust_in_list(cached_augmented_vectors);
    }

    bool cmp_vector_info(local_vector_info const& lhs, local_vector_info const& rhs)
    {
        return lhs.begin < rhs.end;// && lhs.end < rhs.end;
    }

    typename decltype(all_augmented_vectors)::iterator find_in_all_augmented_vectors()
    {
        return std::lower_bound(all_augmented_vectors.begin(), all_augmented_vectors.end()
            , lvi
            , cmp_vector_info);
    }

    void add_to_list()
    {
        local_vector_info lvi = { this, begin(), end(), true };
        auto found = find_in_all_augmented_vectors();
        if (found != all_augmented_vectors.end()) {
            all_augmented_vectors.insert(found, lvi);
        }
    }

    void reposition_in_all_augmented_vectors(typename decltype(all_augmented_vectors)::iterator it_item)
    {
        assert(all_augmented_vectors.size() != 0 && it_item != all_augmented_vectors.end());
        if (it_item + 1 != all_augmented_vectors.end()) {
            if (it_item[1].begin > end()) {
                auto new_location = std::lower_bound(all_augmented_vectors.begin(), it_item, *it_item, cmp_vector_info);
                // rotate
            }
        }
        else if (it_item != all_augmented_vectors.begin()) {
            if (begin() < it_item[-1].end) {
                auto new_location = std::lower_bound(it_item + 1, all_augmented_vectors.end(), *it_item, cmp_vector_info);
                // rotate

            }
        }
    }
    // ==] Main [==============================================================
    augmented_vector() noexcept(noexcept(Allocator()))
        : objects()
        , external_valid_flags()
    {
        allow_write = true;
    }

    explicit augmented_vector(const Allocator& alloc) noexcept
        : objects(alloc)
        , external_valid_flags()
    {
        allow_write = true;
    }

    augmented_vector(size_type count,
        const T& value,
        const Allocator& alloc = Allocator())
        : objects(count, value, alloc)
        , external_valid_flags(count, true)
    {
        allow_write = true;
    }

    explicit augmented_vector(size_type count, const Allocator& alloc = Allocator())
        : objects(count, alloc)
        , external_valid_flags(count, true)
    {
        allow_write = true;
    }

    template< class InputIt >
    augmented_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
        : objects(first, last, alloc)
        , external_valid_flags(objects.size(), true)
    {
        allow_write = true;
    }

    augmented_vector(const augmented_vector& other, const Allocator& alloc)
        : objects(other.objects)
        , external_valid_flags(other.external_valid_flags)
    {
        allow_write = true;
    }

    augmented_vector(augmented_vector&& other) noexcept
        : objects(std::move(other.objects))
        , external_valid_flags(std::move(other.external_valid_flags))
    {
        allow_write = true;
    }

    augmented_vector(augmented_vector&& other, const Allocator& alloc)
        : objects(std::move(other.objects), alloc)
        , external_valid_flags(std::move(other.external_valid_flags))
    {
        allow_write = true;
    }

    augmented_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : objects(init, alloc)
        , external_valid_flags(init.size(), true)
    {
        allow_write = true;
    }

    ~augmented_vector()
    {
        allow_write = false;
        remove_from_list();
    }

    augmented_vector& operator=(const augmented_vector& other)
    {
        objects = other.objects;
        external_valid_flags = other.external_valid_flags;
    }

    augmented_vector& operator=(augmented_vector&& other) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
        || std::allocator_traits<Allocator>::is_always_equal::value
    )
    {
        objects = std::move(other.objects);
        external_valid_flags = std::move(other.external_valid_flags);
    }

    augmented_vector& operator=(std::initializer_list<T> ilist)
    {
        assign(ilist);
    }

    void assign(size_type count, const T& value)
    {
        objects.assign(count, value);
        external_valid_flags.assign(count, true);
    }

    template< class InputIt >
    void assign(InputIt first, InputIt last)
    {
        objects.assign(first, last);
        external_valid_flags.assign(objects.size(), true);
    }

    void assign(std::initializer_list<T> ilist)
    {
        objects.assign(ilist);
        external_valid_flags.assign(ilist.size(), true);
    }

    allocator_type get_allocator() const
    {
        return objects.get_allocator();
    }

    // ==] Element access [====================================================
    reference       at(size_type pos)
    {
        return objects.at(pos);
    }

    const_reference at(size_type pos) const
    {
        return objects.at(pos);
    }

    reference       operator[](size_type pos)
    {
        return objects[pos];
    }

    const_reference operator[]( size_type pos ) const
    {
        return objects[pos];
    }

    reference front()
    {
        return objects.front();
    }

    const_reference front() const
    {
        return objects.front();
    }

    reference back()
    {
        return objects.back();
    }

    const_reference back() const
    {
        return objects.back();
    }

    T* data() noexcept
    {
        return objects.data();
    }

    const T* data() const noexcept
    {
        return objects.data();
    }

    // ==] Iterators [=========================================================
    auto   begin()       noexcept { return objects.  begin(); }
    auto  cbegin() const noexcept { return objects. cbegin(); }
    auto  rbegin()       noexcept { return objects. rbegin(); }
    auto crbegin() const noexcept { return objects.crbegin(); }

    auto     end()       noexcept { return objects.    end(); }
    auto    cend() const noexcept { return objects.   cend(); }
    auto    rend()       noexcept { return objects.   rend(); }
    auto   crend() const noexcept { return objects.  crend(); }

// ==] Capacity [==========================================================
    bool empty() noexcept { return objects.empty(); }
    size_type size() noexcept { return objects.size(); }
    size_type max_size() noexcept { return objects.max_size(); }
    size_type capacity() noexcept { return objects.capacity(); }
    void reserve(size_type new_cap) {
        bool resized = (new_cap > capacity());
        objects.reserve(new_cap);
        external_valid_flags.reserve(new_cap);
        if (resized) {
            adjust_in_list();
        }
    }

    void shrink_to_fit()
    {
        if (size() < capacity())
        {
            objects.shrink_to_fit();
            external_valid_flags.shrink_to_fit();
            adjust_in_list();
        }
    }

    // ==] Modifiers [=========================================================
    void clear() noexcept
    {
        external_valid_flags.clear();
        objects.clear();
    }
    enum class adding { no, yes };
    enum class single { no, yes };
    template <adding Adding, single Single>
    struct adjust
    {
        augmented_vector& v;
        size_type offset;
        iterator old_begin;
        size_type old_size;
        adjust(augmented_vector& v, const_iterator it)
            : v(v)
            , offset(std::distance(v.cbegin(), it))
            , old_begin(v.begin())
            , old_size(v.size())
        {}

        ~adjust() noexcept(false)
        {
            if constexpr (Adding == adding::yes) {
                try {
                    if constexpr (Single == single::yes) {
                        v.external_valid_flags.insert(v.external_valid_flags.begin() + offset, true);
                    }
                    else {
                        v.external_valid_flags.insert(v.external_valid_flags.begin() + offset, v.size() - old_size, true);
                    }
                }
                catch (...) { // FIXME
                    // TODO: What should be done if insert throws?
                    //       Erase last elements that were inserted, till the size matches?
                    v.objects.erase(v.objects.begin() + offset, v.objects.begin() + (v.size() - old_size));
                    if (v.begin() != old_begin) {
                        v.adjust_in_list();
                    }
                    throw;
                }
                if (v.begin() != old_begin) {
                    v.adjust_in_list();
                }
            }
            else {
                try {
                    if constexpr (Single == single::yes) {
                        v.external_valid_flags.erase(v.external_valid_flags.begin() + offset);
                    }
                    else {
                        v.external_valid_flags.erase(v.external_valid_flags.begin() + offset, v.external_valid_flags.begin() + (old_size - v.size()));
                    }
                    assert(v.begin() == old_begin);
                }
                catch (...) { // FIXME
                    // TODO: What should be done if erase throws?  Can it throw?
                    assert(false);  // I think the throw comes from the objects, not the erasing.  A bool doesn't throw, so should be fine.
                    throw;
                }
            }
        }
    };

    iterator insert(const_iterator pos, const T&& value)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        return objects.insert(pos, std::move(value));
    }

    iterator insert(const_iterator pos, const T& value)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        return objects.insert(pos,           value );
    }

    iterator insert(const_iterator pos, size_type count, const T& value)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        return objects.insert(pos, count, value);
    }

    template< class InputIt >
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        adjust<adding::yes, single::no> adjust_(*this, pos);
        return objects.insert(pos, first, last);
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist)
    {
        adjust<adding::yes, single::no> adjust_(*this, pos);
        return objects.insert(pos, ilist);
    }

    template< class... Args > 
    iterator emplace(const_iterator pos, Args&&... args)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        return objects.emplace(pos, std::forward<Args>(args)...);
    }

    iterator erase(const_iterator pos)
    {
        adjust<adding::no, single::yes> adjust_(*this, pos);
        return objects.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        adjust<adding::no, single::no> adjust_(*this, pos);
        return objects.erase(first, last);
    }

    void push_back(const T& value)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        objects.push_back(value);
    }

    void push_back( T&& value )
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        objects.push_back(std::move(value));
    }

    template< class... Args >
    reference emplace_back(Args&&... args)
    {
        adjust<adding::yes, single::yes> adjust_(*this, pos);
        return objects.emplace_back(std::forward<Args>(args)...);
    }

    void pop_back()
    {
        adjust<adding::no, single::yes> adjust_(*this, pos);
        objects.pop_back();
    }

    void resize(size_type count)
    {
        std::ptrdiff_t diff = count - capacity();
        switch (diff)
        {
        case -1:
            {
                adjust<adding::no, single::yes> adjust_;
                objects.resize(count);
            }
            break;
        case 0:
            break;
        case 1:
            {
                adjust<adding::yes, single::yes> adjust_;
                objects.resize(count);
            }
            break;
        default:
            if (diff < 0) {
                adjust<adding::no, single::no> adjust_;
                objects.resize(count);
            }
            else
            {
                adjust<adding::yes, single::no> adjust_;
                objects.resize(count);
            }
            break;
        }
    }

    void resize(size_type count, const value_type& value)
    {
        std::ptrdiff_t diff = count - capacity();
        switch (diff)
        {
        case -1:
            {
                adjust<adding::no, single::yes> adjust_;
                objects.resize(count, value);
            }
            break;
        case 0:
            break;
        case 1:
            {
                adjust<adding::yes, single::yes> adjust_;
                objects.resize(count, value);
            }
            break;
        default:
            if (diff < 0) {
                adjust<adding::no, single::no> adjust_;
                objects.resize(count, value);
            }
            else
            {
                adjust<adding::yes, single::no> adjust_;
                objects.resize(count, value);
            }
            break;
        }
    }

    void swap( augmented_vector& other ) noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value
        || std::allocator_traits<Allocator>::is_always_equal::value
    )
    {
        swap(objects, other.objects);
        swap(external_valid_flags, other.external_valid_flags);
    }
};

template <typename T, typename Allocator   /*= std::allocator<T>*/>
std::vector<typename augmented_vector<T, Allocator>::local_vector_info> augmented_vector<T, Allocator>::all_augmented_vectors;

template <typename T, typename Allocator   /*= std::allocator<T>*/>
boost::circular_buffer<typename augmented_vector<T, Allocator>::local_vector_info> augmented_vector<T, Allocator>::cached_augmented_vectors(8);

template <typename T, typename Allocator   /*= std::allocator<T>*/>
int augmented_vector<T, Allocator>::cached_augmented_vectors_stored = 0;

int main()
{
    destructively_movable<X> x;
    std::cout << "\nlvalues\n";
    x->test();

    std::cout << "\nlvalues\n";
    static_cast<X                & >(x).test();
    static_cast<X       volatile & >(x).test();
    static_cast<X const          & >(x).test();
    static_cast<X const volatile & >(x).test();
    x.destruct();
    x.construct(emplace<X>());

    std::cout << "\nrvalues\n";
    static_cast<X                &&>(x).test();
    static_cast<X       volatile &&>(x).test();
    static_cast<X const          &&>(x).test();
    static_cast<X const volatile &&>(x).test();

    std::cout << "\nlvalues\n";
    static_cast<destructively_movable<X>                & >(x)->test();
    static_cast<destructively_movable<X>       volatile & >(x)->test();
    static_cast<destructively_movable<X> const          & >(x)->test();
    static_cast<destructively_movable<X> const volatile & >(x)->test();

    std::cout << "\nlvalues\n";
    static_cast<destructively_movable<X>                &&>(x)->test();
    static_cast<destructively_movable<X>       volatile &&>(x)->test();
    static_cast<destructively_movable<X> const          &&>(x)->test();
    static_cast<destructively_movable<X> const volatile &&>(x)->test();

    destructively_movable<X> x1(std::move(x));
    destructively_movable<X> x2;
    x1->test();
    x1 = x2;
    x1 = std::move(x2);

    std::cout << "Hello World!\n";
}

//To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
//  clang -std=c++17 -Wall -Xclang -flto-visibility-public-std destructive_move.cpp


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
