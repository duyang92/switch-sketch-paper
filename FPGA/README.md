# Hardware Implementation
## Platform
We implement the hardware version of SwitchSketch on a NetFPGA-embedded prototype, where a NetFPGA-1G-CML development board connects with a workstation (with Ryzen7 1700 @3.0GHz CPU and 64GB RAM) through PCIe Gen2 X4 lanes.
## Description
We provide C++ codes for hardware implementation here, which can be directly synthesized into Verilog codes using Vivado HLS. These codes include online and offline versions of 64-bit, 128-bit, and 256-bit.
