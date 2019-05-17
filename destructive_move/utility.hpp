#pragma once
#ifndef AFH_UTILITY_HPP__
#define AFH_UTILITY_HPP__

#include <utility>
#include <type_traits>

#ifdef _MSC_VER
# define FUNCSIG __FUNCSIG__
# define NO_VTABLE __declspec(novtable)
#else
# define FUNCSIG __PRETTY_FUNCTION__
# define NO_VTABLE 
#endif

// Generate an error to see what the type that an expression is returning.
#define INTEROGATE_TYPE(...)   static_cast<decltype(__VA_ARGS__)>(nullptr)
#define INTEROGATE_TYPE_T(...) using type = __VA_ARGS__;           static_cast<nullptr_t>(type{})

// Debugging output macros
#define OUTPUT_FUNC      (std::cout << __FILE__ << "(" << __LINE__ << "): "                << " " << FUNCSIG << "\n")
#define OUTPUT_THIS_FUNC (std::cout << __FILE__ << "(" << __LINE__ << "): " << (void*)this << " " << FUNCSIG << "\n")

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
//  Returns a type with reference and any cv qualifiers removed.
//  (like in C++20)
template <typename T>
using  remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

//=============================================================================
// template <typename T>
// using remove_refptr_t
//
//  Returns the type T with any reference or first level pointer removed.
template <typename T>
using  remove_refptr_t = std::remove_pointer_t<std::remove_reference_t<T>>;

//=============================================================================
// tempate <typename T>
// using strip_t
//
//  Returns the type T with any reference, first level pointer and any cv
//  qualifiers removed.
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

} // namespace afh
#endif // #ifndef AFH_UTILITY_HPP__