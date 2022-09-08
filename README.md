# bl60x-wifimon
Very simple 2.4 GHz WiFi monitor for the BL602 and related boards.

## Introduction
The focus was on the minimal amount of implementation needed to receive frames.
No WiFi blobs or similar used and compiles to ~15 KiB.

Received packets are hexdump'ed to the UART (2 MBaud) and the green LED is toggled.
Expect to see lots and lots of beacons; see mac.c line 45 for a way to filter them out hardware-side.

## Building
Make sure your PATH has the binaries for a riscv64-unknown-elf-gcc toolchain, then run "make".
You can get a prebuilt toolchain from SiFive: https://github.com/sifive/freedom-tools/releases/tag/v2020.12.0
