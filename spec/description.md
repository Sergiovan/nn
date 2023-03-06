# General description
**nn** (stands for uhhhhhh) is a wannabe systems programming language targetting _exclusively_ RV64GC-none and some other hobby targets later. The purpose hereof is to ~~go insane~~ realize the utopia that is a world without the C language (well, probably not, but maybe a little?). 

I really want to do this, ok? This is just my own thing. 

# Features
* Mistakes.
    * Adherence to backwards compatibility to ridiculous extremes in the current software world has done some damage to the usability and user experience. If something is wrong, it should be rectified and not just patched over. There's a happy medium between never deprecating anything and breaking everything every release that I want to achieve.
* Bad ideas.
    * Can't guarantee everything described here can even work, I'll figure it out as I go. Nobody depends on this, nobody even knows this exists at all tbh.

* No GC.
    * I enjoy the terrifying dread of managing my own stuff.
    * Facilities to avoid errors like owned pointers, non-nullable pointers, RAII, defer, proper arrays, implicit safety checks, etc.
* No inheritance, virtual methods, prototypes, etc.
    * But ways to create inheritance-like patterns.
    * Traits?
* No real function overloading.
    * Every function should have a unique name/path, although facilities should exist to call functions by the same name.
    * Dyanmic, runtime overloading on single parameters?

* Sensible operator precedence.
    * None of that C nonsense.
* Operator overloading.
    * Infix expressions can help readability in a lot of contexts, and it should be allowed for the good cases, rather than prohibited because of bad cases.
    * Custom operators? Sounds like a terrible idea, but FUN.
* Assignment is not an expression.
    * Assignments and variable declarations are statements and cannot be done by accident.
* First class types.
    * Types are merely expressions and can be modified by functions or operators. This also makes generics quite easy.
* First class errors.
    * Specific error type(s) and error handling syntax. 

# Types and Literals
There are literals available for all numeric types, character types, the boolean type, optional types, arrays (with special string literal syntax), tuples, structs, unions and enums. There's also identifier literals and a void literal.

## Integers
Integer types refer to whole numbers. They're separated by their size and signedness, named with an `s` for signed or `u` for unsigned, followed by the number of bits they occupy. The basic built-in integer types are `u8`, `u16`, `u32`, `u64`, `s8`, `s16`, `s32` and `s64` (and maybe `s128` and `u128`). More can be created specifying a signedness and size via TBD, which is useful for packed structures for example.

Integers are derived from the `Integer[unsigned: bool, size: u64]` type. 

### Literals

Numbers are decimal (base 10) integers by default: `12345`

Numbers can be separated with underscores: `12_345`

Hexadecimal (base 16) numbers start with `0x`. Hexadecimal letters may be upper or lowercase: `0xaAbB123`

Octal (base 8) numbers start with `0o`: `0o777`

Binary (base 2) numbers start with `0b`: `0b101010`

Integer literals are of type `integer_literal` and can be implicitly converted to any other integer type that can contain them. They can also be explicitly converted via `as`. If no conversion or inference is available they default to an unsigned integer of enough size. 

Integer literals can also convert to floating point types that can contain them.

## Floating point numbers
Floating point types refer to a subset of real numbers. They're separated by their size, named with a `f` followed by the number of bits they use. The basic built-in floating point types are `f32` and `f64` (and maybe `f16` and `f80` and `f128`?). More can be created by specifying a size via TBD.

Floating point numbers are derived from the `Float[fsize: u64]` type, which is an alias for `Real[size: u64 = fsize, point: ?u64 = null]`.

### Literals

Floating point literals contain a `.`: `1.0`.

Floating point literals are of type `float_literal` and can be implicitly converted to any other floating point type that can contain them. They can also be explicitly converted via `as`. If no conversion or inference is available they default to `f64`. 

## TBD
Fixed point numbers? Or perhaps this is not built-in, but via `as`? `Real[size: u64, points: ?u64]`?

## Characters
Character types refer to text characters, as represented by a computer. They're separated by their size, named with a `c` followed by the number of bits they use. The basic built-in character types is `c32` (and maybe `c8` and `c16`?). More can be created by specifying a size via TBD. (but like, is there a reason to?)

Characters are derived from the `Character[TBD]` type.

### Literals

Character literals are UTF-8 encoded Unicode scalar values (from https://www.unicode.org/versions/Unicode10.0.0/ch03.pdf#G7404), surrounded by `'""`: `'"a"`

Character literals are of type `character_literal` and can be implicitly converted to any other character type that can contain them. They also can be explicitly converted via `as`. If no conversion or inference is available they default to `c32` (As unicode codepoints go as high as 0x10FFFF)

## Strings
String types represent text in a variety of encodings (usually UTF-8, sometimes ASCII).

Strings are simply immutable 0-terminated arrays of bytes (might change in the future). Their type is `[..N;0]byte` (only for now)

## Character escapes
To represent all possible values better, escape codes are available. They can be used inside strings or characters. These sequences start with ``` ` ``` and can be:
- ``` `n```: Newline character
- ``` `t```: Tabulation character
- ``` `r```: Carriage return
- ``` `` ```: Literal character ``` ` ```
- ``` `xXX```: Hexadecimal byte
- ``` `uNNNNNN```: Hexadecimal unicode code point (utf-8 encoded)

### Literals

String literals are unicode text surrounded by double quotes: `"A string"`.

The type of string literals is `string_literal` and can be implicitly converted to a variety of character and byte arrays. They can also be explicitly converted via `as`. If no conversion or inference is available they default to `[..N;0]byte` (TBD)

### TBD

Strings can be encased in a special type that allows for easy unicode manipulation via some special suffix or prefix

Raw strings without escapes in some form

String interpolation, obviously

## Bytes
Bytes represent data as it exists in memory. There's only one byte type, `byte`, which always takes 8 bits of space (NB: There are architectures where this is not the case, but it's not a goal to target them). TBD `b8`, `b16`, etc? `Bits[TBD]`?

Bytes cannot be used for arithmetic but they can be used for logical operations. 

## Booleans
Booleans represent exactly 2 states, `true` and `false`. The only boolean type is `bool`. It is 1 bit in size.

There are two boolean literals, `true` and `false`.

## Pointers
Pointers store the location of other values. They allow you to reach values of other types through indirection. Pointer types are created by adding `*` in front of a type: `*u64` is a pointer to a `u64` value. Pointers to values can be created with the address-of operator `&`: `&some_val`. Pointers can be dereferenced with the dereference operator `@`: `@some_ptr`.

Pointers are derived from the type `Pointer[T: type, sentinel: ?u64]`

## Arrays
Arrays are collections of elements of the same type. They are generally composed of a length and some elements. The size of an array is always fixed.

Array types are created by adding `[]` in front of a type: `[]u64` is an array of `u64`. A number between the brackets defines an array with specified, known at compile time length: `[4]u64` is an array of exactly 4 values of type `u64`.

Additionally, a terminator can be added. Arrays with terminators don't store their length explicitly, but have an extra element with the value of the terminator, which explicitly marks the end of the array. This value cannot be modified directly. For example, `[4;0]byte` is an array of 4 bytes that ends at the first 0, `[;0xff]u8` is an array of `u8` of inferred length that ends at the first `0xff`.

Arrays are derived from the `Array[T: type, size: u64, terminator: ?T]` type.

### Literals
Array literals are created by putting zero or more values inside an array literal block: `'[]`, `'[1, 2, 3, 4]`, `'["hi", "hello", "uwu"]`

## Slices
Slices are constrained parts of other arrays. They're a bit like arrays and a bit like pointers. 

Slice types are created by adding `[..]` in front of a type: `[..]u64` is a slice of `u64`. Slices can be marked with their length, in which case it'll always be known at compile-time as well: `[..100]u64` is a slice of 100 `u64`. All slices of this type have a known length, sometimes at runtime only. 

Slices can also be of unknown size, in which case they behave like a pointer that can be indexed through: `[*]u64` is a slice of an unknown amount of u64.

Just like arrays, slices can have terminators, which can be useful especially for unknown sized arrays.

Slices with a known size (at compile or runtime) are derived from the `Slice[T: type, size: ?u64, terminator: ?T]` type. Slices with an unknown size are derived from the `UncheckedSlice[T: type, terminator: ?T]` type. 

## Ranges
A range is a compact way of storing contiguous datapoints. It contains a `start`, a `step` and an `end`.

The range type is `Range[T: type]`, and can be created for some types if needed TBD. Also some module has shorthands for all built-in types that allow it, also TBD.

Ranges can be quickly constructed with the `..` (end not included) and `..=` (end included) operators operator: `1..10` is a range from 1 to 10 (10 not included), and `1..=10` is a range from 1 to 10 (10 included) 

By default ranges can be made for all numeric signed and unsigned type, and for all character types.

## Structs
A struct is a record type, a collection of elements of any type. 

The type of struct literals is an anonymous struct, created for the literal, that can be cast to any struct that has the same structure and no private fields, or a custom cast function defined from an anonymous literal. 

Struct elements can then be accessed with the access operator `.` and the variable access operator `.[]`: `a.b; a.0; a.["b"]; a.[c];`. Structs can also be swizzled with the `.{}` operator. This returns some or all elements in a different order as a struct, for convenience: `a.{b, 0, ["b"], [c]}`. Structs can also be spread with the `...` operator.

Struct types are declared with the keyword `struct`: `struct {a: u64, _: u64}`. Structs can take constant parameters by declaring them within brackets: `struct foo[T: type, N: u64] {a: [N]T};`. 

Struct data order does not have to be declaration order. There's a way to make declaration order be the data order too. There's a way to make the structs tightly packed. 

### Literals
Structs are created with struct literals: `'{}`. Struct literals may contain values, name-value pairs or a mixture of both, although all name-value pairs must come after all values without a name, similar to function calls: `'{1, 2, 3, bim = 9, sam = 10}`. Struct literals can be implicitly or explicitly cast to any compatible struct. 

## Enums
Enums are simple lists of tags, each with a unique value for that enum. 

Enums types are created with the keyword `enum`: `enum {one, two, three}`. To access the enum tags, the scope operator `::` is used: `some_enum::some_tag`. Enum tags can be brough into the current scope via the `using` keyword: `using some_enum::*`, `using some_other_enum::{a, b, c}`

There's a way to make enums convert to unsigned integers implicitly. There's a way to make enums into "flags". There's ways to make enums with other representations than numeric. There's ways to give enums values, probably.

### Literals
Enum literals are simply identifier literals: `'ababa`, `'uwu`, `'unsigned_integer`. They must be converted to a specific enum type either explicitly or implicitly, otherwise this is an error.

## Unions
Unions are objects that hold one of several datatypes. They have a tag that tells the program which of the objects is being held.

Union types are created with the keyword `union`: `union {None, Some(u64), Many(struct {u64, u64, u64})}`. Union tags act like enums: They can be accessed via the scope operator, brought into scope or act as literals in some circumstances. 

Union types have a `tag` member that is always the current active tag. Union values cannot be used until they've been unwrapped. 

There's a way to make unsafe unions without a tag. These unions cannot be matched against and don't create a tag enum. Access to elements is unchecked and it works more like a struct in that sense. C union.

Unions don't have literals. To create one you call the tag like a function: `Some(5)`, `Many('{1, 2, 3})`. If the type can only unambiguously be associated with one union element, then it does not need the tag to be explicitly mentioned: `var foo: union {None, Some(u64)} = 5;`

## Functions
Function types have the same syntax as functions, but without the code block: `fun[T: type](a: T, b: T -> T)`. See [functions](#Functions-1)

## Optionals, `null` and `null_type`
Optionals are types that can either have a value, or be null. Null optionals have the value `null`, which is the only valid use of `null`. `null` is a singleton of type `null_type`, which implicitly converts to any Optional type's null value. 

The symbol `?` is associated with optionals. Putting it in front of a type makes it an optional type, which works as a union of type and an empty value (this empty value is dependent on what the type is, but can always be converted to `null` and back). `?u64` can be either `null` or a `u64`

Optionals always have to be dealt with before being usable. It's not possible to pass an optional type as if it were the underlying type. 

There's several ways of dealing with optionals. Given an optional `let opt: ?u64 = /* Some value*/;`:
- Nullish coalescence, supplying a default value in case the optional is null. `opt ?? 1;`
- Bubbling up the `null`, which is sent up the call chain. `opt?;`
- Checked access, which is always `null` on null optionals, and returns an optional of the type being accessed. `opt??.prop` or `opt??[0]`
- Pattern matching with `if let`: `if let a: u64 = opt { /* value in a */ } else { /* opt is null */ }`

## Errors and `err`
Errors are represented by the `err` primitive and generally works like any other type, except for its relationship with functions. Errors signify exceptional circumstances that are not meant to happen usually. As such, they should be dealt with always. 

The symbol `!` is associated with errors. Putting it in front of a type makes it a union of an error and that type. `!u64` can be either an error or a `u64`

Errors are returned from functions with the special keyword `raise`. Any identifier can be used in a `raise` statement, and it'll be treated as the name of an error. If an error is the normal return of a function, then `return` can be used as normal. 

There's a couple of ways of dealing with errors. Given a function `f() -> !u32`:
- Executing a different code branch, possibly depending on the error type. `let a: u32 = f() catch e { ... };`
- Supplying a default value to replace the result of an erroring function. `let a: u32 = f() !! 0;`
  - This is equal to `let a: u32 = f() catch _ => 0;`
- Bubbling up the error, which is sent up the call chain. `let a: u32 = f()!;`
- Pattern matching with `if let`: `if let a: u32 = f() {} else {}
- Unwrapping the error while ignoring the return value. This is similar to `if let/else` but you also get the error value in the `catch` block. `try a: u32 = f() { /* value in a */ } catch b { /* error in b */ }`

## `nothing` and `void`
The `nothing` keyword is a singleton of type `void`, which has size 0, and indicates that something has no value. Functions use this as return type to indicate they have no return: `f() -> void`. Arrays of `void` and pointers to `void` have no size either. `nothing` can be cast to any other type, meaning no initialization of any kind.

## `never` and `no_return`
The `never` keyword signifies a code path cannot happen. It is a singleton of type `no_return`, which is a built-in type. It can be used to optimize code in the face of invariants that the compiler cannot know about. `if not x then never else { // Do something with x }`

`never` can also be used to signify a function does not return. `fun() -> no_return; // This function forcibly aborts execution`. Some expressions also behave as if they returned `never`, meaning they don't follow normal execution flow, like `return`, `break` or `continue`. The same can be achieved by specifying the return type as `no_return`, and in fact that's what the function will show as its return type.

# Functions 
Functions are reusable code blocks. 

Functions are created with the `fun` keyword. There's three main parts to a function: Constant (compile-time) parameters and returns, encased in `[]`, normal parameters and returns, encased in `()`, and the code block, encased in `{}`. For example:
```nn
fun[T: type](a: T, b: T -> T) {
    return a + b;
}
```
Instead of a code block a function can have an arrow `=>` followed by an expression, which is the return of the function. In this case the function is terminated with a semicolon:
```nn
fun(a: u64, b: u64) => a + b;
```
Functions can be given a name by writing it between `fun` and the first parameter block.

A parameter is composed of zero or more specifiers, optionally a name/pattern, TBD spreads and variadics, and a type. Parameters come first and are separated by a comma. The return type comes after an arrow `->` and is also composed of zero or more specifiers, optionally a name/pattern, and a type. If there's no return type it is inferred from the function returns.

Either the constant parameters or the normal parameters may be omitted, but not both. These function types are equivalent: `fun[]`, `fun()`. 

Some function modifiers can appear between the keyword `fun` and the name. 

Functions inside structs, unions or enums can take an extra `self` parameter as a first parameter, which makes the function a member function. Additionally, functions inside structs, unions or enums have access to `Self`, the type of the current container.

## Lambdas
Lambdas are functions that also have data contained within them. 

## Traits/Type classes
TBD

# Flow control
All statements without a specified type are of type `void`

## blocks and names
Blocks separate code into scopes. Variables can access data and functions from higher scopes, but not lower scopes. They're created with braces: `{}`.

Expression blocks also separate code into scopes, like normal blocks, but additionally can yield a value from within them, or be broken out of. They're created with the keyword `do`: `do {}`. Expression blocks can be named with an identifier preceded by a hash: `do #a {}`. To yield a value use the `yield` keyword: `yield 1`, `yield #name 1`

```nn
let a = do { let b = 2; yield b * b; };
let c = do {
    let d = some_complex_function();
    yield d * a + d * 2;
};
```

## if
If statements execute code conditionally. It has a condition, which may or may not pass, and depending on the condition the block after it executes: `if condition { /* block */ }`. An optional `else` block can be appended, which will run when the condition does not pass. This block may contain another `if`, and in this way they can be chained: `if x { } else if y { } else { }`. In case of ambiguity, `else` blocks always bind tightly, with the most recent orphaned `if`.

Conditions can have a predicate and any number of declarations before it, and separated with semicolons: `if decl; decl; predicate {}`. If the declarations are non-exhaustively pattern-matched then the condition fails the declaration's pattern-matching fails, in addition to checking the predicate. See Pattern Matching. The predicate must be convertible to `bool` as if by an `as` conversion, or of `bool` type. The result of that conversion is evaluated and used to determine if the code block is executed.

Ifs come in 2 flavors, the normal `if` and the ternary expression `if-then`.

### Normal if
This if can be turned into an expression by naming it, otherwise it's a statement: `#ret if a { yield b; };`. All branches of an if have to return the same type. The code block is always surrounded by braces.

### If-then
The ternary expression `if-then` has the form `if condition then expression`, with optional `else` and `if-then` chaining. It's always an expression, and always returns the value of the expression it executes. 

## while
While statements execute code in a loop while some condition is true or broken out of. It has a condition and a block of code that is repeated. `while condition { /* block */ }`. The condition always has a predicate that can be prepended with any number of declarations, prepended to the condition and separated by semicolons. It may also have any number of update statements, appended to the condition and separated by semicolons. `while decl; decl; predicate; update; update; {}`. Like with an `if`, the predicate must be convertible to `bool` as if by an `as` conversion, or of `bool` type. The result of that conversion is evaluated and used to determine if the code block is executed. Can the declarations also work like an if? TBD.
Before the loop, the declarations are executed and added to the while's scope. Then the predicate is checked. If true, the loop executes once, then the update statements are executed, then the predicate is checked again, and it repeats like that. When the predicate check does not pass, the loop ends and the program continues. 
Using `break` inside a while loop unconditionally ends the loop without checking the predicate again. Using `continue` is equivalent to jumping to the very end of the loop: update statements will be executed after, and the predicate will be checked again. 

While loops can be appended with a `then` block, which always executes once when the while predicate does not pass. This means that it does not execute if the loop is unconditionally escaped with `break` or `yield`. Additionally, `then` blocks may have their own `while` appended, to chain loops. This has the advantage that the declarations of the previous while condition are still accessible. `while let x = false; let y = true; x { } then while y { /* will loop infinitely */ }`

If the `do` keyword is put before a while loop, then the order of execution changes slightly: The condition's declarations are executed, but the body of the loop is run without checking the predicate. After that the loop continues like usual. `do while false { /* will execute once */ }`. 

If the while is named it becomes an expression and the loop has to return a value. In this case, the return must always occur, and it has to be the same for all the blocks.

## loop
Loop statements execute code in a loop, forever, until broken out of via `break`, `return`, `yield` or `raise`. `loop { /* Runs forever */ }`. 

A loop statement can be named to become an expression, and must return a value via `yield`.

## match
Match statements evaluate an expression and then run code depending on its value. The value is pattern-matched against different branches, and if one of them matches, that code block is run. 
`match value { /* cases */ }`. Each case is the keyword `case` followed by a pattern, then either an arrow with a value, or a block with code: `case 1 => 0`, `case 1 { x(); }`. Cases are separated by commas if they're arrows, otherwise no separation is needed. The special `else` case runs if no other has run. All branches of a match expression must return the same type. A single case may have multiple patterns separated by commas.

For pattern matching see the corresponding section.

## for
For-each, TBD.

## break
The `break` keyword exits a `for`, `while` or `loop` block without returning a value and without triggering `then` blocks. It can be provided with a name to break out of an outer loop. `break`, `break #outer`. `break` expressions have type `no_return`

## continue
The `continue` keyword triggers a new iteration of a `for`, `while` or `loop` block. It can be provided with a name to continue an outer loop. `continue`, `continue #outer`. `continue` expressions have type `no_return`

## yield
The `yield` keyword returns a value from an expression block, an if expression, a while expression, a loop expression, a match expression or a for expression. It can be provided with a name to yield a value from an outer block or expression. By default it always yields out of the innermost enclosing expression block, not from statements. `yield 2`, `yield #outer 3`. `yield` expressions have type `no_return`, but they affect the return type of the block they're yielding from. 

## return
The `return` keyword returns a value from a function or lambda. `return 0;`. `return` expressions have type `no_return`, but they affect the return type of the function they're in.

## raise
The `raise` keyword returns an error from a function or lambda. It generates unique errors which can be scoped variably TBD. Error messages TBD. `raise` can only be used with functions that return error types.

# Declarations
A declaration creates a name and binds it to a value. 

## let
The `let` keyword binds a runtime value to one or more names. It requires a pattern/name and a value, and can optionally contain a type and several specifiers for the name. `let mut a: u64 = 64;`. 

The special name `_` creates an anonymous variable that cannot be referenced and is only bound for its side effects. If the type is not provided it is inferred.

### Specifiers
- `mut` makes the value referred to by a name mutable. 
- `ref` makes the value a reference TBD.
- `TBD` makes tbd tbd tbdbbdbdbd

## def
Shorthand for defining types and functions. 
`def fun a ...` is equivalent to `const let a = fun ...`. `def struct b ... ` is equivalent to `const let b: type = struct ...` and so on.  tbd `def type` (for types that are represented the same but are not convertible), `def enum`, `def union`

## alias
Creates a new name as an alias for another name. It does not create additional values, and is simply for convenience. `alias int = s32;`

# Imports and visibility
## import
`import a;` Simple import

`import b::c;` Single item import

`import d::*;` Import all

`import e::{f, g, *, this};` Multiple import

`import h::{i::{j, k}, l};` Multiple nested import

`import "file.nn";` File import

`import "file.nn"::m;` File single item import (multiple and multiple nested skipped)

`import n as p;` Simple import with alias

`import q::r as s;` Single item import with alias

`import t::{u as v, w as x, this as y};` Multiple import with alias

`import z[...];` Parametrized import

## export
`export name;` Simple export

`export def fun ...`

`export const ...`

`export let ...`

`export(api) name;` Library export

etc

## pub
Mark fields in structs as public and accessible outside the module they're declared in.
`struct { pub x: u64, pub def fun so_and_so() { }, y: f32, def fun none_of_this() { } }`

# Operators
We'll see.

# Pattern matching
Very TBD. `as` to bind groups with names. `is` to pattern match types. `if` for extra conditional binding

`let {x, y, z} as s = a` Struct Bind by value

`if let {x = other_value, y as ddd = 0, 42} = a` Compliex Struct bind 

`let {x, y, z} as s = ref a` Struct Bind by reference

`let {x, ref y, z} as s = a` Struct mixed binding

`if let Name(x) = a` Union conditional binding

`if let Name({x, [y, z, ...], ...}) = a` Union conditional nested binding

`let [x, xs...] = a` Array binding

`let [x, 0, y, ...] as s = a` Array binding

`if let [x, xs...] = a` Slice binding

`if let x is u64 = a` any, optional, error union or unambiguous union binding

Numbers and characters can pattern match into ranges of numbers or characters of the same type. All values can pattern match with other values of the same type (equality comparison)

# const
The const keyword marks code as compile-time. It can be used to run code at compile time, or mark functions/values as accessible during compile-time. 
`const { x(); };`

`const let a = 1;`

`let data = const read_file(some_file);`

