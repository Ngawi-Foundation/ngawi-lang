# Ngawi Language

Ngawi targets one goal: write with short syntax and ship native binaries.

## Status

Compiler pipeline in repo:

- Source `.ngawi`
- Lexer
- Parser to AST
- Semantic checks
- C11 codegen
- GCC build to native binary

## Build

```bash
make debug
```

## Run

```bash
./ngawic build examples/hello.ngawi -o hello
./hello
```

## Emit C11

```bash
./ngawic build examples/hello.ngawi -o hello -S
```

Command writes `hello.c`. Generated C uses runtime helpers from `src/runtime/ngawi_runtime.h` and `src/runtime/ngawi_runtime.c`.

## Tests

```bash
make test
```

Test set:

- lexer unit tests
- parser unit tests
- sema unit tests
- end to end compile and run tests
- golden codegen snapshots

Update golden snapshots after codegen changes:

```bash
make update_golden
```

## Example

```ngawi
fn fact(n: int) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * fact(n - 1);
}

fn main() -> int {
  let value: int = fact(5);
  print("fact", 5, "=", value);
  return 0;
}
```
