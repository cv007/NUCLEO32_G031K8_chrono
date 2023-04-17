# NUCLEO32_G031K8_chrono

### Testing the \<chrono\> library using the Cortex-M SysTick timer as a clock source.

### Systick class-

The interrupt rate is 1ms for callback duties (tasks) but can be changed as needed. As a chrono clock its resolution is 1us and does not depend on the interrupt rate.


The Makefile assumes the folders obj and bin exist, so initially creat these folders. Also, the gcc toolchain from Arm needs to be extracted somewhere, and the toolchain vars in the Makefile will need to be appropriate to its location. My toolchain was extracted in the same folder as the project and the Makefile will reflect that.

my folder structure -\
src\
include\
obj\
bin\
arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi
