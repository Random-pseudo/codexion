# Codexion

This project is part of the 42 curriculum by lpoisson.

Codexion is a concurrency project from the 42 curriculum. Coders share dongles to compile. Each coder needs two dongles at the same time.

## Build

```sh
make
make clean
make fclean
make re
```

## Run

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All times are in milliseconds. The program expects exactly eight arguments:

| Argument | Rule |
| --- | --- |
| `number_of_coders` | Must be greater than 0. There is one dongle per coder. |
| `time_to_burnout` | Must be greater than 0. |
| `time_to_compile` | Must be 0 or greater. |
| `time_to_debug` | Must be 0 or greater. |
| `time_to_refactor` | Must be 0 or greater. |
| `number_of_compiles_required` | Must be 0 or greater. The simulation ends when every coder reaches this number. |
| `dongle_cooldown` | Must be 0 or greater. A dongle cannot be reused during this time after release. |
| `scheduler` | Must be `fifo` or `edf`. |

Coders repeat this cycle: compile, debug, then refactor. They need two adjacent dongles to compile. If a coder does not start compiling before their burnout time runs out, the simulation stops. With `number_of_compiles_required` set to `0`, the program exits without running the simulation.

## Examples

```sh
./codexion 5 2000 200 200 200 10 0 fifo
./codexion 5 2000 200 200 200 7 0 edf
```
