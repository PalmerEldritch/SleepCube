#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if command -v rg >/dev/null 2>&1; then
  rg -n "@docready" "${ROOT_DIR}/main" -g "*.h"
else
  grep -Rns "@docready" "${ROOT_DIR}/main" --include="*.h"
fi
