# NOSO-2M
A miner for Nosocryptocurrency Protocol 2.

Noso-2m be written using C/C++, compatible with C++20 standard. So it is
expected to run on a wide range of architectures and operating systems, include
linux, MacOS, Android (Termux), Windows...

## Build

Require clang version 12 or later; or gcc version 10 or later. On Windows as
well require Build Tools for Visual Studio from build a native executable file.

Simple build command below:

### Linux, MacOS and Android (Termux)

`clang++ noso-2m.cpp md5-c.cpp -o noso-2m -std=c++20 -lpthread -O3`

### Windows

`clang-cl noso-2m.cpp md5-c.cpp /o noso-2m /std:c++20 /EHsc /O2 /link Ws2_32.lib`

## Usage

### Linux, MacOS and Android (Termux) 

`./noso-2m -a WALLETADDRESS -i MINERID -t THREADCOUNT` 2>errors.txt

`./noso-2m --help` for the more detail

### Windows

`.\noso-2m.exe -a WALLETADDRESS -i MINERID -t THREADCOUNT` 2> errors.txt

`.\noso-2m.exe --help` for the more detail

