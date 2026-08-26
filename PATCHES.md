# Downstream patch series

The branch applies these commits in order after upstream `v1.2.17-6`:

| Order | Commit | Purpose |
| --- | --- | --- |
| 1 | `d394e30d5` | Keep two Xcode 26 warning classes from becoming build-stopping errors. |
| 2 | `214ceec41` | Prevent `z_validateaddress` from dereferencing a null wallet. |
| 3 | `74f90fd4c` | Honor declared `-regtest` and `-testnet` base-network selection. |
| 4 | `3ede61e7e` | Make isolated regtest mining and identity flows coherent. |
| 5 | `18a14a438` | Preserve transparent/Sapling construction before identity activation. |
| 6 | `24048004a` | Authenticate, verify, resume, and safely extract bootstrap archives. |
| 7 | `4764ae8ca` | Add bounded block-index progress and avoid duplicate header hashing. |

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
zcutil/build-mac-arm.sh
```

`maintainer/verify.sh` enforces both the ordered commit subjects and this file
allowlist before a build or bundle can proceed.
