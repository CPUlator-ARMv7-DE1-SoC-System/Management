# 01 — Management

**Device control and coordination using switches, buttons, LEDs, and seven-segment displays.**

<br>

## Overview

This experiment explores the basic coordination of multiple I/O devices in the CPUlator ARMv7 DE1-SoC System simulator.

The goal is to use switches and push buttons as control inputs to determine which devices are active and how they behave. The experiment combines LED animations with a seven-segment display to demonstrate how multiple peripherals can be controlled within a single C program.

This serves as a starting point for exploring more elaborate device management strategies in subsequent experiments. 

<br>

## Objectives

- Read the current state of switches and push buttons through memory-mapped I/O.
- Use switches to enable or disable individual device functions.
- Control LED animations using push buttons.
- Display a counter on the seven-segment display.
- Explore how different I/O devices can be coordinated through conditional logic and shared program state.

<br>

## Control Scheme

The current implementation uses the switches and push buttons to control the behavior of the LEDs and seven-segment display.

| Input | Behavior |
|---|---|
| Switch 0 ON | Enables LED animation control through the push buttons. |
| Switch 0 OFF | Clears the LEDs. |
| Push button 0 pressed | Runs the right-to-left LED animation and increments the counter after completion. |
| Push button 1 pressed | Runs the left-to-right LED animation and increments the counter after completion. |
| Neither button, both buttons, or another button combination | Clears the LEDs and resets the counter. |
| Switch 1 ON | Displays the counter on the seven-segment display. |
| Switch 1 OFF | Clears the seven-segment display and resets the counter. |

The LED animation and counter share the same program state, while the switches determine which functionality is enabled.

*Note: The button behavior assumes the simulator's button register reports the corresponding button bit values. The implementation currently checks for exact values of `1` and `2`.*

<br>

## Implementation

### 1. Memory-Mapped I/O

The program interacts with the simulated hardware by accessing memory-mapped peripheral registers.

The switch and push-button input registers are read through `volatile uint32_t` pointers. The LED and seven-segment display registers are accessed in the same way for output.

<br>

#### Why `volatile` Is Important

Using `volatile` is important when working with memory-mapped hardware registers because their values can change independently of the program's ordinary variables. It tells the compiler that accesses to these locations are significant and should not be treated like ordinary memory accesses whose values can be assumed to remain unchanged.

The `delay()` function also uses `volatile` for its counter. Without it, the compiler could recognize that the loop only counts down to zero and remove the loop entirely as an unnecessary calculation, depending on the optimization settings. In this case, we want the countdown loop to execute because it introduces a pause between LED updates, making the moving LED patterns slower and easier to see.

However, this is only a simple software delay, not a precise timing mechanism. Its duration depends on factors such as compiler optimization settings and execution speed. 

<br>

### 2. LED Control

The LED functions provide three basic operations:

- **Clear LEDs:** Sets the LED output register to zero.
- **Right-to-left animation:** Rotates the LED pattern in one direction.
- **Left-to-right animation:** Rotates the LED pattern in the opposite direction.

A busy-wait delay is used between successive updates to make the animation visible.

The initial LED pattern is `0x00100010`. The rotation functions operate on a 32-bit value, allowing the experiment to use CPUlator's extended 32-LED view.

<br>

**Challenges and approach**

My main challenges were figuring out how to represent the initial LED pattern and how to rotate that pattern across all 32 LEDs.

The memory view provided the key insight: hexadecimal notation offered a practical way to represent the LED pattern as a single value. Instead of thinking of the 32 LEDs as 32 separate elements, I began to see them as 32 individual bits within one 32-bit integer, with each bit corresponding to an LED.

This changed how I approached the problem. By modifying the value with bitwise operations, I could change the LED pattern directly. I chose left and right shifts to demonstrate how the pattern moves across the LEDs. Other bitwise operations, such as AND, OR, and XOR, can produce different patterns and behaviors, offering further opportunities for experimentation. 

<br>

### 3. Seven-Segment Display

The program converts an integer into the encoding required to display decimal digits on the seven-segment displays.

Each decimal digit is translated into a segment bit pattern, and successive digits are packed into a 32-bit value, with one byte allocated to each display position.

The counter is incremented after an LED animation completes and converted into its display representation whenever Switch 1 is enabled.

The current conversion supports up to four decimal digits. If the number exceeds four digits, only the four least significant decimal digits are displayed.

<br>

**Challenges and approach**

Building on what I learned from controlling the LEDs, I approached the seven-segment displays by examining how their states are represented in memory. This helped me understand how individual segments could be controlled through a numerical value.

I found that a 32-bit value could represent four display positions, with each position occupying one byte. The next step was to determine the hexadecimal encoding required to illuminate the correct segments for each decimal digit and map those encodings to their respective display positions.

Instead of treating the displays as separate physical components, I could now think of them as positions within a single value in memory. This allowed me to construct the displayed number by combining the individual digit encodings into one 32-bit value.

<br>

### 4. Input Handling and Device Coordination

The main loop continuously reads the switch state and evaluates the current inputs.

Switch 0 controls whether LED animation is permitted, while Switch 1 controls whether the counter is displayed. This provides a simple example of enabling different device functions independently.

The implementation uses blocking animation functions, meaning that the main loop cannot process new input while an animation is running. This is an important limitation to consider when moving toward more responsive device management.

<br>

## Notes and Limitations

### Available Switches and `max_switch_value`

The implementation defines:

```c
#define AVAILABLE_SWITCHES ((uint16_t) 3)
```

The value represents the number of switch bits intended to be supported. Based on this value, `max_switch_value` is calculated as:

```c
uint32_t max_switch_value =
    (uint32_t)(pow(2, AVAILABLE_SWITCHES) - 1);
```

For three switches, this produces a maximum value of `7`, corresponding to the bit patterns `000` through `111`.

The current implementation checks whether the entire switch register exceeds this maximum and terminates if it does. This is different from explicitly restricting the program to the supported bits.

An alternative approach would be to use a bit mask:

```c
#define SWITCH_MASK ((1u << AVAILABLE_SWITCHES) - 1u)

current_switches = getCurrentSwitches() & SWITCH_MASK;
```

This would retain only the three least significant bits and ignore all other switch bits. Unlike the current range check, masking evaluates the intended switch inputs independently of unrelated higher bits.

<br>

### 32 LEDs in CPUlator

Although the physical DE1-SoC board provides ten red LEDs, CPUlator allows the LED display to be extended to show 32 LEDs through its simulator interface.

This experiment uses that extended view, so the 32-bit LED pattern and rotation operations are intentional within the chosen simulation configuration. The implementation is therefore tailored to CPUlator's extended LED view rather than restricted to the physical board's ten-LED layout.

When adapting the experiment to physical hardware or another simulator configuration, the LED mask and rotation logic would need to reflect the number of available LEDs.

<br>

### Blocking Delays

The `delay()` function uses a busy-wait loop to slow down the LED animations. The duration depends on the number of loop iterations and the execution environment; it does not provide precise timing.

Because the animation functions block until all iterations finish, switch and button changes are not processed during an animation. A future iteration could explore timer-based or non-blocking animation techniques.

<br>

## Learning Outcomes

This experiment provides a foundation for understanding:

- Memory-mapped peripheral access in C.
- Bitwise operations and register values.
- Converting numeric values into hardware-specific display encodings.
- Coordinating several peripherals using shared program state.
- The distinction between enabling a device function and scheduling its execution.
- The limitations of blocking control flow in interactive applications.

<br>

## Possible Extensions

- Use bit masks to handle supported switch inputs explicitly.
- Introduce configurable device combinations and mutually exclusive modes.
- Replace blocking delays with non-blocking timing.
- Separate input processing, device state, and output updates.
- Explore how the same control scheme could be extended to additional peripherals.

---

*This experiment is part of Dev-Trace, a collection documenting my ongoing learning process with C programming and the CPUlator ARMv7 DE1-SoC System.*
