# NUCLEO32_G031K8_chrono

### Testing the \<chrono\> library using the Cortex-M SysTick timer and LPTIM as a clock source.

### Systick class-

The interrupt rate is 1ms but can be changed as needed. As a chrono clock its resolution is 1us and does not depend on the interrupt rate (the time is using the combined values of the overflow count and the systick counter).

### Lptim1ClockLSI class-

The 'overflow' interrupt rate is 2 seconds (LPTIM counter is 16bits, uses 32kHz clock source). As a chrono clock its resolution is ~30us and does not depend on the interrupt rate (the time is using the combined values of the overflow count and the lptim counter). This timer uses the compare irq to wake for the next soonest task, which is now being tested.



The Makefile assumes the folders obj and bin exist, so initially create these folders. Also, the gcc toolchain from Arm needs to be extracted somewhere, and the toolchain vars in the Makefile will need to be appropriate to its location. My toolchain was extracted in the same folder as the project and the Makefile will reflect that.

my folder structure -\
\
&nbsp;&nbsp;project-\
&nbsp;&nbsp;&nbsp;&nbsp;src\
&nbsp;&nbsp;&nbsp;&nbsp;include\
&nbsp;&nbsp;&nbsp;&nbsp;obj\
&nbsp;&nbsp;&nbsp;&nbsp;bin\
&nbsp;&nbsp;&nbsp;&nbsp;arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi


If using Windows- the toolchain base folder name will be different than the Linux version. For Windows, an easy way to get 'make' and shell support is to get a copy of busybox for Windows.
