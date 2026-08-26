# Unofficial Verus Core macOS arm64 patchset

This repository is an unofficial derivative of
[`VerusCoin/VerusCoin`](https://github.com/VerusCoin/VerusCoin). It is not an
official Verus release and is not endorsed by the VerusCoin project.

The maintained branch starts from upstream `v1.2.17-6` and carries seven
reviewable compatibility, safety, recovery, and observability commits for the
native Apple Silicon desktop wallet. See [`UPSTREAM.md`](UPSTREAM.md) for the
immutable base and [`PATCHES.md`](PATCHES.md) for the ordered delta.

## Local workflow

The workflow is intentionally manual. Nothing runs on a schedule and nothing
pushes to GitHub.

```sh
make -f Makefile.local verify
make -f Makefile.local build
make -f Makefile.local smoke
make -f Makefile.local bundle
```

The complete sequence is:

```sh
make -f Makefile.local release
```

Generated artifacts and reports are written under the ignored
`local-releases/` directory. Because the upstream dependency Makefiles do not
support source paths containing spaces, builds use a reusable detached
worktree under `/private/tmp/veruscoin-macos-arm64-<commit>`. The smoke tests
use disposable data directories,
bind RPC only to `127.0.0.1`, disable peer discovery and listening, disable the
wallet, verify the patched shielded-address RPC, and mine only on isolated
regtest.

The smoke workflow needs the public Zcash proving parameters. It first looks
for the sibling native-wallet cache at
`../Verus-Desktop/.build/cache/zcash-params`. Set
`VERUS_ZCASH_PARAMS_DIR` when the parameters live elsewhere.

## Distribution warning

Preserve `COPYING` and the upstream copyright notices. Upstream documents
additional license considerations for Berkeley DB and macOS compatibility
code. A locally generated archive is unsigned and unnotarized until a separate
reviewed signing process is added.
