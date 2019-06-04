/// \file
// optional_v2 library
//
//  Copyright Adrian Hawryluk 2019.
//
//  Use, modification and distribution is subject to the
//  MIT License. (See accompanying
//  file LICENSE.txt or copy at
//  https://opensource.org/licenses/MIT)
//
// Project home: https://github.com/Ma-XX-oN/destructive-move
//
#pragma once
#ifndef AFH___UTILITY_HPP
#define AFH___UTILITY_HPP

#include <utility>
#include <type_traits>
#include <tuple>
#include <cwchar>
#include <iostream>

#ifdef _MSC_VER
# define AFH___FUNCSIG __FUNCSIG__
# define AFH___NO_VTABLE __declspec(novtable)
#else
# define AFH___FUNCSIG __PRETTY_FUNCTION__
# define AFH___NO_VTABLE 
#endif

#define AFH___CONCAT_(x, y) x ## y
#define AFH___CONCAT(x, y) AFH___CONCAT_(x, y)
#ifdef __COUNTER__
# define AFH___MAKE_UNIQUE(x) AFH___CONCAT(x, __COUNTER__)
#elif
# define AFH___MAKE_UNIQUE(x) AFH___CONCAT(x, __LINE__)
#endif

#define AFH___INTEROGATE_TYPE__(type, ...)   using type = __VA_ARGS__; nullptr_t type##_var = static_cast<nullptr_t>(type{})
#define AFH___INTEROGATE_TYPE_(type, ...)   AFH___INTEROGATE_TYPE__(type, __VA_ARGS__)
// Generate an error to see what the type that an expression is returning.
#define AFH___INTEROGATE_TYPE(...)   AFH___INTEROGATE_TYPE_(AFH___MAKE_UNIQUE(interrogated_type_), decltype(__VA_ARGS__))
// Generate an error to see what the type is.
#define AFH___INTEROGATE_TYPE_T(...) AFH___INTEROGATE_TYPE_(AFH___MAKE_UNIQUE(interrogated_type_),          __VA_ARGS__ )

// Debugging output macros
#define AFH___OUTPUT_FUNC      (std::cout << __FILE__ << "(" << __LINE__ << "): "                << " " << AFH___FUNCSIG << "\n")
#define AFH___OUTPUT_THIS_FUNC (std::cout << __FILE__ << "(" << __LINE__ << "): " << (void*)this << " " << AFH___FUNCSIG << "\n")

namespace afh {
//=============================================================================
// template <typename...>
// always_false
//
//  A false constant value used for creating a dependency on a template type.
template <typename...T>
constexpr bool always_false = false;

//=============================================================================
// template <typename T0, typename T1>
// constexpr bool is_same_cv_v
//
//  Used to compare cv qualifiers on different types. true if same, false
//  otherwise.
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
// template <typename T0, typename T1>
// using is_stronger_cv_v
//
//  Used to compare cv qualifiers on different types. true if T0 cv qualifier is 
//  stronger than T1, false otherwise.
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

//-----------------------------------------------------------------------------
// template <typename T0, typename T1>
// is_stronger_or_same_cv_v
//
//  Used to compare cv qualifiers on different types. true if T0 cv qualifier is 
//  stronger or same as T1, false otherwise.
//
//  NOTE: I could have had made the symetrical is_weaker_cv_v, but that is just
//        not that useful.  This is the function you most likely want anyway.
template <typename T0, typename T1>
constexpr bool is_stronger_or_same_cv_v = is_stronger_cv_v<T0, T1> || is_same_cv_v<T0, T1>;

//=============================================================================
// template <typename T>
// using remove_cvref_t
//
//  Returns a type without any reference and cv qualifiers.
//  (like in C++20)
template <typename T>
using  remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

//=============================================================================
// template <typename T>
// using remove_refptr_t
//
//  Returns the type T without any reference or first level pointer.
template <typename T>
using  remove_refptr_t = std::remove_pointer_t<std::remove_reference_t<T>>;

//=============================================================================
// tempate <typename T>
// using strip_t
//
//  Returns the type T without any reference, first level pointer and cv
//  qualifiers.
template <typename T>
using  strip_t = std::remove_cv_t<remove_refptr_t<T>>;

//=============================================================================
// template <typename From, typename To>
// using copy_rp_t
//
//  Returns the type To with any reference and first level pointer copied to
//  it from type From.
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
// template <typename From, typename To>
// using copy_cv_t
//
//  Copies const/volatile from type From to type To behind any reference and
//  first level pointer if any.
//
// Any reference or first level pointer in type From is first removed.
namespace detail {
    template <typename From, typename To> struct copy_cv                          { using type = To                ; };
    template <typename From, typename To> struct copy_cv<From const         , To> { using type = To  const         ; };
    template <typename From, typename To> struct copy_cv<From       volatile, To> { using type = To        volatile; };
    template <typename From, typename To> struct copy_cv<From const volatile, To> { using type = To  const volatile; };
}
template <typename From, typename To>
using  copy_cv_t = copy_rp_t<To, typename detail::copy_cv<std::remove_reference_t<From>, strip_t<To>>::type>;

//=============================================================================
// template <typename From, typename To>
// using fwd_type_t;
//
//  Used with std::forward to take a type From and copy its cv and rp over to
//  type To.
template <typename From, typename To>
using  fwd_type_t = copy_rp_t<From, copy_cv_t<From, To>>;

//-----------------------------------------------------------------------------
// template <typename From, typename To>
// constexpr fwd_like(To&& obj_ref) noexcept;
//
//  Used to forward cv and rp from type From to type To.
template <typename From, typename To>
constexpr auto&& fwd_like(To&& obj_ref) noexcept
{
    return std::forward<fwd_type_t<From, To>>(obj_ref);
}

//=============================================================================
// template <typename T>
// using to_const_lvalue
//
//  Used to convert type T to a const lvalue reference.
template <typename T>     using  to_const_lvalue = std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;

//=============================================================================
// template <typename T>
// constexpr decltype(auto) rvalue_copy_or_lvalue_const(std::remove_reference_t<T>& x) noexcept;
// template <typename T>
// constexpr T rvalue_copy_or_lvalue_const(std::remove_reference_t<T>&& x) noexcept;
//
//  Used in a the same way as the std::forward template function. If the passed
//  object is an rvalue, a temporary is generated via the copy constructor. If
//  the object is an lvalue, it will be passed as a const lvalue.
template <typename T>
constexpr decltype(auto) rvalue_copy_or_lvalue_const(std::remove_reference_t<T>& x) noexcept
{
    return static_cast<std::conditional_t<std::is_lvalue_reference_v<T>, to_const_lvalue<T>, T>>(x);
}

template <typename T>
constexpr T rvalue_copy_or_lvalue_const(std::remove_reference_t<T>&& x) noexcept
{
    static_assert(!std::is_lvalue_reference_v<T>, "Do not forward an rvalue ref as an lvalue ref");
    return x;
}
//=============================================================================
namespace detail {
    template <typename...Ts, typename Body, std::size_t...I>
    constexpr void for_each_impl(std::tuple<Ts...> const& tuple, Body& body, std::index_sequence<I...>)
        noexcept(noexcept((body(std::get<I>(tuple)), ...)))
    {
        (body(std::get<I>(tuple)), ...);
    }
}

// template <typename...Ts, typename Body>
// void for_each(std::tuple<Ts...> const& tuple, Body&& body);
//
//  Iterate over each tuple element.
template <typename...Ts, typename Body>
constexpr void for_each(std::tuple<Ts...> const& tuple, Body&& body)
    noexcept(noexcept(detail::for_each_impl(tuple, body, std::make_index_sequence<sizeof...(Ts)>{})))
{
    detail::for_each_impl(tuple, body, std::make_index_sequence<sizeof...(Ts)>{});
}

// template <typename T>
// struct member_type;
// 
// template <typename MT, typename C>
// struct member_type<MT C::*>
//
//  If T is a member pointer, then type is the member's type, otherwise it is
//  void.
template <typename T>
struct member_type {
    using type = void;
};

template <typename MT, typename C>
struct member_type<MT C::*>
{
    using type = MT;
};

template <typename T>
using member_type_t = typename member_type<T>::type;

// Helper macro
#define AFH___GET_P(p, c, m) \
    (reinterpret_cast<char*>(p) + offset(c, m))

// Sets value from within an uninitialised memory area [p, p+1), as if c were
// to have been constructed at p, with the member c::m set to the value v.
//
// NOTE: This only would work if member m is accessible (either public, or set
//       from a function that is a friend or is protected and set from a member
//       function that inherits from class c, or from within class c).
// NOTE: Don't use this on a member that holds onto *ANY* resources when
//       initialised, as it will get overwritten when an actual object of type
//       c is instantiated.
#define AFH___SET(p, c, m, v) \
    (*new (AFH___GET_P(p, c, m)) member_type<decltype(&c::m)>(v))

// Gets value from within partially initialised memory area [p, p+1), as if c
// were to have been constructed at p, returns the value of member c::m.  C++
// object model states that something must have been initialised there, so use
// AFH___SET() macro to do that first.
//
// NOTE: See restrictions in AFH___SET() macro.
#define AFH___GET(p, c, m) \
    (*std::launder(reinterpret_cast<member_type<decltype(&c::m)>*>(AFH___GET_P(p, c, m))))

} // namespace afh
#endif // #ifndef AFH___UTILITY_HPP