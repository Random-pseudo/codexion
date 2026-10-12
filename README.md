*This project has been created as part of the 42 curriculum by lpoisson.*

# Codexion: Quantum Concurrency & Resource Scheduling

## Description

**Codexion** is an advanced systems programming and concurrency project inspired by Dijkstra's classic Dining Philosophers problem, set within the context of quantum developers competing for exclusive hardware resources.

In a circular co-working space, $N$ developers sit around a table sharing $N$ quantum compiler hardware keys (**dongles**). Compiling quantum code requires holding **two adjacent dongles** simultaneously (the left and right dongles). Developers cycle through three distinct operational phases:
1. **Compiling**: Holds two dongles for `time_to_compile` milliseconds.
2. **Debugging**: Releases both dongles, which enter an obligatory cooldown phase, and debugs for `time_to_debug` milliseconds.
3. **Refactoring**: Continues without dongles for `time_to_refactor` milliseconds, immediately requesting dongles again upon completion.

If a developer does not start compiling within `time_to_burnout` milliseconds since the start of their previous compile (or the simulation launch), they succumb to **burnout**, terminating the simulation. The simulation also concludes successfully if all coders complete at least `number_of_compiles_required` compilation cycles.

---

## Instructions

### Compilation

The project is compiled using standard POSIX C toolchains (`cc` / `gcc` / `clang`) with strict warning flags:
```bash
make        # Compiles the executable 'codexion'
make clean  # Removes compiled object files
make fclean # Removes object files and the 'codexion' binary
make re     # Performs a clean rebuild
```

### Execution

The binary requires exactly 8 positional arguments:
```bash
./codexion <number_of_coders> <time_to_burnout> <time_to_compile> <time_to_debug> <time_to_refactor> <number_of_compiles_required> <dongle_cooldown> <scheduler>
```

#### Arguments
- `number_of_coders`: Number of developers and dongles ($> 0$).
- `time_to_burnout`: Time in milliseconds before a coder burns out ($> 0$).
- `time_to_compile`: Time in milliseconds required to compile ($\ge 0$).
- `time_to_debug`: Time in milliseconds spent debugging ($\ge 0$).
- `time_to_refactor`: Time in milliseconds spent refactoring ($\ge 0$).
- `number_of_compiles_required`: Number of compilations each coder must complete before the simulation halts ($\ge 0$).
- `dongle_cooldown`: Cooldown duration in milliseconds after a dongle is released ($\ge 0$).
- `scheduler`: Arbitration policy: `fifo` (First In, First Out) or `edf` (Earliest Deadline First).

#### Examples
- Successful simulation with FIFO scheduling:
  ```bash
  ./codexion 5 2000 200 200 200 10 0 fifo
  ```
- Successful simulation with EDF scheduling:
  ```bash
  ./codexion 5 2000 200 200 200 7 0 edf
  ```
- Single coder burnout check:
  ```bash
  ./codexion 1 800 200 200 200 10 0 fifo
  ```
- Heavy contention with 800ms cooldown:
  ```bash
  ./codexion 5 3000 200 200 200 10 800 fifo
  ```

---

## Resources & AI Assistance

### References
- **Dijkstra, E. W. (1965)**: *Hierarchical ordering of sequential processes* & *Dining Philosophers Problem*.
- **Silberschatz, Galvin, Gagne**: *Operating System Concepts* (Process Synchronization, Deadlocks, Real-Time Scheduling).
- **POSIX Threads Programming**: IEEE Std 1003.1c specification for mutexes and condition variables.
- **CLRS (Cormen, Leiserson, Rivest, Stein)**: *Introduction to Algorithms* (Binary Min-Heap Priority Queues).
- **Liu & Layland (1973)**: *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment* (EDF Scheduling Optimality).

### Use of Artificial Intelligence
Artificial Intelligence was utilized strictly in compliance with the pedagogical guidelines:
- **Architecture Planning**: Designing a pair-request dispatcher architecture to eliminate distributed hold-and-wait dependency chains while conforming to 42 Norm requirements.
- **Min-Heap Algorithm Structuring**: Implementing and refactoring a custom binary min-heap data structure within 42 Norm limits ($\le 25$ lines per function, $\le 5$ functions per file).
- **Validation & Edge Case Analysis**: Developing exhaustive concurrency stress tests, memory leak checks (Valgrind Memcheck), and race/deadlock sanitization (Valgrind Helgrind and DRD).

---

## Blocking Cases Handled

### 1. Deadlock Prevention & Elimination of Hold-and-Wait
In a traditional dining philosophers setup, deadlocks occur when threads hold one resource while blocking to wait for the second (Coffman's **Hold and Wait** condition). Under heavy contention or cooldowns, naive locking can also induce severe starvation convoys.
- **Atomic Pair Allocation**: In Codexion, coders submit requests for their atomic pair of required dongles to a centralized priority dispatcher under `sim->table_mutex`.
- Threads never hold one dongle while waiting on another. Either both dongles are available (and outside cooldown) and granted simultaneously, or the coder thread waits on its dedicated condition variable `coder->cond`.
- This eliminates Coffman's **Hold and Wait** and **Circular Wait** conditions simultaneously.

### 2. Starvation Prevention & Fair Arbitration
- A centralized **Binary Min-Heap Priority Queue** manages all active requests.
- **FIFO Policy**: Coders are prioritized strictly by request arrival timestamp (`request_time`), with request sequence numbers (`seq`) as deterministic tie-breakers.
- **EDF Policy**: Coders are prioritized by their absolute burnout deadline:
  $$\text{deadline} = \text{last\_compile\_start} + \text{time\_to\_burnout}$$
  with earlier request times and coder IDs breaking any ties deterministically.
- **Non-Conflicting Concurrency**: When a top-priority request is momentarily waiting on a specific busy or cooling dongle, the dispatcher continues scanning the priority list, allowing independent pairs (e.g. coders 1 and 3) to execute concurrently.

### 3. Cooldown Handling
- When a coder finishes compiling and releases its dongles, each dongle's `cooldown_until` timestamp is set to `now + dongle_cooldown`.
- The dispatcher strictly enforces that a dongle cannot be reacquired until `now >= cooldown_until`.
- Waiting threads compute the next cooldown expiration and use `pthread_cond_timedwait` on `coder->cond`, ensuring immediate reactivation with zero busy-waiting.

### 4. Burnout Detection Within 10 ms
- A dedicated **Monitor Thread** continuously surveys coder health with a high-resolution sleep interval (500 µs).
- Coder compile timestamps are guarded by `coder->state_mutex`.
- If `now - coder->last_compile_start >= time_to_burnout`, the monitor atomically flags `is_stopped = 1`, logs the burnout event under `print_mutex`, and broadcasts across all condition variables (`table_cond`, `coder->cond`, `dongle->cond`).
- The burnout log is guaranteed to be the final printed line and appears within $\le 1$ ms of the theoretical deadline.

### 5. Log Serialization & Output Integrity
- All stdout writes are protected by `print_mutex`.
- Condition checks (`!sim_is_stopped()`) inside the critical section ensure that no logs are printed after a burnout occurs.
- Both "has taken a dongle" messages and the subsequent "is compiling" transition are serialized atomically, preventing interleaved logs across threads.

---

## Thread Synchronization Mechanisms

### Primitives Employed
- `pthread_mutex_t`:
  - **Table Mutex** (`sim->table_mutex`): Protects the centralized priority queue, sorted indices, and atomic dongle dispatch state.
  - **Dongle Mutex** (`dongle->mutex`): Protects individual dongle usage status (`is_in_use`, `holder_id`, `cooldown_until`).
  - **Coder State Mutex** (`coder->state_mutex`): Guarantees data-race-free reads and writes for `last_compile_start` and `compile_count`.
  - **Stop Mutex** (`sim->stop_mutex`): Synchronizes simulation termination status (`is_stopped`).
  - **Print Mutex** (`sim->print_mutex`): Ensures atomic, non-interleaved terminal output.
- `pthread_cond_t`:
  - **Coder Condition Variable** (`coder->cond`): Signals individual coders when their requested dongle pair has been granted or when the simulation halts.
  - **Table Condition Variable** (`sim->table_cond`): Notifies waiting threads of global simulation status updates.
  - **Dongle Condition Variable** (`dongle->cond`): Secondary notification primitive for dongle state changes.

### Data Race Prevention
Every shared variable accessed across threads is strictly protected by a corresponding mutex:
- `last_compile_start` is read by the monitor thread and written by the dispatcher/coder thread strictly under `coder->state_mutex`.
- `is_stopped` is checked under `sim->stop_mutex`.
- All dongle fields are modified under their respective mutexes.
- Exhaustive verification with `valgrind --tool=helgrind` and `valgrind --tool=drd` confirms **0 errors from 0 contexts**.
