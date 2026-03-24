# Ngawi Language Spec

This document defines the current Ngawi language surface and compiler behavior.

Ngawi status: experimental.

## 1. Design

Ngawi follows this principle:

**Write like Python, run like C.**

Compiler pipeline:

`Source (.ngawi) -> Lexer -> Parser (AST) -> Sema -> C11 codegen -> GCC -> Native binary`

## 2. Keywords

Core keywords:

- `fn`
- `return`
- `if`, `elif`, `else`
- `while`, `for`, `match`
- `break`, `continue`
- `let`, `const`
- `true`, `false`

Type keywords:

- `int`, `float`, `bool`, `string`, `void`

## 3. Aliases

Ngawi keeps base names and aliases active at the same time.

### 3.1 Type aliases

- `int` or `amba`
- `float` or `rusdi`
- `bool` or `fuad`
- `string` or `imut`
- `void`

### 3.2 Declaration aliases

- `let` or `muwani`
- `const` or `crot`

### 3.3 Cast aliases

- `to_int(x)` or `to_amba(x)`
- `to_float(x)` or `to_rusdi(x)`

## 4. Types

Ngawi currently supports builtin scalar types plus array MVP types:

- `int`
- `float`
- `bool`
- `string`
- `int[]`, `float[]`, `bool[]`, `string[]`
- `int[][]`, `float[][]`, `bool[][]`, `string[][]` (MVP)
- `void`

Backend mapping to C11:

- `int -> int64_t`
- `float -> double`
- `bool -> bool`
- `string -> const char *`
- `void -> void`

## 5. Statements

Ngawi requires semicolons.

Supported statements:

- top-level import directive: `import "file.ngawi";`
- variable declaration (`let`, `const`)
- assignment
- indexed assignment (`a[i] = v`) for scalar arrays
  - target must be an array variable identifier, or a 2D row element access (`m[i][j]`)
- empty array literal `[]` requires explicit array type context
- `import` is top-level only (inside blocks is invalid)
- compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`)
- postfix increment/decrement (`x++;`, `x--;`)
- expression statement
- `return`
- `if / elif / else`
- `while`
- `for`
- `match`
- `break`
- `continue`
- block `{ ... }`

## 6. Expressions and operators

Literals:

- integer
- float
- string
- bool

Operators:

- arithmetic: `+ - * / %`
- comparison: `== != < <= > >=`
- logical: `&& || !`
- assignment: `=` plus compound assignment
- postfix: `++ --` (statement form)

Operator notes:

- `%` requires `int` operands.
- `+` supports `string + string` concatenation.
- string equality uses value comparison in runtime (`==` and `!=` on `string`).

## 7. Functions

Function form:

```ngawi
fn add(a: int, b: int) -> int {
  return a + b;
}
```

Rules:

- parameter types are required
- return type is required
- `return;` only valid in `void` function
- non-void function must return across control paths (basic check)

Entry point rule:

```ngawi
fn main() -> int { ... }
```

## 8. Control flow examples

### 8.1 for-loop

```ngawi
for (muwani i: amba = 1; i <= 3; i = i + 1) {
  print(i);
}
```

### 8.2 break/continue

```ngawi
while (i < 10) {
  i += 1;
  if (i == 2) { continue; }
  if (i == 7) { break; }
}
```

`break` and `continue` are valid only inside loops.

## 8.3 match

Current `match` MVP supports `int`, `bool`, and `string` subjects.

```ngawi
match x {
  0 => { print("zero"); }
  1 => { print("one"); }
  _ => { print("other"); }
}
```

Rules:

- `match` subject must be `int`, `bool`, or `string` in current MVP
- `int` subjects use int literal arms (`0`, `1`, ...)
- `bool` subjects use `true` / `false` arms
- `string` subjects use string literal arms (`"x"`, `"hello"`, ...)
- `_` is wildcard arm and should appear last
- arms after wildcard are rejected as unreachable
- duplicate literal arms are rejected by sema
- `bool` match without wildcard must include both `true` and `false` arms

## 9. Builtins

### 9.1 print

`print(...)` accepts multiple arguments and prints them with spaces.

### 9.2 casts

- `to_int(x)` / `to_amba(x)` accepts `int` or `float`
- `to_float(x)` / `to_rusdi(x)` accepts `int` or `float`

### 9.3 string helpers

- `len(s) -> int`
  - accepts `string` and scalar arrays
- `push(arr, value) -> array`
  - appends value and returns new array
- `pop(arr) -> array`
  - removes last value and returns new array
  - runtime error if called on an empty array
- `contains(s, sub) -> bool`
- `starts_with(s, prefix) -> bool`
- `ends_with(s, suffix) -> bool`
- `to_lower(s) -> string`
- `to_upper(s) -> string`
- `trim(s) -> string`

All string helper arguments must be `string`.

Runtime note: `to_lower` and string `+` create runtime-owned heap strings. Runtime frees them automatically at process exit.
Array indexing uses runtime bounds checks and aborts on out-of-range access.

## 10. Diagnostics

Compiler diagnostics include:

- `file:line:col` prefix
- error level (`error`, `note`)
- source snippet
- caret marker under failing column
- typo hint (`did you mean ...`) for known symbol mistakes

The parser and sema use error caps to reduce cascade noise.

## 11. Current limits

Not implemented yet:

- arrays
- structs
- generics
- classes
- optimizer passes

## 12. Imports

Import behavior in current MVP:

- import is a top-level directive
- import directives must appear before any function declaration
- import path must end with `.ngawi`
- path resolves relative to the importing file
- repeated imports are loaded once
- circular imports fail with an error

## 13. Example program

```ngawi
fn fact(n: amba) -> amba {
  if (n <= 1) {
    return 1;
  }
  return n * fact(n - 1);
}

fn main() -> amba {
  muwani value: amba = fact(5);
  crot label: imut = "fact";
  print(label, 5, "=", value);
  return 0;
}
```
