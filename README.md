# FreeRTOS over Bao for Fixed Priority Memory Centric scheduler

## Copyright Notice
- [Bao](https://github.com/bao-project/bao-hypervisor "Bao lightweight static partitioning hypervisor on GitHub"): All files inside folders `src/baremetal-runtime` are licensed according to the specified license (See `LICENCE` for more information).
- [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel "'Classic' FreeRTOS distribution (but here only Kernel)"): All files inside the folder `src/freertos` are licensed according to the specified license (See `LICENSE.md` for more information)


## Information
This repository contains the FreeRTOS kernel from the [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel) repository and [Bao's baremetal guest](https://github.com/bao-project/bao-baremetal-guest/). It has been made to be used as a submodule in a [memory centric scheduler](https://github.com/Zefinder/bao-fpmc) over Bao, but feel free to use it and to modify it.