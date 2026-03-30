<<<<<<< HEAD
<div align=center>

# ECE 340: Embedded Systems

</div>

A comprehensive collection of projects on the **Xilinx Zynq-7000 FPGA (Zedboard)** for the ECE340-Embedded Systems cource at the University of Thessaly from my 3rd year

---

<div align=center>

## 1️⃣ Lab 1: FPGA Design of a Floating Point Unit (FPU)

</div>

**Duration:** 3 Weeks | **Focus:** RTL Design, Simulation & FPGA Implementation

### 🎯 Overview
Implementation of a single-precision **Floating Point Adder (FPA)** compliant with the **IEEE 754** standard. The project evolved from a single-cycle behavioral model to a fully implemented system on the Zedboard with real-time I/O handling.

### 📂 Detailed Steps
* **Step 1 & 2: Architecture & Pipelining**
    * Designed a single-cycle FPA logic (Normalization, Leading Zero Counting).
    * Optimized performance by extending the design into a **2-stage pipeline** to improve clock frequency.
* **Step 3 & 4: Hardware Integration**
    * **Display Logic:** Implemented a `SevenSegDisplay` module with time-multiplexing (320ns) to drive 7-segment displays via PMOD.
    * **Memory & Control:** Integrated a `DataMemory` unit to store multiple 32-bit input pairs (A & B). Developed logic to cycle through these test cases using physical buttons, enabling real-time verification of the FPU.
    * **Signal Conditioning:** Implemented a **Digital Debouncer** to ensure signal stabilityand an **Edge Detector** to accurately trigger data transitions, filtering out mechanical switch noise.

### 🛠️ Toolstack
| Category | Tool |
| :--- | :--- |
| **FPGA Board** | Xilinx Zedboard (Zynq-7000) |
| **Development** | Xilinx Vivado 2020.2 |
| **Language** | Verilog HDL |

---

<div align=center>

## 2️⃣ Lab 2: Processor-Based SoC Design & SW Development

</div>

**Duration:** 2 Weeks | **Focus:** ARM Cortex-A9 Integration, Interrupts & Custom IP (PS-PL)

### 🎯 Overview

Development of a complete **System-on-Chip (SoC)** by interfacing the ARM processor (PS) with the FPGA fabric (PL). The project utilized a **Cross-Development** workflow, where software was developed and compiled on a host PC (Vitis IDE) and deployed to the target ARM Cortex-A9 processor for real-time execution and profiling.

### 📂 Detailed Steps

#### Step 1: PS-PL Communication & Interrupt Management
* **1a. Basic Interface:** Built a hardware platform with an ARM CPU, UART, and GPIOs. Developed software to read physical switches and buttons from the PL and output their status to the serial console (Minicom), verifying the link between software and hardware.
* **1b. Interrupt Handling:** Replaced the polling mechanism with an **Interrupt-Driven** approach. Configured the **Generic Interrupt Controller (GIC)** to detect button presses as asynchronous events, triggering specialized Interrupt Service Routines (ISRs) to update a counter.

#### Step 2: Application Profiling & Code Optimization
* **Performance Benchmarking:** Implemented a timer utility using the **ARM Private Timer** to measure execution cycles with high precision.
* **Workload Analysis:** Profiled computationally intensive tasks, specifically **Matrix Multiplication** and the **Calculation of Pi**. 
* **Optimization:** Evaluated the impact of software optimizations (such as loop unrolling) by comparing "Before vs. After" execution times, gaining insight into CPU bottleneck management.

#### Step 3: Custom IP Integration (Lab 1 FPU)
* **IP Packaging:** Converted the Floating Point Adder from Lab 1 into a standalone **AXI4-Lite Peripheral**. 
* **Address Mapping:** Integrated the IP into the SoC's memory map, allowing the ARM CPU to write operands ($A, B$) and control signals directly to the hardware registers.
* **System Integration:** Verified the full path: ARM CPU triggers the FPU $\rightarrow$ Hardware calculates result $\rightarrow$ Output is displayed on Zedboard's **LEDs** and **7-Segment displays**.

### 🛠️ Toolstack
| Category | Tool |
| :--- | :--- |
| **Architecture** | Zynq-7000 SoC (Dual-core ARM Cortex-A9) |
| **Hardware Design** | Xilinx Vivado 2020.2 |
| **Software Dev** | Xilinx Vitis IDE |

---

<div align=center>

## Upcoming Labs

</div>

* **3️⃣ Lab 3 (Pending):** Major 7-week system-level project (Final Capstone).

---
=======
# Embedded-Systems
Embedded Systems projects on Xilinx Zynq-7000 FPGA (Zedboard) for the ECE340 course at the University of Thessaly from my 3rd year.
>>>>>>> 936902baa5df12718cf702241a663ec13e6c64f1
