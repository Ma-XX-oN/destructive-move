// destructive_move.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <utility>
#include <type_traits>
#include <new>
#include <assert.h>

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
// Copies const/volatile from From to To.
//
// Any reference is first removed from From.
template <typename From, typename To> using  copy_cv_t
= copy_rp_t<To, typename detail::copy_cv<std::remove_reference_t<From>, std::remove_pointer_t<std::remove_reference_t<To>>>::type>;

//=============================================================================
// Remove reference, first level pointer and any cv qualifiers.
// Should prolly get rid of array and all pointers?  Should I get rid of pointers?  Dunno...
template <typename T>
using  strip_t = std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;

//=============================================================================
// Used for std::forward to take a type From and copy its cv and rp over to
// type To.
template <typename From, typename To>
using  fwd_like_t = copy_rp_t<std::conditional_t<std::is_lvalue_reference_v<From>, From, From&&>, copy_cv_t<From, To>>;

template <typename From, typename To>
decltype(auto) fwd_like(To&& obj_ref)
{
    return static_cast<fwd_like_t<From, To>>(obj_ref);
}

//=============================================================================
namespace detail {
    template <typename T> struct to_const_impl      { using type = T const & ; };
    template <typename T> struct to_const_impl<T&&> { using type = T const &&; };
    template <typename T> struct to_const_impl<T* > { using type = T const * ; };
}
template <typename T>     using  to_const = typename detail::to_const_impl<T>::type;

//=============================================================================
namespace detail {
    template <typename T> struct to_const_rvalue_impl      { using type = T; };
    template <typename T> struct to_const_rvalue_impl<T&&> { using type = T const&&; };
}
template <typename T>     using  to_const_rvalue = typename detail::to_const_rvalue_impl<T>::type;

//=============================================================================
namespace detail {
    template <typename T> struct to_non_const_impl            { using type = T & ; };
    template <typename T> struct to_non_const_impl<T      &&> { using type = T &&; };
    template <typename T> struct to_non_const_impl<T const& > { using type = T & ; };
    template <typename T> struct to_non_const_impl<T const&&> { using type = T &&; };
    template <typename T> struct to_non_const_impl<T const* > { using type = T * ; };
}
template <typename T>     using  to_non_const = typename detail::to_non_const_impl<T>::type;

//=============================================================================
template <template <typename> class Op, typename T>
auto&& cast(T&& item) {
    return std::forward<fwd_like_t<T, Op<T>>>(const_cast<strip_t<T>&>(item));
}


static_assert(std::is_same_v<fwd_like_t<int const, float&>, float const&&>, "");

// TODO: Will need copy/move constructor/assigment operators
template <typename T, typename Derived>
class destructive_move_container_impl;

template <typename T, typename Is_valid = void>
class destructive_move_container
    : public destructive_move_container_impl<T, destructive_move_container<T, Is_valid>>
{
    using base = destructive_move_container_impl<T, destructive_move_container<T, Is_valid>>;
public:
    void is_valid(bool value)                {        Is_valid(this, value); }
    void is_valid(bool value)       volatile {        Is_valid(this, value); }
    bool is_valid(          ) const          { return Is_valid(this); }
    bool is_valid(          ) const volatile { return Is_valid(this); }

    using base::base;
};

template <typename T>
class destructive_move_container<T, void>
    : public destructive_move_container_impl<T, destructive_move_container<T, void>>
{
    using base = destructive_move_container_impl<T, destructive_move_container<T, void>>;
    bool m_isValid = false;
public:
    void is_valid(bool value)                {        m_isValid = value; }
    void is_valid(bool value)       volatile {        m_isValid = value; }
    bool is_valid(          ) const          { return m_isValid; }
    bool is_valid(          ) const volatile { return m_isValid; }

    using base::base;
};

template <typename T, typename Derived>
class destructive_move_container_impl
{
    Derived                * derived()                { return static_cast<Derived                *>(this); }
    Derived       volatile * derived()       volatile { return static_cast<Derived       volatile *>(this); }
    Derived const          * derived() const          { return static_cast<Derived const          *>(this); }
    Derived const volatile * derived() const volatile { return static_cast<Derived const volatile *>(this); }

    char m_storage[sizeof(T)];

    template <typename U>
    static auto&& object(U&& this_ref) noexcept
    {
        assert(this_ref.is_valid());
        return fwd_like<U>(
            reinterpret_cast<copy_cv_t<U, T>&>(
                *this_ref.m_storage
            )
        );
    }

public:

    template <typename...Ts>
    destructive_move_container_impl(Ts&&...args)
    {
        construct(std::forward<Ts>(args)...);
    }

    ~destructive_move_container_impl()
    {
        if (is_valid()) {
            object().~T();
        }
    }

    template <typename...Ts>
    void construct(Ts&&...args) {
        assert(!is_valid());
        new (m_storage) T(std::forward<Ts>(args)...);
        is_valid(true);
    }

    void destruct() {
        is_valid(false);
        object().~destructive_move_container();
    }

    void is_valid(bool value)                {        derived()->is_valid(value); }
    void is_valid(bool value)       volatile {        derived()->is_valid(value); }
    bool is_valid(          ) const          { return derived()->is_valid(); }
    bool is_valid(          ) const volatile { return derived()->is_valid(); }


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
    //     T&& from &&     | upcast     | true                 ||  explicit  // This will not throw. Use
    //     T&  from &      | upcast     | true                 ||  explicit  // polymorph_dynamic_cast for that.
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

#define IS_DOWNCAST (std::is_base_of<strip_t<U>, T>::value)
#define IS_UPCAST   (std::is_base_of<T, strip_t<U>>::value && !std::is_same<T, strip_t<U>>::value)
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

struct X {
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
int main()
{
    destructive_move_container<X> x;
    std::cout << "\nlvalues\n";
    x->test();

    std::cout << "\nlvalues\n";
    static_cast<X                & >(x).test();
    static_cast<X       volatile & >(x).test();
    static_cast<X const          & >(x).test();
    static_cast<X const volatile & >(x).test();

    std::cout << "\nrvalues\n";
    static_cast<X                &&>(x).test();
    static_cast<X       volatile &&>(x).test();
    static_cast<X const          &&>(x).test();
    static_cast<X const volatile &&>(x).test();

    std::cout << "\nlvalues\n";
    static_cast<destructive_move_container<X>                & >(x)->test();
    static_cast<destructive_move_container<X>       volatile & >(x)->test();
    static_cast<destructive_move_container<X> const          & >(x)->test();
    static_cast<destructive_move_container<X> const volatile & >(x)->test();

    std::cout << "\nlvalues\n";
    static_cast<destructive_move_container<X>                &&>(x)->test();
    static_cast<destructive_move_container<X>       volatile &&>(x)->test();
    static_cast<destructive_move_container<X> const          &&>(x)->test();
    static_cast<destructive_move_container<X> const volatile &&>(x)->test();

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
