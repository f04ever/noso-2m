Version 0.2.6

- Update seed nodes
- New hashing algo to improve mining hashrate and reduce memory usage
- Fixed terminfo path that causes xterm-256color error on termux
- Minor refining

Version 0.2.5

- New function for fastly converting integer to string during hashing, so improve hashrate
- Split the origin big source code file noso-2m.cpp into multiple source files as this is important for further developments.
- Bugfixes


Version 0.2.4

- Improve hashrate
- Logging
- TextUI
- Bugfixes

Version 0.2.3

- Support config file
- `--pools` option now supports both IP address and domain names
- Correct summary report of actual hashrate, balance, payment
- Fixed unexpected exits in Windows.
- Minor refining

Version 0.2.2

- Quick patch number overflow in block summary

Version 0.2.2

- Protect new target from racing conditions
- Block summary fixing
- Update pool protocol for providing miner app's information
- Update seed nodes
- Mainnet timestamp check before mininig

Version 0.2.0

- Support pool mining mode, plus pools failover
- Hashrate improvements
- Support CI/CD build for Linux, Android(Termux), macOS and Windows on amd64/x86-64, i686, arm64/aarch64, arm(v7a) 64-bits and 32-bits versions
- Eliminate redundant information in the summary report (will be improved more next versions)
- Bugfixes and several improvements
- Pump version series to v0.2.x

Version 0.1.3

- Update mainnet seed nodes

Version 0.1.2

- Consensus only once at beginning each blocks
- Now buildable with C++14
- Bug fixed and refining algo

Version 0.1.1

- Windows support

Version 0.1.0

- First release
