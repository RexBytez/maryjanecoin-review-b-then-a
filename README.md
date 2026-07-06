# MaryJaneCoin (MARYJ)

**A privacy-layered Proof-of-Stake cryptocurrency with a native Solana bridge.**

Version 4.2.0 · Released 2026-04 · [maryjanecoin.net](https://maryjanecoin.net) · [whitepaper](https://whitepaper.maryjanecoin.net)

---

## Overview

MaryJaneCoin is a privacy-focused Proof-of-Stake cryptocurrency that implements layered unlinkability at the network, wallet, and consensus levels. The chain is built on the Bitcoin → Peercoin → NovaCoin → MotaCoin lineage and introduces stealth addresses, denomination mixing (CoinJoin), a dual-pool architecture separating staking from spending, and Dandelion++ transaction relay. Transaction amounts are transparent on-chain; confidential amounts are a future feature (MWEB, block 50,000).

MaryJaneCoin is the **first privacy blockchain to launch via pump.fun**. A 1:1 escrow bridge between a Solana SPL token and the native chain provides immediate DEX liquidity on Solana while allowing holders to bridge into the mainchain for unlinkable on-chain transactions (amounts are transparent; confidential amounts activate with MWEB at block 50,000).

The chain runs on a zero-inflation economic model: the entire 1 billion MARYJ supply is minted at genesis and held in bridge escrow. Block rewards are 0%. The network is secured by Proof-of-Stake and funded entirely by transaction fees.

---

## Chain Parameters

| Parameter | Value |
|-----------|-------|
| Consensus | Proof-of-Stake (pure PoS after block 500) |
| Hashing algorithm | X13 (PoW phase), SHA-256d stake kernel |
| Block time | 260 s (4 min 20 s) |
| Max supply | 1,000,000,000 MARYJ |
| Block 1 premine | 1,000,000,000 MARYJ (entire supply, bridge escrow) |
| PoS stake reward | 0% (transaction fees only) |
| Minimum TX fee | 0.420 MARYJ |
| Address prefix | `M` (pubkey byte 50) |
| P2P port | 14200 (testnet 25200) |
| RPC port | 14201 (testnet 25201) |
| Message magic | `0x44455250` (`DERP`) |
| Coinbase maturity | 10 blocks |
| Stake minimum age | 15 minutes |
| Stake maximum age | 25 days |
| Last PoW block | 500 |
| Data directory | `~/.MaryJaneCoin` |

---

## Downloads

Official release binaries are produced with the Bitcoin Core [Guix build system](./contrib/guix/README.md) for deterministic, reproducible artifacts across all six supported platforms:

| Platform | File |
|----------|------|
| Linux x86_64 | `maryjanecoin-4.2.0-x86_64-linux-gnu.tar.gz` |
| Linux aarch64 | `maryjanecoin-4.2.0-aarch64-linux-gnu.tar.gz` |
| Linux ARMv7 (Raspberry Pi) | `maryjanecoin-4.2.0-arm-linux-gnueabihf.tar.gz` |
| Windows x86_64 | `maryjanecoin-4.2.0-win64.zip` / `-setup.exe` |
| macOS Intel | `maryjanecoin-4.2.0-x86_64-apple-darwin.tar.gz` |
| macOS Apple Silicon | `maryjanecoin-4.2.0-arm64-apple-darwin.tar.gz` |

Releases are published on the [GitHub Releases page](../../releases). SHA-256 checksums are included in every release.

---

## Quick Start

### Running the Qt wallet

1. Download the wallet for your platform from the [Releases page](../../releases).
2. Extract and run `MaryJaneCoin-qt`.
3. The wallet syncs automatically from the network. First sync takes a few minutes.

### Running a full node (headless)

```
./MaryJaneCoind -daemon
./MaryJaneCoin-cli getinfo
```

The daemon reads configuration from `~/.MaryJaneCoin/MaryJaneCoin.conf`. See [`share/examples/MaryJaneCoin.conf`](share/examples/MaryJaneCoin.conf) for a minimal example.

### Solana SPL token

The SPL wrapper token on Solana is used for DEX liquidity and pump.fun trading. Use the `getsolcontractaddy` RPC on any full node to retrieve the canonical mint address and token metadata.

---

## Building from Source

### Dependencies

MaryJaneCoin Core depends on Boost, OpenSSL, BerkeleyDB 4.8 (for the wallet), LevelDB (bundled), libevent, libqrencode, miniupnpc, and Qt 5 for the GUI.

### Unix build

```
./autogen.sh
./configure
make -j$(nproc)
```

See [`doc/build-unix.txt`](doc/build-unix.txt) for detailed instructions on Ubuntu and Debian.

### Cross-platform builds

The repository includes a full [Guix build system](./contrib/guix/) that produces reproducible binaries for all six supported targets:

```
cd depends && make HOST=x86_64-w64-mingw32 -j$(nproc)   # Windows
cd depends && make HOST=aarch64-linux-gnu -j$(nproc)    # Raspberry Pi 4 / 64-bit ARM Linux
```

Or run the full Guix build (requires Guix installed):

```
./contrib/guix/guix-build
```

---

## Repository Layout

```
src/                       Blockchain daemon and Qt wallet source
  qt/                      GUI wallet (bridge, coin control, staking status)
  rpc*.cpp                 JSON-RPC server
  kernel.cpp               Proof-of-Stake kernel
  stealth.cpp, bip47.cpp   Stealth addresses + BIP47 reusable payment codes
  coinjoin.cpp, mw/        Denomination mixing + MWEB confidential transactions (dormant)
contrib/                   Init scripts, Debian packaging, Guix build system
depends/                   Cross-compilation dependency tree
doc/                       Build instructions, release notes, coding guidelines
share/examples/            Example configuration file
```

---

## Privacy Architecture

MaryJaneCoin activates its privacy features progressively at fixed block heights. Each layer is additive and independently deployable, allowing the network to stabilize between activations.

| Layer | Scope | Status |
|-------|-------|--------|
| Dandelion++ | Network-level TX relay obfuscation | Implemented |
| Stealth addresses | One-time receiving addresses per payment | Implemented |
| Denomination mixing (CoinJoin) | Amount-correlation defense on spending (≥3 equal-value outputs) | Implemented |
| Dual-pool architecture | Separates transparent staking from unlinkable spending (amounts visible) | Implemented |
| MWEB confidential transactions | Pedersen commitments + Bulletproofs (extension blocks) | Implemented, dormant (block 50,000) |

See the [whitepaper](https://whitepaper.maryjanecoin.net) for full cryptographic detail and activation schedule.

---

## Contributing

Bug reports and pull requests are welcome. Please open an issue before starting work on a substantive change. All contributors must agree to the MIT license.

---

## License

MIT. See [COPYING](COPYING).

Portions inherited from Bitcoin Core, Peercoin, NovaCoin, and MotaCoin are under the original authors' copyrights and the MIT/X11 license.

---

## Links

- Website: [maryjanecoin.net](https://maryjanecoin.net)
- Whitepaper: [whitepaper.maryjanecoin.net](https://whitepaper.maryjanecoin.net)
- Block explorer: [explorer.maryjanecoin.net](https://explorer.maryjanecoin.net)
- Bridge: [bridge.maryjanecoin.net](https://bridge.maryjanecoin.net)
- Paper wallet: [paper.maryjanecoin.net](https://paper.maryjanecoin.net)
- Support: support@maryjanecoin.net
