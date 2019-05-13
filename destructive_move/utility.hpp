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

// Generate an error to see what the type that is being dealt with.
#define INTEROGATE_TYPE(x) static_cast<decltype(x)>(nullptr)

// Debugging output macros
#define OUTPUT_FUNC      (std::cout << __FILE__ << "(" << __LINE__ << "): "                << " " << FUNCSIG << "\n")
#define OUTPUT_THIS_FUNC (std::cout << __FILE__ << "(" << __LINE__ << "): " << (void*)this << " " << FUNCSIG << "\n")

namespace afh {
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

} // namespace afh
#endif // #ifndef AFH_UTILITY_HPP__