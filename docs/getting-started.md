# Getting Started

This guide shows how to build the compiler and run your first Ngawi program.

## Prerequisites

- GCC with C11 support
- `make`
- Linux or macOS shell

## Build compiler

```bash
make debug
```

Binary output:

- `./ngawic`

## Compile and run a program

```bash
./ngawic build examples/hello.ngawi -o hello
./hello
```

## Emit generated C11

```bash
./ngawic build examples/hello.ngawi -o hello -S
```

Output:

- `hello.c`

## Run full test suite

```bash
make test
```

## Common commands

```bash
make clean
make debug
make release
make test
make update_golden
```
