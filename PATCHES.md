# Downstream patch series

The branch applies these commits in order after upstream `v1.2.18`:

| Order | Commit | Purpose |
| --- | --- | --- |
| 1 | `990912a34` | Prevent `z_validateaddress` from dereferencing a null wallet. |
| 2 | `0cdd888c0` | Honor declared `-regtest` and `-testnet` base-network selection. |
| 3 | `c9e321983` | Make isolated regtest mining and identity flows coherent. |
| 4 | `bf0914e6a` | Preserve transparent/Sapling construction before identity activation. |
| 5 | `2ea6d1986` | Authenticate, verify, resume, and safely extract bootstrap archives. |
| 6 | `3261d08c4` | Add bounded block-index progress and avoid duplicate header hashing. |

The expected upstream source delta is restricted to:

```text
src/cc/eval.cpp
src/chainparams.cpp
src/chainparamsbase.cpp
src/main.cpp
src/params.cpp
src/pbaas/reserves.cpp
src/primitives/solutiondata.h
src/rpc/mining.cpp
src/rpc/misc.cpp
src/rpc/pbaasrpc.cpp
src/transaction_builder.cpp
src/txdb.cpp
src/txdb.h
```

`maintainer/verify.sh` enforces both the ordered commit subjects and this file
allowlist before a build or bundle can proceed.

The former Xcode warning patch is retired pending build validation because upstream
replaced the architecture-specific build implementation. Bootstrap TLS uses the
upstream system trust-store setup, including the downstream resume requests.
Block-index callbacks retain upstream’s migration to `std::function`.
