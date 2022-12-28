# NOSO-2M
[![Downloads](https://img.shields.io/github/downloads/f04ever/noso-2m/total)](https://github.com/f04ever/noso-2m/releases)
[![Latest Release](https://img.shields.io/github/v/release/f04ever/noso-2m?label=latest%20release)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support Android](https://img.shields.io/badge/support-Android-blue?logo=Android)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support Windows](https://img.shields.io/badge/support-Windows-blue?logo=Windows)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support macOS](https://img.shields.io/badge/support-macOS-blue?logo=macOS)](https://github.com/f04ever/noso-2m/releases/latest)
[![Support Linux](https://img.shields.io/badge/support-Linux-blue?logo=Linux)](https://github.com/f04ever/noso-2m/releases/latest)
![Build Status](https://github.com/f04ever/noso-2m/actions/workflows/build-release.yml/badge.svg)

A miner for Nosocryptocurrency Protocol-2.

> *** IMPORTANT NOTES ***: RELEASED BINARIES FOR ANDROID AARCH64/ARMV7A VERSIONS SHOULD USE IPv4 ADDRESS FOR POOL OPTIONS IN COMMAND ARGUMENT (`--pools`) AND/OR IN FILE CONFIG (`noso-2m.cfg`). OTHER BINARIES, AND SELF BUILDS NATIVELY ON SUPPORTING DEVICES/PLATFORMS CAN USE EITHER POOL DOMAIN NAME AND/OR IPv4 ADDRESS WITHOUT A PROBLEM. CHECK FILES `noso-2m-SAMPLE.cfg`, AND `noso-2m-SAMPLE-IPv4POOL.cfg` FOR
EXAMPLES OF USING DOMAIN NAME OR IPv4 ADDRESS FOR POOL CONFIG.

`noso-2m` is developed using C/C++, compatible with standards C++17/20. It is expected to be buildable and executable on a wide range of hardware architectures (Intel, AMD, arm, aarch64) and operating systems (Linux, macOS, Android (Termux), and Windows).

## Use `noso-2m` to mine NOSO

### Download binaries `noso-2m`

`noso-2m` currently uses Github Actions to provide automatically executable 64-bits and 32-bits versions for Linux, Android(Termux), macOS, and Windows on architectures amd64/x86\_64, aarch64/arm64, i686, and armv7a. Just download the appropriate version, uncompress the archive is ready to use:

### Run `noso-2m` on Linux, MacOS, or Android (Termux)

```console
$ ./noso-2m --address=WALLETADDRESS --threads=THREADCOUNT --pools="POOL-URL-LIST" 2>errors.txt
```

### Run `noso-2m` on Windows

```console
> noso-2m.exe --address=WALLETADDRESS --threads=THREADCOUNT --pools="POOL-URL-LIST" 2> errors.txt
```

### Options

- By default, `noso-2m` uses options it loads from the config file named `noso-2m.cfg` locate in the same place of `noso-2m`. The config file can be located somewhere else and be specified using option `--config="PATH-TO-CONFIG-FILE"`.

- Be is an example config file. Check files `noso-2m-SAMPLE.cfg`, or `noso-2m-SAMPLE-IPv4POOL.cfg` for more examples:

    ```
    address WALLETADDRESS
    threads THREADCOUNT
    shares MAX-SHARES
    pools POOL-URL-LIST
    binding none
    logging info
    ```

- Syntax of `POOL-URL-LIST` as follows:

    - `POOL-URL-LIST` is a list of `POOL-URL`s, separated by a semicolon (`;`), ex.: `POOL-URL-1;POOL-URL-2;POOL-URL-3`

    - `POOL-URL` syntax: `POOL-NAME:POOL-ADDRESS:POOL-PORT`, the colon (`:`) is used to separate parts.

    - `POOL-NAME` is an arbitrary name, ex.: devnoso, my-pool, pool-1, pool-2, ...

    - `POOL-ADDRESS` is either a valid IPv4 address or a domain name of the pool.

    - `POOL-PORT` is a valid port number for the pool, if omitted `POOL-PORT` will default to port `8082`.

- Options loading from the config file will be overrided by options provided by the command arguments.

- Other options:

    - `--shares` for specifying the shares limit, default 5 shares per pool.

    - `--binding` for binding a specified IPv4 address of your device, default `none`, means no binding.

    - `--logging` for displaying logging information in info or debug levels, default info level.

- Use `--help` for the more details.

## Build from source

### Downloading the source

```console
$ git clone https://github.com/f04ever/noso-2m.git
```

### Dependencies

-   On Linux/macOS/Android(Termux), it requires clang, libc++, libc++abi, libncurses,... The following command installs dependencies on Ubuntu:

```console
$ sudo apt install clang lld libc++-dev libc++abi-dev libncurses-dev
```

-   On Windows, it requires clang and Build Tools for Visual Studio installed.

### Building on Linux, MacOS

```console
$ clang++ \
    noso-2m.cpp inet.cpp comm.cpp util.cpp tool.cpp misc.cpp mining.cpp hashing.cpp md5-c.cpp \
    -o noso-2m \
    -std=c++20 \
    --stdlib=libc++ \
	-march=native \
	-Wall \
	-Wextra \
    -DNDEBUG \
    -DNO_TEXTUI \
	-Ofast \
    -flto \
    -finline-functions -funroll-loops -fvectorize \
    -lpthread -lc++ -lc++abi \
    -lncurses -lform -ltermcap \
    -s
```

### Building on Android (Termux)

```console
$ clang++ \
	noso-2m.cpp inet.cpp comm.cpp util.cpp tool.cpp misc.cpp mining.cpp hashing.cpp md5-c.cpp \
	-o noso-2m \
	-march=native \
	-std=c++20 \
	--stdlib=libc++ \
	-Wall \
	-Wextra \
	-DNDEBUG \
	-DNO_TEXTUI \
	-Ofast \
	-flto \
	-finline-functions -funroll-loops -fvectorize \
	-lpthread \
	-lncurses -lform \
	-s
```

### Building on Windows

```console
> clang++ --target=x86_64-pc-win32 \
    -Imingw-w64-clang-x86_64-ncurses-6_3\\include \
    -Imingw-w64-clang-x86_64-ncurses-6_3\\include\\ncurses \
    mingw-w64-clang-x86_64-ncurses-6_3\\lib\\libncurses.dll.a \
    mingw-w64-clang-x86_64-ncurses-6_3\\lib\\libform.dll.a \
    noso-2m.cpp inet.cpp comm.cpp util.cpp tool.cpp misc.cpp mining.cpp hashing.cpp md5-c.cpp \
    -o noso-2m.exe \
    -Wl,-machine:x64 \
    -std=c++20 \
	--stdlib=libc++ \
	-Wall \
	-Wextra \
    -DNOGDI \
    -DNDEBUG \
    -DNO_TEXTUI \
    -Ofast \
	-flto \
	-finline-functions -funroll-loops -fvectorize \
    -nostdlib \
    -lWs2_32.lib -liphlpapi.lib -lmsvcrt
```

## Donations

Nosocoin: `devteam_donations`

** The donations will go to `devteam_donations` - the wallet address of the [nosocoin's development team](https://www.nosocoin.com/) as they deserve it (***it is not my personal address***).
