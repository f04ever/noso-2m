# NOSO-2M
![Build Status](https://github.com/f04ever/noso-2m/actions/workflows/build-release.yml/badge.svg)
[![Support Linux](https://img.shields.io/badge/support-Linux-blue?logo=Linux)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support Windows](https://img.shields.io/badge/support-Windows-blue?logo=Windows)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support macOs](https://img.shields.io/badge/support-macOS-blue?logo=macOS)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support Android](https://img.shields.io/badge/support-Android-blue?logo=Android)](https://github.com/f04ever/noso-2m/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/f04ever/noso-2m/total)](https://github.com/f04ever/noso-2m/releases)

A miner for Nosocryptocurrency Protocol-2.

`noso-2m` supports mining both ***solo*** and ***pool*** modes on mainnet. `noso-2m` supports *_failover_* to other pools in pool mining modes.

`noso-2m` be developed using C/C++, compatible with standards C++17/20. It is expected be buildable and executable on a wide range of hardware architectures (Intel, AMD, arm, aarch64) and operating systems (Linux, macOS, Android (Termux), and Windows).

From version 0.2.4, noso-2m supports a simple text UI that expects to help normal users starting with noso-2m easier. A logging file `noso-2m.log` is provided as well for advanced users. The command `pools` shows information of pools listed in config file or provided when run the `noso-2m` program. Use command `help` for more utilised commands and helps.

## Run `noso-2m` miner

`noso-2m` currently provides executable 64-bits and 32-bits versions for Linux, Android(Termux), macOS, and Windows pre-built on architectures amd64/x86\_64, aarch64/arm64, i686, and arm. Just download the appropriate version, uncompress the archive and run it from command shell as bellow:

### On Linux, MacOS, or Android (Termux)

`./noso-2m -a WALLETADDRESS -t THREADCOUNT 2>errors.txt`

### On Windows

`.\noso-2m.exe -a WALLETADDRESS -t THREADCOUNT 2> errors.txt`

** NODES:

- By default, `noso-2m` does mining using arguments loading from config file `noso-2m.cfg` at the same location of `noso-2m` program if exist.

- Config file formation like below
<blockquote>
<p>`address WALLETADDRESS`</p>
<p>`minerid MINERID****`</p>
<p>`threads THREADCOUNT`</p>
<p>`pools POOL-URL-LIST`</p>
<p>`solo false`</p>
</blockquote>

- Config file can locate at diffent location and be specified using option `--config="PATH-TO-CONFIG-FILE"`.

- Arguments loading from config file be overwrited by options provided in the running command.

- By default, `noso-2m` does mining using `pool mining mode`, and does failover between two pools `f04ever` pool and `devnoso` pool.

- Provide pool addresses to the running command by using option `--pools="POOL-URL-LIST"` (opening and closing quotation marks (`"`) are mandatory if having more than one pool URLs provided)

- If more than one pool address be provided, `noso-2m` will do failover between provided pools when the current mining pool unreacheable (pool off, network problem, ...).

- Formation of `POOL-URL-LIST` as following:

    + `POOL-URL-LIST` is a list of `POOL-URL`s, separate each other pool by a semicolon (`;`), ex.: `POOL-URL-1;POOL-URL-2;POOL-URL-3`

    + `POOL-URL` has formation: `POOL-NAME:POOL-ADDRESS:POOL-PORT`, the colon (`:`) be used to separate parts.

    + `POOL-NAME` is an arbitrary name, ex.: devnoso, my-pool, pool-1, pool-2, ...

    + `POOL-ADDRESS` is either a valid IP address or a domain name of pool.

    + `POOL-PORT` is a valid port number of pool, omitted `POOL-PORT` then the default port `8082` be used instead.

    + Pools `f04ever` and `devnoso` can also be provided to `POOL-URL-LIST` in a short form by their names and omit `POOL-IP-ADDRESS` and `POOL-PORT` as well.

    + An example: `./noso-2m -a N3G1HhkpXvmLcsWFXySdAxX3GZpkMFS -i 1000 -t 8 --pools="f04ever:209.126.80.203:8082;devnoso:45.146.252.103:8082"`

- Use option `--solo` for mainnet solo mining mode instead. Should provide `Miner ID` by option `-i` / `--minerid` if mining from more than one machine using one wallet address in solo mode.

- Use `--help` for the more command detail.

## Build from source

On Linux/macOS/Android(Termux), requires clang, or gcc.

On Windows, requires clang and Build Tools for Visual Studio.

From version 0.2.4, need library NCURSES for the text UI. NCURSES is provided already (or very easy to install) on Linux/macOS/Android platforms, on Windows requires a NCURSES on top of MinGW.

** NOTES:

- Currently `noso-2m` is compatiple with C++14/17/20. So, clang version 3.4 or later, or gcc version 6.1 or later. Recommend to build `noso-2m` with `c++20`.

- Can replace `c++20` in the build commands below by `c++17` for older versions of clang, gcc, or Windows Build Tools

Simple command for download source code and build `noso-2m` as below:

### On Linux, MacOS, or Android (Termux)

`clang++ noso-2m.cpp md5-c.cpp -o noso-2m -std=c++20 -O3 -DNDEBUG --stdlib=libc++ -fuse-ld=lld -lpthread -lc++abi -lncurses -lform -ltermcap`

Or use gcc,

`g++ noso-2m.cpp md5-c.cpp -o noso-2m -std=c++20 -O3 -DNDEBUG -lpthread -lncurses -lform -ltermcap`

### On Windows

`clang++ noso-2m.cpp md5-c.cpp -o noso-2m.exe -std=c++20 -O2 -DNDEBUG -lWs2_32.lib`

Or use clang compatible driver mode for Microsoft Build Tools

`clang-cl noso-2m.cpp md5-c.cpp /o noso-2m.exe /std:c++20 /O2 /EHsc /DNDEBUG /link Ws2_32.lib`

## Donations

Nosocoin: `devteam_donations`

** The donations will go to `devteam_donations` - the wallet address of the [nosocoin's development team](https://www.nosocoin.com/) as they deserve (***it is not my personal address***)
