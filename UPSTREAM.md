# Upstream provenance

| Field | Value |
| --- | --- |
| Repository | `https://github.com/VerusCoin/VerusCoin.git` |
| Tag | `v1.2.17-6` |
| Commit | `dedb3d9af7bb0bab854f5004aea81032d5a9d699` |
| Downstream branch | `patched/v1.2.17-6` |
| Downstream release tag | `v1.2.17-6-macos-arm64.1` |
| Patchset version | `1` |

The `upstream` remote must continue to point to the official repository. The
local verifier rejects a moved tag, a non-descendant branch, reordered patch
commits, unexpected Core source changes, or a dirty worktree.

For a future upstream release, create a new branch from the exact upstream tag
and cherry-pick the seven patch commits in the order listed in `PATCHES.md`.
Do not merge a moving upstream default branch into a release branch and do not
reuse an official upstream tag for a derivative release.
