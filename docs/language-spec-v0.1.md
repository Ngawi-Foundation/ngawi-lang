# Ngawi Language Spec v0.1

This spec defines the MVP language and compile pipeline.

## 1. Core choices

- Keywords use English.
- Blocks use `{` and `}`.
- Statements end with `;`.
- Type system uses static checks.
- Local variables allow inference from initializer.
- Compiler requires `fn main() -> int`.
- Backend emits C11 and calls GCC.

## 2. Tokens

### 2.1 Keywords

`fn`, `return`, `let`, `const`, `if`, `else`, `while`, `true`, `false`, `void`, `int`, `float`, `bool`, `string`

### 2.2 Identifiers

- First char: `[A-Za-z_]`
- Next chars: `[A-Za-z0-9_]*`

### 2.3 Literals

- int: `0 | [1-9][0-9]*`
- float: `[0-9]+\.[0-9]+`
- string: `"..."` with escapes `\"`, `\\`, `\n`, `\t`
- bool: `true | false`

### 2.4 Operators

- arithmetic: `+ - * /`
- compare: `== != < <= > >=`
- logical: `&& || !`
- assign: `=`
- return type arrow: `->`

### 2.5 Delimiters

`(` `)` `{` `}` `,` `:` `;`

### 2.6 Comments

Line comment: `// ...` until end of line.

## 3. Grammar (EBNF)

```ebnf
program        := { function_decl } EOF ;

function_decl  := "fn" IDENT "(" [ param_list ] ")" "->" type block ;
param_list     := param { "," param } ;
param          := IDENT ":" type ;

type           := "int" | "float" | "bool" | "string" | "void" ;

block          := "{" { statement } "}" ;

statement      := var_decl ";"
                | const_decl ";"
                | assign_stmt ";"
                | expr_stmt ";"
                | return_stmt ";"
                | if_stmt
                | while_stmt
                | block ;

var_decl       := "let" IDENT [ ":" type ] "=" expression ;
const_decl     := "const" IDENT [ ":" type ] "=" expression ;
assign_stmt    := IDENT "=" expression ;
expr_stmt      := expression ;
return_stmt    := "return" [ expression ] ;

if_stmt        := "if" "(" expression ")" statement [ "else" statement ] ;
while_stmt     := "while" "(" expression ")" statement ;

expression     := logical_or ;
logical_or     := logical_and { "||" logical_and } ;
logical_and    := equality    { "&&" equality } ;
equality       := comparison  { ( "==" | "!=" ) comparison } ;
comparison     := term        { ( "<" | "<=" | ">" | ">=" ) term } ;
term           := factor      { ( "+" | "-" ) factor } ;
factor         := unary       { ( "*" | "/" ) unary } ;
unary          := ( "!" | "-" ) unary | primary ;

primary        := INT_LIT
                | FLOAT_LIT
                | STRING_LIT
                | "true"
                | "false"
                | IDENT
                | call
                | "(" expression ")" ;

call           := IDENT "(" [ arg_list ] ")" ;
arg_list       := expression { "," expression } ;
```

## 4. Semantic checks

### 4.1 Symbols and scope

- Each block opens a scope.
- Same scope blocks redeclare errors.
- Use before declaration fails.

### 4.2 Variables

- `let` is mutable.
- `const` is immutable.
- Annotation type must match initializer type.
- Inferred type comes from initializer.
- Variable type `void` fails.

### 4.3 Functions

- Params require explicit types.
- Return type requires explicit type.
- Call target must exist.
- Call arg count must match param count.
- Call arg types must match param types.
- Non `void` function must return on each path in basic control flow check.

### 4.4 Operators

- Arithmetic operators require same numeric type.
- Compare operators require same numeric type and return `bool`.
- Equality operators require same type and return `bool`.
- Logical operators require `bool` operands and return `bool`.

### 4.5 Entry point

Program must define:

```ngawi
fn main() -> int { ... }
```

## 5. Codegen mapping

- `int` -> `int64_t`
- `float` -> `double`
- `bool` -> `bool`
- `string` -> `const char *`
- `void` -> `void`

Builtins:

- `print(...)` lowers to runtime calls in `src/runtime/ngawi_runtime.*`.

Codegen emits C11 with function prototypes first, then function bodies. This supports forward calls.

## 6. Diagnostics

Compiler prints errors in this format:

`<file>:<line>:<col>: error: <message>`

Parser and sema also print source snippet and caret marker under the error column.
