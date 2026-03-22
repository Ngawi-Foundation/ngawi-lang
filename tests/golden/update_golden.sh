#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

mkdir -p tests/golden/expected

./ngawic build examples/hello.ngawi -o tests/golden/expected/hello -S >/dev/null
./ngawic build examples/factorial.ngawi -o tests/golden/expected/factorial -S >/dev/null
./ngawic build examples/if_else.ngawi -o tests/golden/expected/if_else -S >/dev/null
./ngawic build examples/while.ngawi -o tests/golden/expected/while_loop -S >/dev/null
./ngawic build examples/forward_call.ngawi -o tests/golden/expected/forward_call -S >/dev/null

echo "Golden files updated"
