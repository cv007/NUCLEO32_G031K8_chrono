# NUCLEO32_G031K8_chrono

### Testing the \<chrono\> library using the Cortex-M SysTick timer as a clock source.

### Systick class-

The interrupt rate is 1ms for callback duties (tasks) but can be changed as needed. As a chrono clock its resolution is 1us and does not depend on the interrupt rate.
