# Ngawi Language

Ngawi is an experimental programming language project. It targets short syntax with native output.

## Project Status

Stage: experimental.

You can build and run real programs, but language design, diagnostics, and code generation can still change between commits.
Do not treat current output as production stable.

Current language surface:

- control flow: `if/elif/else`, `while`, `for`, `match` (int+bool+string MVP), `break`, `continue`
- arrays (MVP): scalar arrays (`int[]`, `float[]`, `bool[]`, `string[]`) plus nested scalar forms (`int[][]`, `float[][]`, `bool[][]`, `string[][]`), with literals, indexing, indexed assignment, `len(array)`, `push`, and `pop`
- operators: arithmetic, comparison, logical, compound assignment, postfix `++`/`--`
- strings: equality by value, concatenation with `+`, `len`, `contains`, `starts_with`, `ends_with`, `to_lower`, `to_upper`, `trim`
- modules: top-level `import "file.ngawi";` with cycle detection

Compiler pipeline in this repository:

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

## Aliases

Ngawi supports base names and aliases.

Type aliases:

- `int` or `amba`
- `float` or `rusdi`
- `bool` or `fuad`
- `string` or `imut`
- `void`

Declaration aliases:

- `let` or `muwani`
- `const` or `crot`

Cast builtin aliases:

- `to_int(x)` or `to_amba(x)`
- `to_float(x)` or `to_rusdi(x)`

## Documentation

- Docs index: `docs/README.md`
- Getting started: `docs/getting-started.md`
- Syntax guide: `docs/syntax-guide.md`
- Language specification: `docs/language-spec.md`
- Compiler architecture: `docs/compiler-architecture.md`
- Testing guide: `docs/testing.md`
- Contributing guide: `docs/contributing.md`
- Roadmap: `docs/roadmap.md`

## Example

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
  muwani ok: fuad = true;
  muwani scale: rusdi = 1.0;
  print(label, 5, "=", value, ok, scale);
  return 0;
}
```
