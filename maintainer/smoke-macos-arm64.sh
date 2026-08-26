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

DAEMON="${REPOSITORY_ROOT}/src/verusd"
CLI="${REPOSITORY_ROOT}/src/verus"
[[ -x "${DAEMON}" ]] || fail "src/verusd is missing; run the build first"
[[ -x "${CLI}" ]] || fail "src/verus is missing; run the build first"
command -v gtimeout >/dev/null || fail "gtimeout is required from Homebrew coreutils"
command -v python3 >/dev/null || fail "python3 is required to reserve a loopback port"

PARAMETER_DIRECTORY="${VERUS_ZCASH_PARAMS_DIR:-}"
if [[ -z "${PARAMETER_DIRECTORY}" ]]; then
  SIBLING_PARAMETER_DIRECTORY="$(
    cd "${REPOSITORY_ROOT}/../Verus-Desktop/.build/cache/zcash-params" \
      2>/dev/null && pwd || true
  )"
  PARAMETER_DIRECTORY="${SIBLING_PARAMETER_DIRECTORY}"
fi
[[ -d "${PARAMETER_DIRECTORY}" ]] ||
  fail "set VERUS_ZCASH_PARAMS_DIR to the verified proving-parameter directory"
for PARAMETER_FILE in \
  sapling-spend.params \
  sapling-output.params \
  sprout-groth16.params; do
  [[ -f "${PARAMETER_DIRECTORY}/${PARAMETER_FILE}" ]] ||
    fail "missing proving parameter: ${PARAMETER_FILE}"
done

REPORT_DIRECTORY="${REPOSITORY_ROOT}/local-releases/reports"
/bin/mkdir -p "${REPORT_DIRECTORY}"

free_port() {
  python3 - <<'PY'
import socket
with socket.socket() as listener:
    listener.bind(("127.0.0.1", 0))
    print(listener.getsockname()[1])
PY
}

run_case() (
  set -e
  set -u
  set -o pipefail

  MODE="$1"
  SMOKE_ROOT="$(/usr/bin/mktemp -d /private/tmp/verus-patched-smoke.XXXXXX)"
  DAEMON_PID=""
  cleanup() {
    if [[ -n "${DAEMON_PID}" ]] && /bin/kill -0 "${DAEMON_PID}" 2>/dev/null; then
      /bin/kill -TERM "${DAEMON_PID}" 2>/dev/null || true
      wait "${DAEMON_PID}" 2>/dev/null || true
    fi
    case "${SMOKE_ROOT}" in
      /private/tmp/verus-patched-smoke.*)
        /bin/rm -rf -- "${SMOKE_ROOT}"
        ;;
      *)
        printf '[FAIL] refusing to remove unexpected smoke path\n' >&2
        ;;
    esac
  }
  trap cleanup EXIT

  DATA_DIRECTORY="${SMOKE_ROOT}/data"
  PARAMETER_LINK_PARENT="${SMOKE_ROOT}/Library/Application Support"
  /bin/mkdir -p "${DATA_DIRECTORY}" "${PARAMETER_LINK_PARENT}"
  /bin/ln -s "${PARAMETER_DIRECTORY}" \
    "${PARAMETER_LINK_PARENT}/ZcashParams"

  RPC_PORT="$(free_port)"
  RPC_USER="verus_local_release"
  RPC_PASSWORD="$(/usr/bin/uuidgen | /usr/bin/tr '[:upper:]' '[:lower:]')"
  RPC_ARGUMENTS=(
    "-datadir=${DATA_DIRECTORY}"
    "-rpcconnect=127.0.0.1"
    "-rpcport=${RPC_PORT}"
    "-rpcuser=${RPC_USER}"
    "-rpcpassword=${RPC_PASSWORD}"
  )
  DAEMON_ARGUMENTS=(
    "${RPC_ARGUMENTS[@]}"
    "-server=1"
    "-rpcbind=127.0.0.1"
    "-rpcallowip=127.0.0.1"
    "-listen=0"
    "-discover=0"
    "-dnsseed=0"
    "-listenonion=0"
    "-connect=0"
    "-disablewallet=1"
    "-showmetrics=0"
    "-printtoconsole=1"
  )
  if [[ "${MODE}" == "regtest" ]]; then
    DAEMON_ARGUMENTS+=(
      "-regtest=1"
      "-mineraddress=RTZMZHDFSTFQst8XmX2dR4DaH87cEUs3gC"
      "-minetolocalwallet=0"
    )
  fi

  LOG_FILE="${REPORT_DIRECTORY}/smoke-${MODE}-${RELEASE_TAG}.log"
  : >"${LOG_FILE}"
  env HOME="${SMOKE_ROOT}" "${DAEMON}" \
    "${DAEMON_ARGUMENTS[@]}" >"${LOG_FILE}" 2>&1 &
  DAEMON_PID=$!

  READY=0
  for _ATTEMPT in $(/usr/bin/jot 90 1); do
    if ! /bin/kill -0 "${DAEMON_PID}" 2>/dev/null; then
      wait "${DAEMON_PID}" || true
      fail "${MODE} daemon exited before RPC became ready; see ${LOG_FILE}"
    fi
    if env HOME="${SMOKE_ROOT}" gtimeout 5 \
      "${CLI}" "${RPC_ARGUMENTS[@]}" getblockchaininfo \
      >"${SMOKE_ROOT}/blockchain.json" 2>/dev/null; then
      READY=1
      break
    fi
    /bin/sleep 1
  done
  (( READY == 1 )) || fail "${MODE} daemon did not become ready"

  env HOME="${SMOKE_ROOT}" gtimeout 10 \
    "${CLI}" "${RPC_ARGUMENTS[@]}" getnetworkinfo \
    >"${SMOKE_ROOT}/network.json"

  if [[ "${MODE}" == "main" ]]; then
    env HOME="${SMOKE_ROOT}" gtimeout 10 \
      "${CLI}" "${RPC_ARGUMENTS[@]}" z_validateaddress \
      "zs1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq" \
      >"${SMOKE_ROOT}/z-validateaddress.json"
  else
    GENESIS_HASH="$(
      env HOME="${SMOKE_ROOT}" gtimeout 10 \
        "${CLI}" "${RPC_ARGUMENTS[@]}" getblockhash 0
    )"
    [[ "${GENESIS_HASH}" == "${EXPECTED_REGTEST_GENESIS}" ]] ||
      fail "regtest genesis hash does not match the reviewed chain"
    env HOME="${SMOKE_ROOT}" gtimeout 60 \
      "${CLI}" "${RPC_ARGUMENTS[@]}" generate 2 \
      >"${SMOKE_ROOT}/generated-blocks.json"
    [[ "$(
      env HOME="${SMOKE_ROOT}" gtimeout 10 \
        "${CLI}" "${RPC_ARGUMENTS[@]}" getblockcount
    )" == "2" ]] || fail "regtest did not mine exactly two blocks"
  fi

  env HOME="${SMOKE_ROOT}" gtimeout 10 \
    "${CLI}" "${RPC_ARGUMENTS[@]}" stop >/dev/null
  wait "${DAEMON_PID}"
  DAEMON_PID=""
  printf '[PASS] isolated %s smoke\n' "${MODE}"
)

run_case main
run_case regtest

HEAD_COMMIT="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)"
SMOKE_MARKER="${REPORT_DIRECTORY}/smoke-${HEAD_COMMIT}.passed"
printf '%s\n' "${HEAD_COMMIT}" >"${SMOKE_MARKER}"
printf '[PASS] wallet-disabled RPC and regtest mining lifecycle\n'
