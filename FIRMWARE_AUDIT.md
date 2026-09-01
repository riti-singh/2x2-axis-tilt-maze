# Firmware Audit

This audit records the source-level findings for the repository snapshot. It distinguishes the active maze path in `main.c` from support modules that are present but not integrated. This pull request is documentation-only: every correction below is a proposal, not an implemented firmware change. Compilation and target-hardware validation are still required before any proposal is adopted.

## Proposed source changes

No source change in this table is included in the documentation-only pull request.

| File / area | Original behavior | Identified issue | Proposed correction | Rationale | Validation still required |
| --- | --- | --- | --- | --- | --- |
| `adc.c`: GPIO mode | ADC setup enables the ADC and selects PA0/PA1 channels but does not explicitly configure PA0 and PA1 as analog inputs or disable their pulls | Operation depends on reset state or external initialization, making reuse and startup assumptions fragile | Enable GPIOA, set PA0/PA1 to analog mode, and clear their pull configuration before ADC setup | Make the ADC module own the pin configuration it requires | Compile against the exact STM32C0 CMSIS headers; confirm PA0/PA1 routing, ADC readings, leakage, and joystick range on hardware |
| `adc.c`: channel mask | `adc_setChannel()` uses signed `1 << chNum` | Signed shifts are less portable and can become undefined for out-of-range channels | Use `1U << chNum` and optionally validate the supported channel range | Express register masks as unsigned values and guard invalid callers | Compile with warnings enabled and exercise every supported ADC channel |
| `main.c`: output writes | LED, direction, and step signals are changed with `ODR` read-modify-write operations | A read-modify-write can lose an update if another execution context changes a different bit on the same port | Write the STM32 `BSRR` set/reset fields through a shared output helper | Individual GPIO transitions become atomic | Compile against the target header and verify every LED, direction, and step waveform with a logic analyzer |
| `main.c`: button debounce | The code waits for the first low sample and then delays 100 ms without confirming the button is still low | A transient can advance the state machine; the delay alone is not a stable-state check | Wait for a low sample, delay for the chosen debounce interval, and accept it only if the input remains low | Reject switch bounce and short glitches | Measure the actual button waveform and verify press/release behavior, desired latency, and active-low polarity |
| `main.c`: motor direction setup | Direction is written immediately before the step signal rises | The driver may require a nonzero direction setup time | Establish direction before the rising edge using a delay or, preferably, a timer-driven scheduler based on the driver specification | Avoid violating setup timing when direction changes | Obtain the driver timing specification and measure direction setup/hold plus step high/low times on both axes |
| `main.c`: repeated sensor logging | While PA5 remains low, “Marble left the start zone” prints on every game-loop iteration | Repeated polling logs can flood the blocking UART and further delay sensor sampling | Latch the first departure event or log only on a validated edge | Preserve a meaningful event log and reduce avoidable blocking | Confirm sensor polarity and whether re-entry should clear the latch; test noisy transitions on hardware |
| `main.c`: UART text | Most messages use `\n`, and one message contains a Unicode target character | Bare-metal serial terminals commonly expect CRLF; the character may not render with the selected C library/terminal encoding | Use consistent `\r\n` endings and portable ASCII diagnostic text | Improve terminal interoperability without affecting control behavior | Confirm output through the actual `printf` retarget, C library, UART connection, and terminal |
| `main.c`: internal linkage/types | File-local helpers and state have external linkage, direction/on values use `int`, and several masks use signed literals | The public symbol surface is larger than necessary and intent is less explicit | Mark file-local state/functions `static`, use `bool` for logical values, and use unsigned masks | Improve maintainability and compiler diagnostics | Compile all modules to detect header/API dependencies before changing linkage |
| `sysinit.c`: ISR-shared counter | `currentMilliseconds` is written by `SysTick_Handler()` and read by foreground code without `volatile` | Optimization may prevent foreground code from observing asynchronous ISR updates | Declare the counter `volatile` while retaining naturally atomic 32-bit accesses on Cortex-M0+ | Communicate asynchronous modification to the compiler | Inspect optimized output and run delay tests on the target; assess critical sections if future code performs compound operations |
| `sysinit.c`: wraparound | `delay_ms()` computes an absolute stop time and loops while `milliseconds() < stop` | A delay crossing the 32-bit tick wrap can end immediately or behave incorrectly | Compare unsigned elapsed time: `milliseconds() - start < duration` | Standard modulo arithmetic remains correct across one counter wrap | Unit-test values around `UINT32_MAX` and verify timing on the target |
| `sysinit.c`: `inline` linkage | `milliseconds()` is defined as non-static `inline` while `rpm.c` expects an external definition | C inline-linkage rules can produce missing or toolchain-dependent external symbols | Provide a normal external definition, or move a consistent `static inline` definition into a shared header | Make linkage deterministic | Link the complete project with the intended GNU dialect and inspect symbols |
| `rpm.c`: EXTI flag clear | The handler clears `EXTI->RPR1` using `|=` on a write-one-to-clear register | The preliminary read can include other pending flags, causing the subsequent write to clear unrelated events | Assign `SENSOR_MASK` directly to `RPR1` | Clear only the interrupt this handler serviced | Compile against the exact device header and test simultaneous EXTI sources |
| `tim.c`: zero PWM value | `tim14_pwm_set(0)` calculates `value - 1`, producing `65535` in the 16-bit compare register | A request for zero can become an effectively maximum compare value | Handle zero explicitly and define/document the function's count-to-duty mapping | Prevent unsigned underflow and ambiguous zero behavior | Verify ARR/CCR off-by-one semantics and measure 0%, minimum, and maximum duty cycles on PA7 |
| `uart.c`: pin comments | Comments label PA2 as RX and PA3 as TX, while the register mapping correctly configures PA2 TX and PA3 RX | Misleading comments can cause wiring or maintenance errors | Correct comments only; retain the existing AF1 register configuration | Align documentation with the STM32 pin assignment already implemented | Confirm the exact MCU/package alternate-function table and perform a UART loopback or console test |
| `main.c`: timeout state | `TIMEOUT` exists but no timer starts and no transition assigns it | The timeout branch is unreachable | Define the game duration and timing requirements, then implement a nonblocking elapsed-time transition | Avoid claiming an unfinished state works or inventing a product requirement | Agree on duration/pause/restart semantics; compile and test boundary timing and finish-versus-timeout races |
| `main.c` / `SevenSeg.c`: display | Driver/formatting routines exist, but `main.c` does not initialize I2C/display or call them; text output remains TODO | Countdown, `DONE`, and `FAIL` are not displayed | Integrate I2C/display initialization and implement required glyphs only after confirming the module and UI | Complete the intended user interface without overstating current capability | Confirm the HT16K33 address/wiring, display geometry, I2C timing, glyph mapping, and failure handling |
| Polling peripheral helpers | ADC, UART, and I2C loops wait indefinitely for status flags | A peripheral fault or missing device can stall the entire application | Add bounded waits and return errors through revised APIs | Allow the state machine to detect and report peripheral failures | Define timeout policy, compile all callers, and inject NACK/not-ready/fault conditions |
| Build/portability | Clock constants are fixed at 12 MHz; required headers/startup/linker/generated make inputs are absent; the makefile embeds an absolute Windows path | The snapshot cannot be independently compiled or reproduced | Restore a complete project and use project-relative build inputs with one authoritative clock configuration | Enable reproducible builds and meaningful automated validation | Reconstruct the original target configuration, build with the intended toolchain, flash, and run hardware regression tests |

## Active game-path review

### GPIO and ADC

`gpio_init()` configures PA9/PC7 for X direction/step, PB0/PA7 for Y direction/step, PB3/PB10 for LEDs, PB4 as a pulled-up input, and PA5/PA6 as unpulled sensor inputs. `adc_init()` does not explicitly configure joystick pins PA0 and PA1 as analog inputs. Output type and speed remain at reset defaults; whether those defaults suit the external driver must be verified electrically.

The source treats PA5 low as “left the start zone” and PA6 high as “reached the goal.” Because neither input has an internal pull, the driver board must provide stable logic levels. These polarities and external biasing cannot be proven without the schematic or hardware.

### Bit manipulation

Active output changes use `GPIOx_ODR` read-modify-write sequences. The proposed `GPIOx_BSRR` correction would make these transitions atomic. Configuration-register read-modify-write operations remain appropriate during single-threaded initialization.

### Blocking behavior

The following operations deliberately block:

- waiting for the start button;
- the three-second countdown;
- each direction setup and step high/low interval;
- polling ADC and UART readiness;
- the terminal `FINISHED` and `TIMEOUT` loops; and
- all transactions in the optional I2C module.

At maximum joystick displacement, an axis requests six steps. Each original step holds the signal high for 1 ms and low for 1 ms, so one axis can defer the next sensor check by about 12 ms; simultaneous X and Y requests can defer it by about 24 ms, excluding ADC and UART time. This architecture is adequate only if that response latency is acceptable.

### Motor constraints

The checked-in project description specifies no more than 1 kHz step frequency. The original software waveform holds step high for 1 ms and low for 1 ms, separating consecutive rising edges by approximately 2 ms (about 500 Hz). It changes direction immediately before the rising edge. The frequency meets the stated maximum, but a maximum alone is insufficient: minimum direction setup/hold and step high/low requirements must be checked against the actual driver-board interface.

X bursts are completed before Y bursts. The code does not produce coordinated two-axis trajectories, acceleration ramps, homing, or travel-limit enforcement.

### State-machine correctness

The implemented transitions are deterministic:

1. `INIT` waits for a low sample and then delays 100 ms; it does not re-check stability.
2. `READY` performs the countdown and always enters `RUNNING`.
3. `RUNNING` remains active until PA6 reads high.
4. `FINISHED` is terminal until reset.

`TIMEOUT` is unreachable: no timer is started, no duration is defined, and no code assigns `state = TIMEOUT`. Its LED/message branch is only a placeholder. The seven-segment TODOs in `countdown`, `FINISHED`, and `TIMEOUT` likewise do not execute display code.

## Support-module review

- `uart.c`: polling TX/RX can wait forever if the peripheral never becomes ready. Baud rate assumes a 12 MHz USART clock. PA2 TX and PA3 RX register configuration is internally consistent, but two mode-setting comments reverse those roles.
- `i2c.c`: every transfer is polling and has no software timeout or returned error status. NACK paths do not communicate failure to callers. Timing constants assume a 12 MHz I2C clock and 100 kHz operation.
- `SevenSeg.c`: formats numeric data for an HT16K33, but has no application call path. It cannot display the TODO words `DONE` or `FAIL` through any existing game code.
- `lsm303agr.c`: provides accelerometer register access only; the game does not initialize I2C or call it.
- `tim.c`: provides independent TIM16 interrupt and TIM14 PWM setup helpers, but the game does not call them. TIM14's PA7 PWM assignment conflicts with the active Y-step GPIO assignment if both are enabled.
- `rpm.c`: performs a blocking one-second measurement and assumes 20 pulses per revolution. Its PA1 input conflicts with joystick X. It is not called by the game.
- `sysinit.c`: owns the SysTick handler and assumes a 12 MHz core clock. Any framework supplying another `SysTick_Handler` would conflict.

## Maintainability recommendations not implemented

These changes need hardware requirements or a broader redesign and were intentionally left out:

- replace blocking step bursts with a timer-driven pulse scheduler;
- define a verified driver timing contract in microseconds;
- calibrate joystick center/endpoints and ADC sampling time;
- add sensor edge debouncing/filtering and explicit polarity names;
- implement a restart transition instead of terminal infinite loops;
- add timeouts and error returns to ADC/UART/I2C polling;
- remove or separately build unused/conflicting support modules;
- reconstruct a complete, relocatable build with owned headers and linker/startup files; and
- implement and test timeout/display behavior only after duration and UI requirements are defined.
