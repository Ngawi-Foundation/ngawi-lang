# Syntax Guide

This page describes the current Ngawi syntax.

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

## Type aliases

- `int` / `amba`
- `float` / `rusdi`
- `bool` / `fuad`
- `string` / `imut`
- `void`
