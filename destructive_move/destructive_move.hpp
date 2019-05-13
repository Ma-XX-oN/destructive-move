#pragma once
#ifndef AFH_DESTRUCTIVE_MOVE_HPP__
#define AFH_DESTRUCTIVE_MOVE_HPP__

#include "utility.hpp"
#include <new> // for launder
#include <tuple>
#include <assert.h>
namespace afh {
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
        //OUTPUT_THIS_FUNC;
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
        OUTPUT_THIS_FUNC;
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
        OUTPUT_THIS_FUNC;
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

} // namespace detail
} // namespace afh
#endif // #ifndef AFH_DESTRUCTIVE_MOVE_HPP__
