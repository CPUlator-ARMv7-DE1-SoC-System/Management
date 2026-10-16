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

My main challenge was representing the initial LED pattern and rotating it across all 32 LEDs. Examining the memory view helped me understand how a single 32-bit integer could represent the LED states, with each bit corresponding to an LED. Using hexadecimal notation and bitwise shifts, I could then move the pattern in either direction. Other operations, such as AND, OR, and XOR, offer further opportunities to experiment with different patterns and behaviors.

<br>

### 3. Seven-Segment Display

The program converts an integer into the encoding required to display decimal digits on the seven-segment displays.

Each decimal digit is translated into a segment bit pattern, and successive digits are packed into a 32-bit value, with one byte allocated to each display position.

The counter is incremented after an LED animation completes and converted into its display representation whenever Switch 1 is enabled.

The current conversion supports up to four decimal digits. If the number exceeds four digits, only the four least significant decimal digits are displayed.

**Challenges and approach**

Building on what I learned from controlling the LEDs, I approached the seven-segment displays by examining how their states are represented in memory. This helped me understand how individual segments could be controlled through numerical values.

I found that a 32-bit register can represent four display positions, with each position occupying one byte. The next step was to determine the hexadecimal encoding required to illuminate the correct segments for each decimal digit and map those encodings to their respective display positions.

The DE1-SoC system provides eight seven-segment display positions, divided into two groups of four. The first group is controlled through the memory-mapped register at `0xFF200020`, while the second group is controlled through `0xFF200030`. Since each register holds 32 bits, each can store the segment encodings for four displays, with one byte allocated to each position.

To keep the program relatively simple, I deliberately chose to access only `0xFF200020` and work with the first four display positions. This allowed me to focus on understanding the digit encodings and how multiple values can be packed into a single register without introducing the additional logic needed to update both groups of displays.

Rather than treating the displays as separate physical components, I could now think of them as positions within a value in memory. This allowed me to construct the displayed number by combining the individual digit encodings into one 32-bit value.

**Integer-to-segment conversion**

The `castIntToSegmentNumber()` function converts an integer into a representation that can be written directly to the seven-segment display register. It does this by extracting each decimal digit, converting it into its corresponding segment encoding, and combining the encodings into a single 32-bit value.

The conversion works as follows:

1. **Initialize the result:** `segment_number` starts at zero. The variable `shift` tracks the position of the digit currently being processed.
2. **Extract the last digit:** The modulo operator (`number % 10`) returns the last decimal digit of the number. For example, `1234 % 10` gives `4`.
3. **Convert the digit:** `castIntDigitToSegmentDigit()` translates the decimal digit into the hexadecimal bit pattern required to illuminate the appropriate segments.
4. **Position the encoding:** The expression `segment_digit << (shift * 8)` shifts the encoding into the correct byte position. Each display position occupies eight bits, so the shift increases by eight bits for each successive digit.
5. **Combine the result:** The bitwise OR operator (`|=`) inserts the encoded digit into `segment_number`.
6. **Continue with the next digit:** Integer division (`number /= 10`) removes the last decimal digit, allowing the function to process the next digit.

The `do-while` loop ensures that at least one digit is processed, even when the input is zero. The condition `number > 0 && shift < 4` stops the conversion when all digits have been processed or when four display positions have been filled.

For example, the integer `1234` is processed from right to left: first `4`, then `3`, then `2`, and finally `1`. Their segment encodings are placed into successive bytes, producing a single 32-bit value ready to be written to the display register.

This approach connects decimal arithmetic with bitwise operations. Instead of converting the entire number at once, the function builds the hardware representation one digit at a time.

*Note: The code handles up to four digits, but it does not explicitly discard digits beyond the fourth by calculating modulo `10,000`. It simply stops processing once shift reaches four. For positive integers, the result therefore represents the four least significant decimal digits, as intended.*

<br>

### 4. Input Handling and Device Coordination

The main loop continuously reads the switch state and evaluates the current inputs. Switch 0 determines whether LED animation is permitted, while Switch 1 controls whether the counter is displayed. This demonstrates how separate device functions can be enabled independently through conditional logic.

The blocking nature of the animation functions limits how responsively the program can react to changing inputs, as discussed in the limitations section.

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
