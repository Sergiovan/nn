# Basic nn features

This document loosely explains all basic nn features on top of the features already descibed in [nn as better C](./0100_BETTER_C_NN.md)

All of these are subject to rethinking. Like, hard rethinking. Like, holy moly I'm writing these down and it's not all good in the hood.

## Extra types
### any
The `any` type can be used to store data of an unspecified type and pass it around. This data cannot be used until it has been cast to a specific type. This is not a primitive, reliant on heap memory. Perhaps a stack-based version can be available somehow too.

### Character
Character types `c8, c16, c32` store character data of 8, 16 or 32 bits respectively. I don't want to think about encodings yet so that's all for now.

### Error
Error type `e64` is used to encode error codes for function returns. This also might need to be rethought but it's staying here for now.

### Tuples
Tuple types group heterogeneous data together into an immutable package. 

### Arrays
Array types group homogeneous data together into a mutable package. They come with length information, accessible via `TBD IDK Please help`

### Type type
The `type` type is used to identify types, and access type information of types. New types can be created at compiletime by operating on this type, but that's not something for right now.

## Control flow
### if, for, while, match, block expressions
All these are expressions, not statements. Blocks now have types and can return values. 

### Named blocks
Blocks can be named for easy exit or value returning.

### break
`break` can be used to return values from blocks, or break from named blocks, not just the innermost block.

### continue
`continue` can be used with a name to continue a specific loop, instead of just the innermost one.

### raise
`raise` keyword used to specify error-returns with extra data usable by error types.

### defer
`defer` keyword to run code at the exit point of a block, rather than where it is written. Code that is deferred is evaluated when it is executed, not at the point of `defer`. Useful for doing cleanup.

### try catch
`try` block automatically checks error types from functions returned inside of itself without needing to manually scaffhold the code. `catch` block inspects the error received and runs code based on it, like a `match` block.

### for
`for` statements are exclusively for usage with arrays and other "traversible" data. It iterates over the data in order.

### Initializers
`if`, `while` and `match` get initializers, where variables just for the scope of the block can be declared.

## Operators
### Multiaccess
`.()`, `.[]` and `.{}` operators to mix, reorder and take multiple things at once. 

### Select
`::[]` operator to, like, select things statically of some sort? I gotta revise this too. 

### typeof
The `typeof` operator returns the type of an expression, without evaluating the expression. 

### Bit operators
Operators to set (`@|`), clear (`@&`), toggle (`@^`) and check (`@?`) specific bits of a value. 

## Other
### infer
Type inference via no type annotations or the `infer` keyword. 

### Namespaces
Namespaces with the `namespace` keyword add additional named scoping for types, variables and functions so they don't collide with other names that might be the same scope.

### Name imports
Lift names out of namespaces or modules (tbd) into the current scope.

### Uniform call syntax
Calling functions and calling methods is equivalent. `a.b()` is equivalent to `b(a)` (mostly)

### Zero-initialization and void
Everything is zero-initialized by default, unless other behavior is specified. Keyword `void` is used to avoid this behavior if needed.

### Literal identifiers
For compatibility or ease of use, literals can also be identifiers with special syntax `$""`.

### Multiple return
Functions can return multiple values. But like, this really is just tuples + destructuring. I think this can be scrapped, we'll see.

### Nested functions
Functions can be defined inside of other functions. They are then accessible inside that function only. 

### Lambdas
Anonymous functions that can capture values and be passed around to other functions. 

### Function captures
Functions cannot use external variables unless they are explicitly captured. That means functions don't have side effects by default. This is a TBD tbh.

### Destructuring
Allows lifting values out of a structure, tuple or array.

### Anonymous objects
Objects, more specifically variables, need not have a name, discarding whatever operation was to be done on them. The special keyword `_` can be used to denote lack of name, and this cannot be referenced otherwise.

### Methods
Structs, unions and enums can have methods which have an implicit `this` parameter, refering to whichever object it's being called on. These methods can access the object they're called on implicitly.

### this and This
`this` is a keyword that refers to the object a method is being called on. It can also be used to determine qualifiers for the object being called (like `const`). `This` is the type of `this` (shorthand for `typeof(this)`).

### Named parameters
Calling parameters out of order, via their names. But like, this has some implications I hadn't thought of before for APIs, so maybe reconsider this.

#### Tangent
So like, the reason this might be a bad idea is that changing a parameter name then constitutes an API break, because someone might be using that API? And like, as much as I don't care for people bitching about this I'd still like it to happen the least amount possible. But there could be a tool that does a language-dependent diff on code, and then you can distribute those diffs and they can be used to fix API breaks. I feel like that could work with the proper tooling. I gotta revisit this later.