# nn as a better C

This document describes the feature set and changes from [basic nn](./0300_BASIC_NN.md) for the first part of the project. For more details see the [roadmap](../ROADMAP.md).

---

## Feature overlap
Features of C that already overlapped with features planned for nn, with syntax changed to be nn-like.

### Basic datatypes
Numeric datatypes only. `u0, u8, u16, u32, u64, s8, s16, s32, s64, u1, f32, f64`, where `u` is an unsigned integer, `s` is a signed integer and `f` is a floating point. The number determines the bit width of each type.

### Declarations
Declaring a defining variables (`var`), functions (`fun`), structs (`struct`), untagged unions (`union`) and C enums (`enum`).

### Pointers
Pointer types as fancy addresses with `*` prepended to type names. Instead of C `NULL` macro use keyword `null` for null pointers.

### Basic flow control
Basic flow control as statements only: `if`, `while`, `match` (instead of C `switch`) and `case`, `goto`, `loop` (instead of C `for(;;)`) and `while` (instead of C `while` and `for`), `break`, `continue` and `return`.

### Literals
Integer and string literals, i.e. numbers and C-like strings as code objects, for example `32` or `"Hello world"`.

### Type aliases
Aliasing types with `using` (like C++) instead of `typedef` (like C).

---

## Differences with C
Features of C that are included with different semantics, or removed altogether. Features that are not in C.

### Extern blocks
`extern C {}` blocks for calling C functions. Provides some sort of ffi.

### Labels
`goto` labels use a `label` keyword for clarity.

### Imports
`import` keyword and semantics instead of textual `#include`.

### Omissions
No ternary operator (`?:` in C)

No default fallthrough in `case`s inside a `match` block

No preprocessor (no textual replacement)

No volatile, register, inline, restrict, static, or any other new keywords that start with _ (e.g. `_Generic`)

No arrays

No arrow syntax for dereferencing struct members

No asm blocks

No bitfields

This list is not exhaustive, C has _a lot_ of features
