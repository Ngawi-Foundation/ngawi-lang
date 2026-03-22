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
- `if`, `else`
- `while`, `for`
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

Ngawi currently supports five builtin types:

- `int`
- `float`
- `bool`
- `string`
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

- variable declaration (`let`, `const`)
- assignment
- compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`)
- expression statement
- `return`
- `if / else`
- `while`
- `for`
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

## 9. Builtins

### 9.1 print

`print(...)` accepts multiple arguments and prints them with spaces.

### 9.2 casts

- `to_int(x)` / `to_amba(x)` accepts `int` or `float`
- `to_float(x)` / `to_rusdi(x)` accepts `int` or `float`

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
- modules/imports
- generics
- classes
- optimizer passes

## 12. Example program

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
