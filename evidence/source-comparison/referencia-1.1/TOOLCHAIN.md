# Toolchain and source-integrity audit

Audit date: 2026-10-10.

## Official inputs

| Component | Identity |
|---|---|
| FLoRa | tag `v1.1.0`; commit `51018f80fdafc562d8f9fa022e551f835c8969ee`; tree `b565093c4491b318ec5ab6174e6f0a33f45d1be6` |
| INET | tag `v4.4.0`; commit `ceafc917d5ee293cf4828845841c73890b2133a7`; tree `66fca9ac41ceb4d3eb2ed1a106281caa7ce4cd0b` |
| OMNeT++ | 6.0 final, build `220413-71d8fab425`, Clang 10, release/Cmdenv |
| OMNeT++ core archive | SHA-256 `a8edb446e409058ade3e2256239171682bf24b0997235288f148b9b2839e7695`; upstream MD5 `eff4bbd7fcb26984bb9d0b2aa4875fe2` |

Both upstream Git worktrees were clean when this audit was written. A
path-by-path comparison of tracked inputs found zero content differences
between `upstream/` and `work/`: 93/93 FLoRa entries and 7,369/7,369
INET entries matched. The INET count includes 83 preserved symbolic links
whose targets may be absent from the release documentation tree; they are
not missing source files.

## Executed binaries

| Artifact | SHA-256 |
|---|---|
| `toolchain/omnetpp-6.0/bin/opp_run` | `45eae155dd4854c77a92e4a57431dbd2ba72b3e95e91df99b4f7532e00a39d1a` |
| `work/flora/src/libflora.so` | `9469feb6724ad4c81b128ed0bd1d61ad17f0d57ecaca3a2790c38bc33924e809` |
| `work/inet4.4/src/libINET.so` | `213414e8b2159b85b0999948ed924684f2e7a1fec8760c7b6c85e93c60075895` |

The sealed campaign manifest has SHA-256
`f47ec9b319b88fd1ab2205f08f4e32261f3fd1e62b554dd41d9e10aa39ad2684`.
It hashes the official source trees, executed binaries, experiment
configuration, compatibility NED, and runtime XML inputs.

## Compatibility boundary

No official FLoRa or INET C++ source is patched. The experiment adds only
`experiment/compat.ned`, outside the official tree, to mark stationary
`initialX` and `initialY` mutable under OMNeT++ 6.0 final. This lets the
official application's own circular placement execute and does not replace
the radio, channel, traffic, duty-cycle, or ADR implementations.

The existing OMNeT++ 6.0pre11 installation was not used for this reference.
The retained attempt and the direct header audit are described in
`attempts/inet4.4-pre11-failed/ATTEMPT.md`.
