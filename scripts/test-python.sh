#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if command -v xvfb-run >/dev/null 2>&1; then
	exec xvfb-run -a -s '-screen 0 1024x768x24' python -m pytest "$@"
fi

exec python -m pytest "$@"
