#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

run_case() {
  local src="$1"
  local bin="$2"
  local expected="$3"

  ./ngawic build "$src" -o "$bin" >/dev/null
  local got
  got="$(./$bin)"
  if [[ "$got" != "$expected" ]]; then
    echo "E2E FAIL: $src"
    echo "  expected: [$expected]"
    echo "  got:      [$got]"
    return 1
  fi
  rm -f "$bin" "$bin.c"
}

run_fail_case() {
  local src="$1"
  local bin="$2"

  if [[ "${NGAWI_TEST_SHOW_ERRORS:-0}" == "1" || "${NGAWI_TEST_SHOW_ERRORS:-}" == "true" ]]; then
    if ./ngawic build "$src" -o "$bin" >/dev/null; then
      echo "E2E FAIL: $src"
      echo "  expected build failure but build succeeded"
      rm -f "$bin" "$bin.c"
      return 1
    fi
  else
    if ./ngawic build "$src" -o "$bin" >/dev/null 2>/dev/null; then
      echo "E2E FAIL: $src"
      echo "  expected build failure but build succeeded"
      rm -f "$bin" "$bin.c"
      return 1
    fi
  fi

  rm -f "$bin" "$bin.c"
}

run_runtime_fail_case() {
  local src="$1"
  local bin="$2"

  ./ngawic build "$src" -o "$bin" >/dev/null
  if [[ "${NGAWI_TEST_SHOW_ERRORS:-0}" == "1" || "${NGAWI_TEST_SHOW_ERRORS:-}" == "true" ]]; then
    if ./$bin >/dev/null; then
      echo "E2E FAIL: $src"
      echo "  expected runtime failure but program succeeded"
      rm -f "$bin" "$bin.c"
      return 1
    fi
  else
    if ./$bin >/dev/null 2>/dev/null; then
      echo "E2E FAIL: $src"
      echo "  expected runtime failure but program succeeded"
      rm -f "$bin" "$bin.c"
      return 1
    fi
  fi
  rm -f "$bin" "$bin.c"
}

run_case examples/hello.ngawi e2e_hello "Hello, Ngawi"
run_case examples/factorial.ngawi e2e_factorial "fact 5 = 120"
run_case examples/if_else.ngawi e2e_if_else "big"
run_case examples/while.ngawi e2e_while $'1\n2\n3'
run_case examples/forward_call.ngawi e2e_forward "42"
run_case examples/for_loop.ngawi e2e_for "6"
run_case examples/break_continue.ngawi e2e_break_continue "8"
run_case examples/modulo.ngawi e2e_modulo "1"
run_case examples/cast.ngawi e2e_cast "7 3"
run_case examples/string_eq.ngawi e2e_string_eq "true true"
run_case examples/compound_assign.ngawi e2e_compound "0"
run_case examples/incdec.ngawi e2e_incdec "4"
run_case examples/len.ngawi e2e_len "5"
run_case examples/string_concat.ngawi e2e_string_concat "ngawi"
run_case examples/elif.ngawi e2e_elif "two"
run_case examples/match.ngawi e2e_match "two"
run_case examples/match_bool.ngawi e2e_match_bool "yes"
run_case examples/match_string.ngawi e2e_match_string "yes"
run_case examples/string_builtins.ngawi e2e_string_builtins "true true true ngawilang NGAWILANG"
run_case examples/string_alloc_stress.ngawi e2e_string_alloc_stress "40 true true"
run_case examples/array_int.ngawi e2e_array_int "99 1.5 false cc 3"
run_case examples/array_nested.ngawi e2e_array_nested "10 5 3.5 true k 3"
run_case examples/array_empty.ngawi e2e_array_empty "0"
run_case examples/import_main.ngawi e2e_import "7"
run_case examples/import_nested_main.ngawi e2e_import_nested "7"
run_case examples/import_duplicate_main.ngawi e2e_import_dup "18"
run_runtime_fail_case examples/array_oob.ngawi e2e_array_oob
run_fail_case examples/import_cycle_main.ngawi e2e_import_cycle
run_fail_case examples/import_missing_main.ngawi e2e_import_missing
run_fail_case examples/import_bad_syntax_main.ngawi e2e_import_bad_syntax

echo "All e2e tests passed"
