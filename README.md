# NOSO-2M

A miner for Nosocryptocurrency Protocol 2.

Noso-2m be written using C/C++, compatible with C++14/17/20 standard.
It works on a wide range of hardware architectures (Intel, AMD, ARM) and
operating systems (Linux, macOS, Android (Termux), and Windows).

## Build

On Linux/macOS/Android(Termux), requires clang, or gcc.

On Windows, requires clang and Build Tools for Visual Studio.

NOTES:

- Currently noso-2m is compatiple with C++14/17/20. So, clang version 3.4
or later, or gcc version 6.1 or later

- Can replace `c++20` in the build commands below by `c++17`, `c++14`
for older versions of clang, gcc, or Windows Build Tools

Simple build command below:

### Linux, MacOS and Android (Termux)

`clang++ noso-2m.cpp md5-c.cpp -o noso-2m -std=c++20 -lpthread -O3`

Or,

`g++ noso-2m.cpp md5-c.cpp -o noso-2m -std=c++20 -lpthread -O3`

### Windows

`clang-cl noso-2m.cpp md5-c.cpp /o noso-2m /std:c++20 /EHsc /O2 /link Ws2_32.lib`

## Usage

### Linux, MacOS and Android (Termux) 

`./noso-2m -a WALLETADDRESS -i MINERID -t THREADCOUNT` 2>errors.txt

`./noso-2m --help` for the more detail

### Windows

`.\noso-2m.exe -a WALLETADDRESS -i MINERID -t THREADCOUNT` 2> errors.txt

`.\noso-2m.exe --help` for the more detail
