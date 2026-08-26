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

"${SCRIPT_DIRECTORY}/verify.sh"
[[ "$(/usr/bin/uname -m)" == "arm64" ]] ||
  fail "the local release must be built natively on Apple Silicon"

BUILD_JOBS="${VERUS_BUILD_JOBS:-$(/usr/sbin/sysctl -n hw.logicalcpu)}"
[[ "${BUILD_JOBS}" =~ ^[1-9][0-9]*$ ]] ||
  fail "VERUS_BUILD_JOBS must be a positive integer"

REPORT_DIRECTORY="${REPOSITORY_ROOT}/local-releases/reports"
/bin/mkdir -p "${REPORT_DIRECTORY}"
BUILD_LOG="${REPORT_DIRECTORY}/build-${RELEASE_TAG}.log"
HEAD_COMMIT="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)"
BUILD_WORKSPACE="/private/tmp/veruscoin-macos-arm64-${HEAD_COMMIT:0:12}"

if [[ -d "${BUILD_WORKSPACE}" ]]; then
  [[ "$(git -C "${BUILD_WORKSPACE}" rev-parse HEAD 2>/dev/null)" == \
    "${HEAD_COMMIT}" ]] ||
    fail "existing build workspace belongs to a different commit"
  [[ -z "$(git -C "${BUILD_WORKSPACE}" status --porcelain --untracked-files=no)" ]] ||
    fail "existing build workspace has tracked changes"
else
  git -C "${REPOSITORY_ROOT}" worktree add \
    --detach \
    "${BUILD_WORKSPACE}" \
    "${HEAD_COMMIT}"
fi

printf 'Building %s with %s jobs\n' "${RELEASE_TAG}" "${BUILD_JOBS}"
printf 'Reusable no-spaces worktree: %s\n' "${BUILD_WORKSPACE}"
(
  cd "${BUILD_WORKSPACE}"
  ./zcutil/build-mac-arm.sh "-j${BUILD_JOBS}"
) 2>&1 | /usr/bin/tee "${BUILD_LOG}"

BUILT_DAEMON="${BUILD_WORKSPACE}/src/verusd"
BUILT_CLI="${BUILD_WORKSPACE}/src/verus"
[[ -x "${BUILT_DAEMON}" ]] || fail "build did not produce src/verusd"
[[ -x "${BUILT_CLI}" ]] || fail "build did not produce src/verus"
/bin/cp "${BUILT_DAEMON}" "${REPOSITORY_ROOT}/src/verusd"
/bin/cp "${BUILT_CLI}" "${REPOSITORY_ROOT}/src/verus"
/bin/chmod 755 "${REPOSITORY_ROOT}/src/verusd" "${REPOSITORY_ROOT}/src/verus"
DAEMON="${REPOSITORY_ROOT}/src/verusd"
CLI="${REPOSITORY_ROOT}/src/verus"
[[ "$(/usr/bin/lipo -archs "${DAEMON}")" == "arm64" ]] ||
  fail "verusd is not a native arm64 executable"
[[ "$(/usr/bin/lipo -archs "${CLI}")" == "arm64" ]] ||
  fail "verus is not a native arm64 executable"
"${DAEMON}" --version | /usr/bin/grep -F \
  "Verus Daemon version ${EXPECTED_DAEMON_VERSION}" >/dev/null ||
  fail "verusd version does not match the downstream release"

BUILD_MARKER="${REPORT_DIRECTORY}/build-${HEAD_COMMIT}.passed"
printf '%s\n' "${HEAD_COMMIT}" >"${BUILD_MARKER}"

printf '[PASS] native arm64 build\n'
printf '[PASS] daemon version %s\n' "${EXPECTED_DAEMON_VERSION}"
printf 'Build log: %s\n' "${BUILD_LOG}"
