# Compiler Architecture

Ngawi compiles source code to native binaries through C11.

## Pipeline

`Source (.ngawi) -> Lexer -> Parser (AST) -> Sema -> C11 codegen -> GCC -> Binary`

## Modules

### Frontend

- `src/lexer/*`
  - tokenization
  - keyword and operator recognition
- `src/parser/*`
  - recursive descent parser
  - AST construction
  - parser recovery and error caps

### Semantic analysis

- `src/sema/*`
  - symbol table and scope checks
  - type checks
  - control flow checks (`break` / `continue` scope)
  - typo hints (`did you mean`)

### Backend

- `src/codegen/*`
  - AST to C11 output
  - function prototype emission
  - builtin lowering
  - array MVP lowering (`int[]` literal and index)

### Runtime

- `src/runtime/*`
  - print helpers
  - string helpers (`eq`, `len`, `concat`, `contains`, `starts_with`, `ends_with`, `to_lower`, `to_upper`, `trim`)

Current string allocation model:

- `ng_string_concat` and `ng_string_to_lower` allocate owned runtime buffers
- runtime tracks these buffers and frees them automatically at process exit (`atexit` cleanup)
- allocations still grow with string-heavy workloads during program lifetime
- future improvement target: reusable arena/ring strategy for long-running programs

### Driver

- `src/main.c`
  - CLI handling
  - import graph loading (top-level `import` with cycle detection)
  - parse + sema + codegen orchestration
  - GCC invocation

## Error model

- Parser and sema report `file:line:col`
- Source snippet and caret marker included
- Error caps prevent noise from cascades
