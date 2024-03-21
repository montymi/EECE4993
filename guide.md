install `usbipd` using instructions at https://learn.microsoft.com/en-us/windows/wsl/connect-usb 
pre-requisites:
```
export Development=$HOME/path/to/development/directory
[sudo]
    apt-get install make libtinfo5 libncursesw5 minicom
    apt install usbutils
```
Setup Nuclei Development Environment:
```
cd $Development/Nuclei
tar -xzf nuclei-openocd-2023.10-linux-x64.tgz
tar -xjf 'nuclei_riscv_newlibc_prebuilt_linux64_2023.10 (1).tar.bz2'
git clone https://github.com/Nuclei-Software/nuclei-sdk sdk
export RISCV_GCC=$Development/Nuclei/gcc
export OPENOCD=$Development/Nuclei/openocd/2023.10
export PATH=$PATH:$RISCV_GCC/bin:$OPENOCD/bin
echo "NUCLEI_TOOL_ROOT=$Development/Nuclei/gcc/" > setup_config.sh
```
Writing changes to $Development/Nuclei/openocd/$OCDVERSION/SoC/gd32vf103/build.mk for Sipeed Longan Nano Board ->
```
BOARD ?= gd32vf103c_longan_nano
INCDIRS += $(Development)/Nuclei/testing/Include
C_SRCDIRS += $(Development)/Nuclei/testing/Source
RISCV_ARCH ?= rv32imac
RISCV_ABI ?= ilp32
```
Writing changes to $Development/Nuclei/sdk/Build/Makefile.rules for uploading to board ->
```
OPENOCD_CFG := ~/Lab/Development/Nuclei/sdk/SoC/gd32vf103/Board/gd32vf103c_longan_nano/openocd_gd32vf103.cfg
upload: $(TARGET_ELF)
    @$(ECHO) "Download and run $<"
    openocd -f $(OPENOCD_CFG) -c "program $< verify reset exit"
```
Attaching and flashing to USB from WSL:
```
[ADMIN] usbipd list                         # for listing current USB inputs
[ADMIN] usbipd bind --busid 1-8             # for binding to Windows system
[ADMIN] usbipd attach --wsl --busid 1-8     # for binding to WSL system
lsusb                                       # for checking on WSL system
cd $Development/Nuclei/sdk/application/baremetal/helloworld
make upload
[sudo] minicom -D /dev/ttyUSB1              # monitoring output 
```
Files added for testing:
$(Development)/Nuclei/testing/Include/get_clock.h
```
// get_clock.h

#ifndef GET_CLOCK_H
#define GET_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t get_clock(void);

#ifdef __cplusplus
}
#endif

#endif /* GET_CLOCK_H */

```
$(Development)/Nuclei/testing/Source/get_clock.c
```
// get_clock.c

#include "get_clock.h"
#include <time.h> // Include necessary headers for time functions

uint64_t get_clock(void) {
    // Use platform-specific function or standard library to get current time
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time); // Use CLOCK_MONOTONIC for monotonic time
    return (uint64_t)(current_time.tv_sec) * 1000000000 + current_time.tv_nsec;
}

```
