# Clock Bindings in Device Tree

## Overview

Clock bindings describe:
```text
hardware clock sources and clock relationships
```

inside:
- Device Tree (DTS/DTB)
- Embedded Linux systems
- SoC platforms

Clock bindings are essential because:
```text
almost every hardware peripheral depends on clocks
```

Examples:
- CPU clocks
- UART clocks
- SPI clocks
- I2C clocks
- PLL clocks
- Timer clocks

Linux kernel uses clock bindings to:
- enable clocks
- disable clocks
- set clock frequencies
- manage power efficiently

---

# 1. What are Clock Bindings?

Clock bindings are:
```text
Device Tree descriptions for clock hardware
```

They define:
- clock providers
- clock consumers
- clock relationships
- frequencies

---

# 2. Why Clocks are Important

Digital hardware requires:
```text
timing signals
```

called:
```text
clocks
```

Without clocks:
- CPU cannot execute
- UART cannot transmit
- peripherals cannot operate

---

# 3. Clock Architecture Overview

```text
Crystal Oscillator
        ↓
PLL
        ↓
Clock Controller
        ↓
Peripheral Clocks
``` id="arch1"

---

# 4. Linux Clock Framework

Linux uses:
```text
Common Clock Framework (CCF)
```

for clock management.

---

# 5. Clock Framework Goals

CCF provides:
- clock abstraction
- clock hierarchy management
- dynamic frequency scaling
- power optimization

---

# 6. Clock Provider and Consumer

| Component | Role |
|-----------|------|
| Clock Provider | Generates clocks |
| Clock Consumer | Uses clocks |

---

# 7. High-Level Clock Binding Flow

```text
DTS Clock Nodes
       ↓
Kernel Parses DTB
       ↓
Clock Providers Registered
       ↓
Consumer Drivers Request Clocks
       ↓
Clock Enabled
``` id="flow1"

---

# 8. Clock Provider Node

A clock controller is described as:

```dts
clock-controller@40000000 {
    compatible = "vendor,clock";
};
``` id="prov1"

---

# 9. Important Clock Binding Properties

| Property | Purpose |
|----------|---------|
| compatible | Driver matching |
| reg | MMIO registers |
| #clock-cells | Clock specifier format |
| clocks | Clock references |
| clock-names | Human-readable names |
| clock-frequency | Fixed frequency |

---

# 10. #clock-cells Property

Defines:
```text
number of cells used in clock specifier
```

Example:

```dts
#clock-cells = <1>;
``` id="cells1"

---

# 11. Clock Specifier

Clock consumers reference clocks using:
```text
phandle + specifier
```

Format:

```text
<&clock_controller clock_id>
```

---

# 12. Clock Consumer Example

```dts
uart0: serial@101f1000 {
    clocks = <&clkctrl 3>;
};
``` id="cons1"

Meaning:
- clock provider = clkctrl
- clock ID = 3

---

# 13. clock-names Property

Provides readable names.

Example:

```dts
clock-names = "uartclk";
``` id="names1"

---

# 14. Multiple Clock Example

```dts
clocks = <&clkctrl 1>,
         <&clkctrl 2>;

clock-names = "core",
              "bus";
``` id="multi1"

---

# 15. Clock Tree Hierarchy

```text
Oscillator
    ↓
PLL
    ↓
Clock Controller
    ↓
Peripheral Clocks
``` id="tree1"

---

# 16. Oscillator Clock Example

```dts
oscillator {
    compatible = "fixed-clock";
    #clock-cells = <0>;
    clock-frequency = <24000000>;
};
``` id="osc1"

---

# 17. Fixed Clock

Fixed clocks:
```text
constant frequency clocks
```

Example:
- crystal oscillators

---

# 18. clock-frequency Property

Defines:
```text
clock rate in Hz
```

Example:

```dts
clock-frequency = <24000000>;
``` id="freq1"

Meaning:
```text
24 MHz
```

---

# 19. PLL (Phase Locked Loop)

PLL generates:
```text
higher frequencies
```

from oscillator input.

---

# 20. PLL Clock Flow

```text
24 MHz Crystal
      ↓
PLL
      ↓
1 GHz CPU Clock
``` id="pll1"

---

# 21. Peripheral Clock Example

```dts
spi0 {
    clocks = <&clkctrl 5>;
};
``` id="spi1"

---

# 22. UART Clock Example

```dts
uart0 {
    clocks = <&clkctrl 3>;
    clock-names = "uartclk";
};
``` id="uart1"

---

# 23. CPU Clock Example

```dts
cpu@0 {
    clocks = <&cpuclk 0>;
};
``` id="cpu1"

---

# 24. Clock Provider Labels

Example:

```dts
clkctrl:
```

creates:
```text
phandle reference
```

---

# 25. Clock Consumer Flow

```text
Driver Requests Clock
        ↓
Clock Framework Searches Provider
        ↓
Clock Handle Returned
``` id="flow2"

---

# 26. Linux Clock APIs

Important APIs:
- clk_get()
- clk_prepare_enable()
- clk_disable_unprepare()
- clk_set_rate()

---

# 27. Clock Request Example

```c
clk = devm_clk_get(dev, "uartclk");
``` id="api1"

---

# 28. Enable Clock Example

```c
clk_prepare_enable(clk);
``` id="api2"

---

# 29. Disable Clock Example

```c
clk_disable_unprepare(clk);
``` id="api3"

---

# 30. Set Clock Rate Example

```c
clk_set_rate(clk, 48000000);
``` id="api4"

---

# 31. Clock Matching with Device Tree

Driver request:

```c
devm_clk_get(dev, "uartclk");
```

matches:

```dts
clock-names = "uartclk";
``` id="map1"

---

# 32. Clock Controller Driver

Clock controller driver:
- registers clocks
- controls PLLs
- manages gating

---

# 33. Clock Gating

Unused clocks can be:
```text
disabled
```

to save power.

---

# 34. Clock Gating Flow

```text
Peripheral Unused
       ↓
Clock Disabled
       ↓
Power Saved
``` id="gate1"

---

# 35. Dynamic Frequency Scaling

Clock rates can change dynamically for:
- power saving
- thermal management
- performance scaling

---

# 36. CPU Frequency Scaling

Example:

```text
1 GHz → 400 MHz
```

during idle state.

---

# 37. Clock Parent Relationship

Clocks may have:
```text
parent-child hierarchy
```

---

# 38. Parent Clock Example

```text
PLL
 ↓
Bus Clock
 ↓
UART Clock
``` id="parent1"

---

# 39. Clock Multiplexers

Clock mux selects:
```text
one of multiple clock sources
```

---

# 40. Clock Divider

Divider reduces:
```text
clock frequency
```

---

# 41. Divider Example

```text
100 MHz / 2 = 50 MHz
``` id="div1"

---

# 42. Common Clock Framework Layers

```text
Clock Consumer Drivers
         ↓
Common Clock Framework
         ↓
Clock Provider Drivers
         ↓
Hardware Registers
``` id="ccf1"

---

# 43. Device Tree Clock Parsing

Kernel parses:
- clocks
- clock-names
- frequencies

during boot.

---

# 44. Clock Bindings YAML

Modern bindings stored in:

```text
Documentation/devicetree/bindings/clock/
```

---

# 45. YAML Binding Example

```yaml
properties:
  clocks:
    maxItems: 1
``` id="yaml1"

---

# 46. Debugging Clocks

---

## Kernel Logs

```bash
dmesg
``` id="dbg1"

---

## Clock DebugFS

```bash
cat /sys/kernel/debug/clk/clk_summary
``` id="dbg2"

---

## Device Tree Inspection

```bash
cat /proc/device-tree/
``` id="dbg3"

---

# 47. Common Clock Binding Problems

| Problem | Description |
|---------|-------------|
| Missing clocks property | Driver probe fails |
| Wrong clock ID | Incorrect clock source |
| Missing clock-names | Driver lookup fails |
| Incorrect frequency | Peripheral malfunction |

---

# 48. Real Embedded Linux Example

```text
UART Driver Probes
        ↓
Requests uartclk
        ↓
Clock Framework Enables UART Clock
        ↓
UART Registers Become Functional
``` id="real1"

---

# 49. Complete Clock Initialization Workflow

```text
DTS Defines Clock Providers
        ↓
DTB Generated
        ↓
Kernel Parses Clock Bindings
        ↓
Clock Drivers Registered
        ↓
Peripheral Drivers Request Clocks
        ↓
Clocks Enabled
        ↓
Hardware Operates
``` id="final1"

---

# 50. Clock Binding Example (Complete)

```dts
clkctrl: clock-controller@40000000 {
    compatible = "vendor,clock";
    reg = <0x40000000 0x1000>;
    #clock-cells = <1>;
};

uart0: serial@101f1000 {
    compatible = "vendor,uart";
    reg = <0x101f1000 0x1000>;

    clocks = <&clkctrl 3>;
    clock-names = "uartclk";
};
``` id="full1"

---

# 51. Advantages of Clock Bindings

| Advantage | Description |
|-----------|-------------|
| Hardware abstraction | Yes |
| Power management | Yes |
| Dynamic scaling | Yes |
| Reusable drivers | Yes |

---

# 52. Clock Binding Best Practices

- use meaningful clock names
- define correct parent clocks
- use proper frequencies
- follow YAML bindings

---

# 53. Clock Binding vs GPIO Binding

| Feature | Clock Binding | GPIO Binding |
|---------|----------------|--------------|
| Purpose | Timing source | Digital I/O |
| Framework | CCF | GPIO subsystem |
| Property | clocks | gpios |

---

# 54. Embedded Linux Boot Flow

```text
Bootloader Loads DTB
        ↓
Kernel Parses Clock Nodes
        ↓
Clock Providers Registered
        ↓
Peripheral Drivers Probe
        ↓
Clocks Enabled
        ↓
Devices Functional
``` id="boot1"

---

# 55. Summary

- Clock bindings describe hardware clocks in Device Tree
- Linux uses Common Clock Framework (CCF)
- Clock providers generate clocks
- Clock consumers use clocks
- Essential for peripheral operation and power management
- Critical in Embedded Linux and SoC platforms

---


````
