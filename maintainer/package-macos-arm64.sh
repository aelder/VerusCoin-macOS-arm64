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

HEAD_COMMIT="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)"
REPORT_DIRECTORY="${REPOSITORY_ROOT}/local-releases/reports"
[[ -f "${REPORT_DIRECTORY}/build-${HEAD_COMMIT}.passed" ]] ||
  fail "this exact commit has not passed the local build"
[[ -f "${REPORT_DIRECTORY}/smoke-${HEAD_COMMIT}.passed" ]] ||
  fail "this exact commit has not passed the local smoke tests"

DAEMON="${REPOSITORY_ROOT}/src/verusd"
CLI="${REPOSITORY_ROOT}/src/verus"
[[ -x "${DAEMON}" ]] || fail "src/verusd is missing"
[[ -x "${CLI}" ]] || fail "src/verus is missing"

RELEASE_ROOT="${REPOSITORY_ROOT}/local-releases/${RELEASE_TAG}"
BUNDLE_NAME="Verus-CLI-MacOS-${RELEASE_TAG}-arm64"
BUNDLE_DIRECTORY="${RELEASE_ROOT}/${BUNDLE_NAME}"
ARCHIVE="${RELEASE_ROOT}/${BUNDLE_NAME}.tgz"
[[ ! -e "${RELEASE_ROOT}" ]] ||
  fail "release directory already exists; preserve or move it before rebuilding"
/bin/mkdir -p "${BUNDLE_DIRECTORY}"

/bin/cp "${DAEMON}" "${BUNDLE_DIRECTORY}/verusd"
/bin/cp "${CLI}" "${BUNDLE_DIRECTORY}/verus"
/bin/cp "${REPOSITORY_ROOT}/zcutil/fetch-params.sh" \
  "${BUNDLE_DIRECTORY}/fetch-params"
/bin/cp "${REPOSITORY_ROOT}/vcutil/fetch-bootstrap.sh" \
  "${BUNDLE_DIRECTORY}/fetch-bootstrap"
/bin/cp "${REPOSITORY_ROOT}/COPYING" "${BUNDLE_DIRECTORY}/COPYING"
/bin/cp "${REPOSITORY_ROOT}/README-PATCHED.md" \
  "${BUNDLE_DIRECTORY}/README-PATCHED.md"
/bin/cp "${REPOSITORY_ROOT}/UPSTREAM.md" "${BUNDLE_DIRECTORY}/UPSTREAM.md"
/bin/cp "${REPOSITORY_ROOT}/PATCHES.md" "${BUNDLE_DIRECTORY}/PATCHES.md"
/bin/chmod 755 \
  "${BUNDLE_DIRECTORY}/verusd" \
  "${BUNDLE_DIRECTORY}/verus" \
  "${BUNDLE_DIRECTORY}/fetch-params" \
  "${BUNDLE_DIRECTORY}/fetch-bootstrap"

PROVENANCE_FILE="${BUNDLE_DIRECTORY}/PROVENANCE.json"
python3 - \
  "${PROVENANCE_FILE}" \
  "${UPSTREAM_REPOSITORY}" \
  "${UPSTREAM_TAG}" \
  "${UPSTREAM_COMMIT}" \
  "${PATCHSET_VERSION}" \
  "${RELEASE_TAG}" \
  "${HEAD_COMMIT}" \
  "$(/usr/bin/shasum -a 256 "${DAEMON}" | /usr/bin/awk '{print $1}')" \
  "$(/usr/bin/shasum -a 256 "${CLI}" | /usr/bin/awk '{print $1}')" <<'PY'
import json
import pathlib
import sys

(
    destination,
    upstream_repository,
    upstream_tag,
    upstream_commit,
    patchset_version,
    release_tag,
    downstream_commit,
    daemon_sha256,
    cli_sha256,
) = sys.argv[1:]

document = {
    "schemaVersion": 1,
    "distribution": "unofficial-local-unsigned-build",
    "upstream": {
        "repository": upstream_repository,
        "tag": upstream_tag,
        "commit": upstream_commit,
    },
    "downstream": {
        "releaseTag": release_tag,
        "patchsetVersion": int(patchset_version),
        "commit": downstream_commit,
    },
    "platform": "darwin",
    "architecture": "arm64",
    "assets": {
        "verusd": {"sha256": daemon_sha256},
        "verus": {"sha256": cli_sha256},
    },
}
pathlib.Path(destination).write_text(json.dumps(document, indent=2) + "\n")
PY

(
  cd "${RELEASE_ROOT}"
  COPYFILE_DISABLE=1 /usr/bin/tar -czf "${ARCHIVE}" "${BUNDLE_NAME}"
)
/usr/bin/shasum -a 256 "${ARCHIVE}" >"${ARCHIVE}.sha256"

printf '[PASS] local unsigned release bundle\n'
printf 'Archive:  %s\n' "${ARCHIVE}"
printf 'Checksum: %s\n' "${ARCHIVE}.sha256"
