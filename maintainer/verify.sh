#!/bin/bash

set -e
set -u
set -o pipefail

SCRIPT_DIRECTORY="$(cd "$(dirname "$0")" && pwd)"
REPOSITORY_ROOT="$(cd "${SCRIPT_DIRECTORY}/.." && pwd)"
source "${SCRIPT_DIRECTORY}/release.env"

fail() {
  printf '[FAIL] %s\n' "$1" >&2
  exit 1
}

[[ "$(git -C "${REPOSITORY_ROOT}" rev-parse --show-toplevel)" == \
  "${REPOSITORY_ROOT}" ]] || fail "script is not inside the expected repository"

[[ -z "$(git -C "${REPOSITORY_ROOT}" status --porcelain)" ]] ||
  fail "working tree must be clean"

[[ "$(git -C "${REPOSITORY_ROOT}" remote get-url upstream)" == \
  "${UPSTREAM_REPOSITORY}" ]] || fail "upstream remote is not official VerusCoin"

RESOLVED_UPSTREAM_COMMIT="$(
  git -C "${REPOSITORY_ROOT}" rev-parse "${UPSTREAM_TAG}^{commit}"
)"
[[ "${RESOLVED_UPSTREAM_COMMIT}" == "${UPSTREAM_COMMIT}" ]] ||
  fail "upstream tag does not resolve to the pinned commit"

git -C "${REPOSITORY_ROOT}" merge-base --is-ancestor \
  "${UPSTREAM_COMMIT}" HEAD || fail "HEAD is not based on the pinned upstream commit"

EXPECTED_PATCH_SUBJECTS=(
  "build(macOS): tolerate Xcode 26 warning classes"
  "fix(rpc): make z_validateaddress safe without a wallet"
  "fix: honor regtest and testnet network selection"
  "compat(regtest): support isolated mining and identity flows"
  "fix: preserve pre-identity Sapling transaction building"
  "security: verify bootstrap downloads and extraction"
  "perf: report and optimize block index loading"
)
DOWNSTREAM_SUBJECTS=()
while IFS= read -r SUBJECT; do
  DOWNSTREAM_SUBJECTS+=("${SUBJECT}")
done < <(
  git -C "${REPOSITORY_ROOT}" log \
    --reverse \
    --format=%s \
    "${UPSTREAM_COMMIT}..HEAD"
)
(( ${#DOWNSTREAM_SUBJECTS[@]} >= ${#EXPECTED_PATCH_SUBJECTS[@]} )) ||
  fail "the ordered patch series is incomplete"
for INDEX in "${!EXPECTED_PATCH_SUBJECTS[@]}"; do
  [[ "${DOWNSTREAM_SUBJECTS[${INDEX}]}" == \
    "${EXPECTED_PATCH_SUBJECTS[${INDEX}]}" ]] ||
    fail "patch commit order or subject changed at position $((INDEX + 1))"
done

EXPECTED_SOURCE_CHANGES="$(
  printf '%s\n' \
    src/cc/eval.cpp \
    src/chainparams.cpp \
    src/chainparamsbase.cpp \
    src/main.cpp \
    src/params.cpp \
    src/pbaas/reserves.cpp \
    src/primitives/solutiondata.h \
    src/rpc/mining.cpp \
    src/rpc/misc.cpp \
    src/rpc/pbaasrpc.cpp \
    src/transaction_builder.cpp \
    src/txdb.cpp \
    src/txdb.h \
    zcutil/build-mac-arm.sh
)"
ACTUAL_SOURCE_CHANGES="$(
  git -C "${REPOSITORY_ROOT}" diff \
    --name-only \
    "${UPSTREAM_COMMIT}..HEAD" \
    -- src zcutil
)"
[[ "${ACTUAL_SOURCE_CHANGES}" == "${EXPECTED_SOURCE_CHANGES}" ]] ||
  fail "Core source changes do not match the reviewed allowlist"

git -C "${REPOSITORY_ROOT}" diff --check "${UPSTREAM_COMMIT}..HEAD"

printf '[PASS] upstream %s resolves to %s\n' \
  "${UPSTREAM_TAG}" \
  "${UPSTREAM_COMMIT}"
printf '[PASS] seven patch commits are present in reviewed order\n'
printf '[PASS] Core source delta is restricted to 14 reviewed files\n'
printf '[PASS] clean downstream HEAD %s\n' \
  "$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)"
