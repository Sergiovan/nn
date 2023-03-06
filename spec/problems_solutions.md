# Move and copy

How to determine how move and copy works? There's RAII, so there should be a difference between moving and copying, and defaults for both. 

## `move` and `copy`
They keywords `move` and `copy` can be used to move or copy objects, respectively. `move` causes a new temporary to be created via a move, and `copy` causes a new temporary to be created via a copy. Then those temporaries follow normal rules.

Types are copy by default, unless they're marked with `move` or they contain elements that have `move` type. Types that are `move` due to elements can also be marked `copy` to undo this. 

Moved-from objects become invalid and cannot be used anymore unless they're given new values. Custom move and copy mechanisms can be defined for types.

## Destructors and `delete`

When an object reaches the end of its life cycle its destructor is called. This can also be done with `delete`, after which the object becomes unusable. 
```nn
delete a;
a.foo(); // Causes an error, cannot be used
a = 1; // Okay, lifetime restarted
```

`delete` can be user-defined to do funky stuff

# Canonical forms

Canonical forms have no syntax sugar, and are the way the compiler itself sees your code. In a way, at least.

## Structs
Canonical form on top.
```nn
const foo = fun[T: type -> type] {
  return struct {
    a: T;
  };
};
// =======================================
const foo = struct[T: type] {
  a: T;
};
// =======================================
def struct foo {
  a: infer T;
}
```

## Functions
Canonical form on top.
```nn
const foo = fun[T: type, U: type] {
    $type_check(U, (Callable & Mutable) | (u64 | u32 | u16));
    
    return fun(a: T, b: U, c: U -> if U is Callable then typeof(U::call).return_type else U) {
        inline if U is Callable {
            return a + b() + c();
        } else {
            return b + c;
        }
    };
};
// =====================================================
const foo = fun[T: type, U: type](a: T, b: U, c: U) {
    $with {
        U is (Callable & Mutable) | (u64 | u32 | u16);
    }
    
    inline if U is Callable {
        return a + b() + c();
    } else {
        return b + c;
    }
};
// =====================================================
def fun foo[T: type, U: (Callable & Mutable) | (u64 | u32 | u16)](a: T, b: U, c: U) {
    inline if U is Callable {
        return a + b() + c();
    } else {
        return b + c;
    }
}
// ===================================================== (return type simplified to infer)
def fun foo(a: infer T, b: infer U is (Callable & Mutable) | (u64 | u32 | u16), c: U) {
    inline if U is Callable {
        return a + b() + c();
    } else {
        return b + c;
    }
}
// =====================================================
def fun foo(a: infer, b: infer is (Callable & Mutable) | (u64 | u32 | u16), c: typeof(b)) {
    inline if typeof(b) is Callable {
        return a + b() + c();
    } else {
        return b + c;
    }
}
```