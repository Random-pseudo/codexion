*This project has been created as part of the 42 curriculum by Eleve.*

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
  ./codexion 4 800 200 100 100 3 20 fifo
  ```
- Successful simulation with EDF scheduling and dongle cooldown:
  ```bash
  ./codexion 4 800 200 100 100 3 20 edf
  ```
- Burnout test with a single coder:
  ```bash
  ./codexion 1 800 200 100 100 1 0 fifo
  ```

---

## Resources & AI Assistance

### References
- **Dijkstra, E. W. (1965)**: *Hierarchical ordering of sequential processes* & *Dining Philosophers Problem*.
- **Silberschatz, Galvin, Gagne**: *Operating System Concepts* (Chapters on Process Synchronization, Deadlocks, and Real-Time Scheduling).
- **POSIX Threads Programming**: IEEE Std 1003.1c specification for mutexes and condition variables.
- **CLRS (Cormen, Leiserson, Rivest, Stein)**: *Introduction to Algorithms* (Binary Min-Heap Priority Queues).
- **Liu & Layland (1973)**: *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment* (EDF Scheduling Optimality).

### Use of Artificial Intelligence
Artificial Intelligence was utilized strictly in compliance with the pedagogical guidelines of Chapter II:
- **Architecture Planning**: Assisting in formalizing the state diagram and decomposing the modules into 42 Norm-compliant files.
- **ELI10 Didactic Formulations**: Generating pedagogical analogies (the circular table, toy consoles, bathroom locks, hospital triage for EDF) to elucidate complex concurrency mechanics for peers.
- **Test Generation & Edge Cases**: Formulating edge cases including single coder scenarios ($N=1$), zero compilation requirements, and sub-millisecond cooldown bounds.
- **Review & Verification**: Systematic validation using Valgrind and ThreadSanitizer to guarantee zero memory leaks and absence of data races.

---

## Blocking Cases Handled

### 1. Deadlock Prevention & Coffman's Conditions
Deadlock occurs if and only if all four Coffman conditions hold concurrently:
- **Mutual Exclusion**: Required because dongles are non-shareable physical resources.
- **Hold and Wait**: Coders hold one dongle while waiting for their second dongle.
- **No Preemption**: Dongles cannot be forcibly stolen while in active compilation.
- **Circular Wait**: A closed dependency chain where coder $i$ waits on coder $i+1$, and coder $N$ waits on coder 1.

**Our Solution**: We eliminate the **Circular Wait** condition via Dijkstra's **Resource Hierarchy Ordering**.
For each coder $i$, let their two neighboring dongles be $D_A = i$ and $D_B = (i \bmod N) + 1$. Each coder always requests:
$$D_{\min} = \min(D_A, D_B) \quad \text{followed by} \quad D_{\max} = \max(D_A, D_B)$$
Consequently, coder $N$ requests Dongle 1 prior to requesting Dongle $N$. Because every coder acquires resources in strictly ascending index order, cyclic dependencies are mathematically impossible, creating a Directed Acyclic Graph (DAG) of wait dependencies.

### 2. Starvation Prevention & Fair Arbitration
- Each dongle maintains its own **Binary Min-Heap Priority Queue**.
- Under `fifo`, requests are served by arrival timestamp (`request_time`).
- Under `edf`, requests are prioritized by the earliest burnout deadline:
  $$\text{deadline} = \text{last\_compile\_start} + \text{time\_to\_burnout}$$
- **Deterministic Tie-Breaker**: If two coders have equal deadlines, the tie is broken deterministically by earlier request time, and if still identical, by lower `coder_id`. This guarantees reproducible, starvation-free scheduling where coders closest to burnout are granted priority.

### 3. Cooldown Handling
- When a dongle is released, its `cooldown_until` timestamp is set to `get_time_ms() + dongle_cooldown`.
- Threads waiting for a cooled-down dongle use `pthread_cond_timedwait()` targeted precisely at `cooldown_until`. This prevents CPU-intensive busy-waiting while ensuring immediate wake-up the exact millisecond the cooldown elapses.

### 4. Precise Burnout Detection (< 10 ms)
- A dedicated **Monitor Thread** continuously scans all coders with a high-resolution polling interval ($\le 0.5$ ms).
- If `current_time - coder->last_compile_start >= time_to_burnout`, the monitor atomically flags the simulation as stopped, issues the exact burnout log, and broadcasts to all condition variables, ensuring termination latency well below the 10 ms threshold.

### 5. Log Serialization
- All standard outputs are protected by a shared `print_mutex`.
- Condition checks (`!sim->is_stopped`) inside the lock ensure that no state messages are interleaved, and that absolutely no further messages are printed after a burnout event.

---

## Thread Synchronization Mechanisms

### Primitives Employed
- `pthread_mutex_t`:
  - **Dongle Mutex** (`dongle->mutex`): Guards the dongle's internal state (`is_in_use`, `holder_id`, `cooldown_until`) and its associated min-heap priority queue.
  - **Coder State Mutex** (`coder->state_mutex`): Guarantees atomic read/write access to `last_compile_start` and `compile_count` across the coder thread and the monitor thread.
  - **Stop Mutex** (`sim->stop_mutex`): Guarantees race-free modification and inspection of `sim->is_stopped`.
  - **Print Mutex** (`sim->print_mutex`): Enforces total ordering and serial execution of console logs.
- `pthread_cond_t`:
  - **Dongle Condition Variable** (`dongle->cond`): Notifies waiting threads when a dongle is released (`pthread_cond_broadcast`) or when the simulation halts.
  - Paired with `pthread_cond_timedwait` for sub-millisecond precision during dongle cooldown expiration.

### Race Condition Prevention Example
When the monitor thread reads a coder's `last_compile_start` while the coder begins compiling:
- The coder thread locks `coder->state_mutex`, updates `last_compile_start = get_time_ms()`, and unlocks.
- The monitor thread locks `coder->state_mutex`, reads `last_compile_start`, and unlocks.
Data races on 64-bit timestamps are completely eliminated.

### Thread-Safe Communication Between Coders and Monitor
Communication is decoupled:
1. Coders update their local state under `coder->state_mutex`.
2. The monitor inspects each coder's state. Upon burnout or target compilation fulfillment, the monitor sets `sim->is_stopped = 1` and calls `pthread_cond_broadcast` on every dongle.
3. Sleeping or waiting coder threads wake up immediately, detect the stop signal, cleanly release any held locks or queue positions, and terminate.

