#pragma once
#ifndef AFH_DESTRUCTIVE_MOVE_HPP__
#define AFH_DESTRUCTIVE_MOVE_HPP__

#include "utility.hpp"
#include <new> // for launder
#include <tuple>
#include <assert.h>
namespace afh {

template<typename T, typename const_tag, typename...Ts>
struct emplace_params;

template <size_t I, typename...Ts>
auto&& get(emplace_params<Ts...>& params);

struct make_lvalue_const_tag {};
struct make_rvalue_copy_lvalue_const_tag {};

//=============================================================================
// template<typename T, typename const_tag, typename...Ts>
// struct emplace_params;
//
//  Used to construct on uninitialised memory by constructing a T object with
//  references to original values and then passing those references to the
//  constructor.
//
//  See template <typename T, typename...Ts> emplace<T>(Ts...) for more
//  details.
template<typename T, typename const_tag, typename...Ts>
struct emplace_params
{
    template <size_t I, typename...Us>
    friend auto&& get(emplace_params<Us...>& params);

    emplace_params(Ts&&...args)
        : ref_storage(std::forward<Ts>(args)...)
    {}

    auto* uninitialized_construct(void* storage)
        noexcept(noexcept(T(std::forward<Ts>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(std::forward<Ts>(args)...);
        }, ref_storage);
    }

    // If "this" is const, then pass the parameters as const lvalue references.
    auto* uninitialized_construct(void* storage) const
        noexcept(noexcept(uninitialized_construct(storage, const_tag{})))
    {
        return uninitialized_construct(storage, const_tag{});
    }

private:
    auto* uninitialized_construct(void* storage, make_lvalue_const_tag) const
        noexcept(noexcept(T(afh::rvalue_copy_or_lvalue_const<Ts>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(afh::rvalue_copy_or_lvalue_const<Ts>(args)...);
            }, ref_storage);
    }

    auto* uninitialized_construct(void* storage, make_rvalue_copy_lvalue_const_tag) const
        noexcept(noexcept(T(static_cast<to_const_lvalue<Ts>>(std::declval<Ts>())...)))
    {
        return std::apply([storage](auto &&... args) {
            return new (storage) T(static_cast<to_const_lvalue<Ts>>(args)...);
            }, ref_storage);
    }

    std::tuple<decltype(std::forward<Ts>(std::declval<Ts>()))...> ref_storage;
};

//-----------------------------------------------------------------------------
// template <typename T, typename const_tag = make_lvalue_const_tag, typename...Ts>
// auto emplace(Ts&& ...args)
//
//  Construct an emplace_params<T, Ts...> object to allow storing the
//  parameters to be passed onto the constructor when calling its
//  uninitialized_construct() member function.  This allows passing all the
//  required information as one parameter to be initiaised elsewhere.  Useful
//  for containers.
//
// Note:
//  If wanting to reuse the emplace_params object, make into a const object
//  and use that.  Otherwise, any rvalues that were there before the first
//  uninitialized_construct() call, will more than likely be moved out from
//  their source.  Calling uninitialized_construct() on a const object will
//  do one of two things, depending on the const_tag.
//
//  make_lvalue_const_tag (default)
//   Causes all the values to be passed as lvalue const references.  As this is
//   the nicest in terms of performance, this is the default.  If, however,
//   your constructor requrires something to be moved in, then you can try the
//   other option.
//
//  make_rvalue_copy_lvalue_const_tag
//   Cause the rvalues to generate a temporary copy and the lvalues will be
//   passed as const.  This may not be the best from a performance POV, but is
//   safer in the event that the rvalues have their guts ripped out and the
//   lvalues are modified after the first iteration.
//
//  If nither of these options sound good, then consider constructing them on
//  the fly.
template <typename T, typename const_tag = make_lvalue_const_tag, typename...Ts>
auto emplace(Ts&& ...args)
{
    return emplace_params<T, const_tag, Ts...>(std::forward<Ts>(args)...);
}

//-----------------------------------------------------------------------------
// template <size_t I, typename...Ts>
// auto&& get(emplace_params<Ts...>& params);
//
//  Gets the Ith stored reference in the emplace_params object.  As this
//  function doesn't care what the types are, it just puts them all into a
//  parameter pack.
template <size_t I, typename...Ts>
auto&& get(emplace_params<Ts...>& params)
{
    return std::get<I>(params.ref_storage);
}

//=============================================================================
namespace detail
{
    template <typename T, typename Derived>
    class destructively_movable_impl;
}

//-----------------------------------------------------------------------------
// struct tombstone_tag {};
//
//  Tag to tell destructively_movable to create Contained type in a tombstoned
//  state.  Also used in the Is_valid function object to tell Contained object
//  to mark itself in a tombstoned state.
struct tombstone_tag {};

//-----------------------------------------------------------------------------
// template <typename T>
// struct destructively_movable_trait;
//
//  Specifies a default function overload object to mark or get valid state of
//  type T.  If set to tombstone_tag, then this explictly states that the type
//  can not be used with a destructively_movable object.
template <typename T>
struct destructively_movable_trait
{
    using Is_valid = void;
};

//-----------------------------------------------------------------------------
// template <typename Contained,
//   typename Is_valid = typename destructively_movable_trait<Contained>::Is_valid>
// class destructively_movable;
//
// template <typename Contained>
// class destructively_movable<Contained, void>;
//
//  The will create a destructively movable container for an object.  This
//  means if this object is moved, the container's contents is also moved. Then
//  the container that was moved from will not call the destructor of its
//  contained object.  This can have performance benifits if the Contained
//  destructor has to call other destructors for other objects that it
//  contains, either iteratively or recersively.
//
////
// Template Parameters
////
//  Contained
//
//   This is the type to be contained.
////
//  Is_valid
//
//   This is an function object, which contains the following two overloads
//   (four if using in a volatile context):
//
//     bool operator()(Contained const         *) noexcept;
//     bool operator()(Contained const volatile*) noexcept;
//     void operator()(Contained               *, tombstone_tag) noexcept;
//     void operator()(Contained       volatile*, tombstone_tag) noexcept;
//
//   The first two will query the Contained object if it is valid. The second
//   two explicitly tell the object to put itself into an invalid state. The
//   second is only called if the destructively_movable object was constructed
//   using the tombstone_tag, or was destructed using the destruct()
//   operation.  A moved object should cause that object to be implictly marked
//   as invalid.
//
//   It is required that the object, when its contents have been moved to
//   another object, if queried if it's valid, that it will respond false.
//   This state doesn't invalidate the "valid but otherwise indeterminate
//   state" mantra.  This tombstoned state is part of that state.  If,
//   however, the class is never used without the destructively_movable
//   template wrapper, then it is concievable that it could programmed such
//   that it might be put in a truely invalid state, where if destructed
//   without the wrapper, the behaviour would be undefined.
//
//   If Is_valid isn't provided, then the compiler will attempt to find one
//   located in the destructively_movable_trait template class under the type
//   Is_valid.  If such a trait class is not defined, the defaut type for
//   Is_valid is void.  That specialisation will append a bool flag to the end
//   of the type, to be used as the tombstone marker, and this flag will be
//   updated as needed.
//
////
// Constructors
////
//  destructively_movable::destructively_movable(Contained      && object_to_move_from);
//  destructively_movable::destructively_movable(Contained const & object_to_copy_from);
//
//   Forwards to destructively_movable_impl::destructively_movable_impl(...).
////
//  template<typename...Ts>
//  destructively_movable_impl::destructively_movable_impl(Ts&&...args);
//
//   Forwards to destructively_movable_impl::construct(...).
////
//  destructively_movable_impl::destructively_movable_impl(tombstone_tag);
//
//   Marks the contained object space or external marker as invalid.
////
//  template <typename...Ts>
//  Derived* destructively_movable_impl::construct(Ts&&...args)
//
//   Forwards to destructively_movable_impl::construct(emplace_params<Contained, Ts>(...).
////
//  template <typename T, typename const_tag, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  Derived* destructively_movable_impl::construct(emplace_params<T, const_tag, Ts...>&& emplace)
////
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
//  template <typename T, typename...Ts
//      , std::enable_if_t<
//          std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
//      , int> = 0>
//  Derived* destructively_movable_impl::construct(emplace_params<T, Ts...> const& emplace)
//   Constructes Contained, or any type that is derived from Contained that is
//   the same size as Contained.
//
////
// Destructors
////
//  destructively_movable_impl::~destructively_movable_impl()
//
//   Calls destructor on Contained only if is_valid() is true.
//
//  void destructively_movable_impl::destruct()
//
//   Calls destructor on Contained object. is_valid() must be true before
//   calling this function.
//  
////
// Assignment
////
// template <typename T> auto&& operator=(T&& rhs)          &;
// template <typename T> auto&& operator=(T&& rhs) volatile &;
//
//  Assigns l or or rvalue reference object of type T to the lvalue Contained object.
// 
// Does it even make sense to assign to a rvalue?  Limited value?
// template <typename T> auto&& operator=(T&& rhs)          && { return assign(std::move(*this), std::forward<T>(rhs)); }
// template <typename T> auto&& operator=(T&& rhs) volatile && { return assign(std::move(*this), std::forward<T>(rhs)); }
//
//  Assigns l or or rvalue reference object of type T to the rvalue Contained object.
//
////
// is_valid getter/setter
////
//  destructively_movable_impl::is_valid(bool value)               ;
//  destructively_movable_impl::is_valid(bool value)       volitile;
//
//   Sets the tombstone state.  If this is an interal tombstone, will only set
//   to false.  A user defined one should only be an internal tombstone value.
//   Setting this external to the class is not a good idea.  May make these
//   private.
//
//  destructively_movable_impl::is_valid()           const         ;
//  destructively_movable_impl::is_valid()           const volitile;
//
//   Gets if the Contained object is constructed (valid).
//
//

////
// Caveats
////
//  It is always better to have the tombstone internal to the object.  Although
//  an external tombstone is allowed, two issues can arise:
//
//  1. If the Contained object gets moved without the container knowing about
//     it.  If this happens, then the Contained object will still get its
//     destructor called.  So long as the object maintains the "valid but
//     otherwise indeterminate state", this will result in still defined
//     behaviour, although it will negate the performance improvement.
//
//  2. A more serious issue is, if after a move, the Contained object retains
//     any resources.  This can occur if a move is done in terms of a "copy and
//     swap" or if resources are reallocated to the moved object
//     (cou-MicroSoft-gh! ;)). In this case, resources will then leak from the
//     moved from object.
//
//  FOR THIS TO WORK WITHOUT LEAKING, the MOVED FROM OBJECT **MUST** NOT RETAIN
//  **ANY **RESOURCES!  Types can be prevented from being allowed in the
//  continer

template <typename Contained,
    typename Is_valid = typename destructively_movable_trait<Contained>::Is_valid>
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

    // Does it even make sense to assign to a rvalue?  Limited value?
    destructively_movable&& operator=(destructively_movable     && obj)          && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj)          && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }
    destructively_movable&& operator=(destructively_movable     && obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(std::move(obj)))) { return std::move(*this).base::operator=(std::move(obj)); }
    destructively_movable&& operator=(destructively_movable const& obj) volatile && noexcept(noexcept(std::move(*this).base::operator=(          obj ))) { return std::move(*this).base::operator=(          obj ); }

    void is_valid(bool value)                noexcept { assert(!value);        Is_valid(this, tombstone_tag()); }
    void is_valid(bool value)       volatile noexcept { assert(!value);        Is_valid(this, tombstone_tag()); }
    bool is_valid(          ) const          noexcept {                 return Is_valid(this); }
    bool is_valid(          ) const volatile noexcept {                 return Is_valid(this); }

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
    // or results in an extra copy.
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

    // Does it even make sense to assign to a rvalue?  Limited value?
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
template <size_t Size>
class storage
{
    std::byte m_storage[Size];
};

template <>
class storage<0>
{
};

template <typename Contained, typename Derived>
class alignas(Contained) destructively_movable_impl
    : private storage<std::is_empty<Contained>::value ? 0 : sizeof(Contained)>
{
    Derived                * derived()                noexcept { return static_cast<Derived                *>(this); }
    Derived       volatile * derived()       volatile noexcept { return static_cast<Derived       volatile *>(this); }
    Derived const          * derived() const          noexcept { return static_cast<Derived const          *>(this); }
    Derived const volatile * derived() const volatile noexcept { return static_cast<Derived const volatile *>(this); }

    template <typename U>
    static auto&& object(U&& this_ref) noexcept
    {
        assert(this_ref.is_valid());

#if 0 // Looks like std::launder isn't working properly atm.
        return fwd_like<U>(
            *std::launder(reinterpret_cast<copy_cv_t<std::remove_reference_t<U>, Contained>*>(
                &this_ref
            )));
#else
        return fwd_like<U>(
            reinterpret_cast<copy_cv_t<U, Contained>&>(
                this_ref
            )
        );
#endif
    }

    template <typename T> struct bare_type_impl                           { using type = T; };
    template <typename T> struct bare_type_impl<destructively_movable<T>> { using type = T; };

    // Strips and extract the wrapped type or if not wrapped, just return the type.
    template <typename T> using  bare_t = typename bare_type_impl<strip_t<T>>::type;
    
    template <typename T> using  fwd_to_bare_type_t = fwd_type_t<T, bare_t<T>>;
 
    template <typename T>
    static void move_from_destructively_movable_implies_no_longer_valid(T&& value) noexcept
    {
        if constexpr (is_destructively_movable<T>::value)
        {
            // NOTE: is_destructively_movable<T>::value implies that
            //       first_element is an rvalue, otherwise it would be an
            //       lvalue reference, which would fail to match.

            // Type passed is a destructively_movable with external tombstone.
            if constexpr (std::is_same<T, destructively_movable<Contained, void>>::value) {
                value.is_valid(false);
            }
            if constexpr (std::is_same<Contained, typename T::type>::value)
            {
                // Operation was a move on a destructively_movable object.
                assert(!value.is_valid());
            }
        }
    }
    destructively_movable_impl(destructively_movable_impl const&) = delete;
    destructively_movable_impl& operator=(destructively_movable_impl const&) = delete;
public:

    using type = Contained;
    destructively_movable_impl(tombstone_tag) noexcept {
        is_valid(false);
    }

    template <typename...Ts>
    destructively_movable_impl(Ts&&...args) noexcept(noexcept(
        construct(std::forward<Ts>(args)...)
    ))
    {
        construct(std::forward<Ts>(args)...);
        assert(is_valid());
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
    Derived* construct(Ts&&...args)
    {
        construct(emplace<Contained>(std::forward<Ts>(args)...));
        assert(is_valid());
        return static_cast<Derived*>(this);
    }

    // Does emplace construction of T
    // If going to accept derived types, then the size of objects must be the same.
    template <typename T, typename const_tag, typename...Ts
        , std::enable_if_t<
            std::is_base_of<Contained, T>::value && sizeof(Contained) == sizeof(T)
        , int> = 0>
    Derived* construct(emplace_params<T, const_tag, Ts...>&& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        //OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (std::is_same<Derived, destructively_movable<Contained, void>>::value) {
            is_valid(true);
        }
        assert(is_valid());
        if constexpr (sizeof...(Ts) == 1)
        {
            using first_element = std::tuple_element_t<0, std::tuple<Ts...>>;
            move_from_destructively_movable_implies_no_longer_valid(
                std::forward<first_element>(get<0>(emplace))
            );
        }
        return static_cast<Derived*>(this);
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
    Derived* construct(emplace_params<T, Ts...> const& emplace) noexcept(noexcept(
        emplace.uninitialized_construct(this)
    ))
    {
        OUTPUT_THIS_FUNC;
        emplace.uninitialized_construct(this);
        if constexpr (std::is_same<Derived, destructively_movable<Contained, void>>::value) {
            is_valid(true);
        }
        assert(is_valid());
        return static_cast<Derived*>(this);
    }

    ~destructively_movable_impl()
    {
        if constexpr (!std::is_trivially_destructible<Contained>::value) {
            if (is_valid()) {
                object().~Contained();
            }
        }
    }

    void destruct()
    {
        assert(is_valid());
        object().~Contained();
        is_valid(false);
        assert(!is_valid());
    }

private:
    template <typename T, typename U>
    static auto&& assign(T&& lhs, U&& rhs)
        noexcept(
            noexcept(std::forward<T>(lhs).object().operator=(std::forward<U>(rhs)))
            && noexcept(lhs.construct(std::forward<U>(rhs)))
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
        move_from_destructively_movable_implies_no_longer_valid(std::forward<U>(rhs));
        return fwd_like<T>(static_cast<Derived&>(lhs));
    }

public:
    template <typename T> auto&& operator=(T&& rhs)          &  { return assign(         (*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile &  { return assign(         (*this), std::forward<T>(rhs)); }

    // Does it even make sense to assign to a rvalue?  Limited value?
    template <typename T> auto&& operator=(T&& rhs)          && { return assign(std::move(*this), std::forward<T>(rhs)); }
    template <typename T> auto&& operator=(T&& rhs) volatile && { return assign(std::move(*this), std::forward<T>(rhs)); }

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
    // chart.  That's 4*8 = 32 functions.  That's a lot of code, with a lot of
    // potential for error.  Because the main body does exactly the same thing
    // (and also happens to be very simple as they just hand off to other
    // functions), using a macro is the safest and clearest way to do this.
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
