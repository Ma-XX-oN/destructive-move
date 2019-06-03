# destructively_movable

## Introduction

This library is in responce to the question in [CppCon Grill the Committee](https://youtu.be/cH0nJPbMFAY?t=1263) about having destructive moves added to C++.  I thought that it was an interesting problem, and wondered if a library solution could be made.  So I made this "toy" container to flesh out the possibility and, with the advent of *C++17*, it worked out quite well.

`destructively_movable` is a template container that allows the "dropping on the floor" of an object husk after its contents has been moved to another location.  This means that the destructor for that object is never called which, if it contains many objects, can result in significant performance improvements depending on types of compiler optimisations that are performed.

NOTE: This is still in some development and testing stage, but is in a point where it does work and where I feel can be shown to the community for feedback.

There has been a couple of articles that I found stated by Sean Parent on this subject.  One called [Reply to Twitter Conversation on Move](https://github.com/sean-parent/sean-parent.github.io/wiki/Reply-to-Twitter-Conversation-on-Move) and another called [Move](https://sean-parent.stlab.cc/2014/05/30/about-move.html).  I think the second is a more formalisim of the first, so I'm guessing that that is the order that they occured, though I could be wrong.  In any case, I think what is missing from this idea, is the ability to interrogate the space where the object could reside, to see if an object actually exists there.  If we could do that, then problems of holes in containers which aren't directly controlled by the compiler, can be queried, allowing an appropriate response to be made, but yet still allow elide

## Description

A `destructively_movable` template is a type that hides its Contained type from the compiler, but will call that Contained type's destructor if it is determined that the object isn't constructed (the tricky part).

It has the same size[<sup>[1]</sup>](#caveat-same-size) and alignment of the Contained type and is supposed to be an (almost) drop-in replacement[<sup>[2]</sup>](#caveat-drop-in-replacement) for that type.

The syntax is straight forward:

```c++
struct X { /*...*/ };

afh::destructively_movable<X> x;
```

This will define a variable `x` that is of type X which is destructively movable.  Note that this will work for the trivial case where X is trivially destructible and doesn't add any overhead in that case.

## Tombstoned State

Without any additional code, the previous example will result in the size to be a little larger than `X` would have been on its own.  This is because there is nothing describing an internal tombstone state or how to set it, so it must add an external Boolean to the object to flag that state.  To specify what the internal tombstone state is and how to set it a trait can be specified inside of the class:

```c++
struct X {
  /*...*/
  struct Is_tombstoned {
    bool operator()(X const& x) {
      // Check x for tombstone state
      return /*...*/;
    }
    void operator()(X& x, afh::tombstone_tag) {
      // Set x to tombstone state.  Assumed that x is uninitiaised
      // or that the contents have been moved.
    }
  };
};
```

Alternatively, the trait can be added to a `destructively_movable_traits` specialisation:
```c++
template<>
afh::destructively_movable_traits<X> {
  struct Is_tombstoned {
    bool operator()(X const& x) {
      // Check x for tombstone state
      return /*...*/;
    }
    void operator()(X& x, afh::tombstone_tag) {
      // Set x to tombstone state.  Assumed that x is uninitiaised
      // or that the contents have been moved.
    }
  };
};
```

The trait inside of the class will always take precedence.

What is a tombstoned state?  It is a state that indicates the the object holds no resources and can be destroyed without having to call it's destructor.  Having an external tombstone state has its drawbacks (see caveats[<sup>[3]</sup>](#caveat-external-tombstone)).

# Drop-in Replacment
As this is ment to be a drop-in replacement[<sup>[2]</sup>](#caveat-drop-in-replacement), the constructor of the Contined type is called via perfect forwarding.  So this:

```c++
X x{a, b, c};
```

becomes:
```c++
afh::destructively_movable<X> x{a, b, c};
```

Calls to functions that take `X` don't have to change at all.  So calling `fn(X&)` or `fn(base_of_X&)` will just work as well if you pass an `afh::destructively_movable<X>` object rather than a `X` object.  However, if passing to a function as an rvalue (e.g. `fn(X&& x)`, where `x` will have it's contents moved, then see caveats section[<sup>[3]</sup>](#caveat-external-tombstone).

Implicit casts all work as well, from changing to a stronger cv qualifier, to downcasting.  However, explict casts also work, from changing to a weaker/different cv qualifiers, to upcasting[<sup>[4]</sup>](#caveat-upcasting), to switching between lvalue/rvalue reference types.

The `operator&` is also overloaded to return a cv qualified `Contained*` and if for some reason, the object needs to be retrived in some other way that isn't usable via the other means, then there is an `object()` method that returns a fully ref and cv qualified reference based on the qualifiers of the `destructively_movable` container object.

## Destruction
When a `destructively_movable` object is destroyed, its destructor is still called, but the destuctor will only call the Contained object's destructor if the tombstone marker is set.  So, if this object contains more than one sub-object that have non-trivial destructors, this should cause a slight performance boost.  The more sub-objects, the greater the performance gain.  A moved object that allocates/holds onto resources will not work in this scenario (see caveats[<sup>[5]</sup>](#caveat-hold-resource-after-move))

## Caveats

1. <a name="caveat-same-size"></a>
   The size is only the same if the type has some intenal tombstone marker, otherwise a bool flag will be appended to the object, which isn't optimal, but can do in a pinch.  See caveats[<sup>[3]</sup>](#caveat-external-tombstone).

2. <a name="caveat-drop-in-replacement"></a>
   The structure reference (*`operator.`*) isn't overloadable.  So we must be satisfied with either using the structure dereference (*`operator->`*) to access the Contained object's members or getting the object via cast or function call and performing operations on that.

3. <a name="caveat-external-tombstone"></a>
   It is always better to have the tombstone internal to the object.  Although an external tombstone is allowed, an issue can arise if the Contained object gets moved without the container knowing about it.  If this happens, then the Contained object will still get its destructor called.  So long as the object maintains the "valid but otherwise indeterminate state", this will result in still defined behaviour, although it will negate the performance improvement.\
\
   This performance loss can be regained by calling `has_been_moved()`, which will correct the state and ensure that the validity chaecks out correctly.  It can be called even if it has an internal tombstone without any performance penelty (with optimisations on). If called when the contained object has not been moved, it will most likely result in resource leaks, so don't do that.

4. <a name="caveat-upcasting"></a>
   Upcasting is not available unless the type being casted to doesn't increase in size.

5. <a name="caveat-hold-resource-after-move"></a>
   If, after a move, the Contained object allocates/retains any resources (*cou-MicroSoft-gh! ;)*), then using `destructively_movable` on that object will cause a leak in the moved from object.

   To get around this issue, one can define a two trait `constexpr` values:
   - `is_destructive_move_disabled` is a `bool` can be set to `true` and it will disallow the type being even contained in the template.
   - `destructive_move_exempt` contains member pointers *and* if the member's type has the trait `is_destructive_move_disabled` set to `true`, then it will call the member's destructor.  This would have to be added for any Contained type that contains types that have this issue.

    E.g.
    ```c++
    struct X {
      /*...*/
      static constexpr bool is_destructive_move_disabled = true;
    };

    struct Y {
      X x;
      // afh::destructive_move_exempt(...) is a variadict template function.
      static constexpr auto destructive_move_exempt = afh::destructive_move_exempt(&X::x);
    };
    ```

    Or put in the `destructively_movable_traits` specialisation:
    ```c++
    template<>
    afh::destructively_movable_traits<X> {
      static constexpr bool is_destructive_move_disabled = true;
    };

    template<>
    afh::destructively_movable_traits<Y> {
      // afh::destructive_move_exempt(...) is a variadict template function.
      static constexpr auto destructive_move_exempt = afh::destructive_move_exempt(&X::x);
    };
    ```

    This is really a workaround.  IMO, the standard should be made more specific to say something to the effect that Objects that have been moved, should not retain (or create) *ANY* resources.  Doing so causes problems here and other places (such as move constructor to be not marked as noexcept because of allocating of memory *cough-cough*).

    At least doing it in this way will allow user's of a class to explictly disallow vendor classes from being accidentally instantiated and specify in the user's classes the members that have to be cleaned up.  It also further allows vendors to fix their classes and transperently push the fix to the users by setting `is_destructive_move_disabled` to `false` in their class. This has the effect of stopping the vendor's class destructor from being called when used in a Contained object, without requiring any changes to the user's class by modifying `destructive_move_exempt`.