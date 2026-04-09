# Contributing

The best contribution is a fork.

This repo is a reference implementation — a worked example of one device
that speaks the BLE protocol described in [REFERENCE.md](REFERENCE.md).
It's not an actively maintained project, and we'd rather you build the
thing _you_ want than bend this one into it.

## So what should I do instead?

**Fork it and make it yours.** Swap the M5Stick for a Pi Pico W. Replace
the ASCII pets with an e-ink panel. Put it in a 3D-printed shell. Rip
out everything but `ble_bridge.cpp` and the JSON parser. The protocol is
the stable surface — `REFERENCE.md` is the contract, this firmware is
just one way to honor it.

## What we will take

- Corrections to `REFERENCE.md` if the protocol docs are wrong or
  unclear
- Fixes for bugs that make the reference _not work as a reference_ —
  i.e., it doesn't pair, doesn't render, crashes on boot

## What we won't take

- New features, new pets, new screens
- Ports to other boards (fork!)
- Refactors, style changes, dependency bumps

If you're unsure which bucket something falls in, it's likely the second one.
