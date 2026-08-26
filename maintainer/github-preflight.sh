#!/bin/bash

set -e
set -u
set -o pipefail

SCRIPT_DIRECTORY="$(cd "$(dirname "$0")" && pwd)"
REPOSITORY_ROOT="$(cd "${SCRIPT_DIRECTORY}/.." && pwd)"

fail() {
  printf '[FAIL] %s\n' "$1" >&2
  exit 1
}

command -v gh >/dev/null || fail "GitHub CLI is not installed"
gh auth status >/dev/null || fail "GitHub CLI authentication is not ready"

if git -C "${REPOSITORY_ROOT}" remote get-url origin >/dev/null 2>&1; then
  printf '[PASS] origin is configured: %s\n' \
    "$(git -C "${REPOSITORY_ROOT}" remote get-url origin)"
else
  printf '[READY] GitHub authentication works; no origin is configured yet.\n'
fi
printf 'This check does not create, push, or publish anything.\n'
