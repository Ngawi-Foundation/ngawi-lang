# Syntax Guide

This page describes the current Ngawi syntax.

## Import

```ngawi
import "math.ngawi";
```

Import rules (current MVP):

- import is top-level only
- path is resolved relative to the current file
- path must end with `.ngawi`
- duplicate imports are loaded once
- circular imports fail with an error

## Function

```ngawi
fn add(a: int, b: int) -> int {
  return a + b;
}
```

## Variables

```ngawi
let x: int = 10;
const name: string = "ngawi";
```

Alias form:

```ngawi
muwani x: amba = 10;
crot name: imut = "ngawi";
```

## Control flow

### if / elif / else

```ngawi
if (x > 0) {
  print("positive");
} elif (x == 0) {
  print("zero");
} else {
  print("negative");
}
```

### while

```ngawi
while (x < 10) {
  x += 1;
}
```

### match

```ngawi
match x {
  0 => { print("zero"); }
  1 => { print("one"); }
  _ => { print("other"); }
}
```

Current MVP supports `int`, `bool`, and `string` subjects.
Wildcard arm `_` should be last.
For `bool`, if wildcard is omitted, include both `true` and `false` arms.

```ngawi
match name {
  "ngawi" => { print("ok"); }
  _ => { print("other"); }
}
```

### for

```ngawi
for (muwani i: amba = 0; i < 5; i += 1) {
  print(i);
}
```

### break / continue

```ngawi
while (true) {
  if (x == 2) { continue; }
  if (x == 8) { break; }
  x += 1;
}
```

### increment / decrement

```ngawi
i++;
i--;
```

## Arrays (MVP)

```ngawi
let a: int[] = [1, 2, 3];
let f: float[] = [1.5, 2.5];
let b: bool[] = [true, false];
let s: string[] = ["aa", "bb"];

a[1] = 9;
let x: int = a[1];
let y: float = f[0];
let z: bool = b[1];
let t: string = s[1];
let n: int = len(a);
a = push(a, 10);
a = pop(a);
```

`pop` on an empty array is a runtime error.

Current MVP supports scalar arrays (`int[]`, `float[]`, `bool[]`, `string[]`) and nested scalar arrays (`int[][]`, `float[][]`, `bool[][]`, `string[][]`).
Empty array literal is supported only with explicit type context, for example:
`let a: int[] = [];`
Array indexing performs runtime bounds checks.
Indexed assignment target must be an array variable (for 1D) or a 2D row element access like `m[i][j]`.
Arbitrary expression targets are not allowed.

## Operators

- Arithmetic: `+ - * / %`
- Compound assign: `+= -= *= /= %=`
- Compare: `== != < <= > >=`
- Logical: `&& || !`

String note:

- `+` supports string concatenation when both operands are `string`.

## Builtins

### print

```ngawi
print("value", x);
```

### Cast

```ngawi
let a: int = to_int(3.9);
let b: float = to_float(7);

let c: amba = to_amba(3.9);
let d: rusdi = to_rusdi(7);
```

### String helpers

```ngawi
let s: string = "NgawiLang";
let n: int = len(s);
let c: bool = contains(s, "Lang");
let p: bool = starts_with(s, "Nga");
let e: bool = ends_with(s, "Lang");
let low: string = to_lower(s);
let up: string = to_upper(s);
let t: string = trim("  ngawi  ");
```

## Type aliases

- `int` / `amba`
- `float` / `rusdi`
- `bool` / `fuad`
- `string` / `imut`
- `void`
