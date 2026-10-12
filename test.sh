#!/bin/bash

# test_args.sh — focused on argument parsing, error handling, memory
# correctness, and thread-safety, using the official grading-scale
# reference cases as benchmarks.
#
# Three separate Valgrind tools are used (they cannot be combined):
#   - memcheck (--leak-check=full)  : leaks, invalid reads/writes, double free
#   - helgrind                      : data races, lock-ordering issues
#   - drd                           : data races, deadlocks (different heuristics than helgrind)
#
# Usage: ./test_args.sh

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RESET='\033[0m'

MEMCHECK="valgrind --tool=memcheck --track-origins=yes --leak-check=full --error-exitcode=42 --quiet"
HELGRIND="valgrind --tool=helgrind --error-exitcode=42 --quiet"
DRD="valgrind --tool=drd --error-exitcode=42 --quiet"
EXEC="./codexion"
LOG_DIR="./log_args"
DEFAULT_TIMEOUT=8
# helgrind/drd instrument every memory access and every lock/unlock,
# which is far slower than memcheck — scale the timeout up so a slow
# tool pass isn't mistaken for a real hang.
THREAD_TOOL_MULTIPLIER=4


rm -rf "$LOG_DIR"
mkdir -p "$LOG_DIR"

clear
echo -e "${BLUE}=== RECOMPILATION ===${RESET}"
make re
if [ $? -ne 0 ]; then
	echo -e "${RED}Compilation failed.${RESET}"
	exit 1
fi

mkdir -p "$LOG_DIR"

TOTAL=0
FAILED=0

slugify() {
	echo "$1" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/_/g' | sed 's/__*/_/g' | sed 's/^_//;s/_$//'
}

# Runs one command under one valgrind tool, logs it, returns 0/1 (ok/fail).
# $1=logfile $2=timeout $3=tool_label $4=tool_cmd, remaining args (array-safe,
# via "$@") = the exact program arguments, whitespace/empty entries preserved.
run_under_tool() {
	local logfile="$1"
	local test_timeout="$2"
	local tool_label="$3"
	local tool_cmd="$4"
	shift 4
	local ret

	{
		echo "=== [$tool_label] ==="
		printf 'Command: %s' "$EXEC"
		printf ' [%s]' "$@"
		echo
		echo "---"
	} >> "$logfile"

	# $tool_cmd is intentionally left unquoted: it is a fixed set of
	# valgrind flags meant to word-split. "$@" is NOT flattened: each
	# program argument is forwarded exactly as captured, so a leading
	# space, a trailing space, or an empty string reaches ./codexion
	# unmodified instead of being re-split by the shell.
	timeout "$test_timeout" $tool_cmd "$EXEC" "$@" >> "$logfile" 2>&1
	ret=$?
	echo "--- [$tool_label] return code: $ret ---" >> "$logfile"

	if [ $ret -eq 42 ]; then
		echo -e "${RED}    [$tool_label] FAILURE (error detected, code 42)${RESET}"
		return 1
	elif [ $ret -eq 124 ]; then
		echo -e "${RED}    [$tool_label] FAILURE (timeout / hang)${RESET}"
		return 1
	elif [ $ret -eq 139 ]; then
		echo -e "${RED}    [$tool_label] FAILURE (segfault)${RESET}"
		return 1
	fi
	echo -e "${GREEN}    [$tool_label] OK (code $ret)${RESET}"
	return 0
}

# expect_reject "description" -- args...
# No threads are expected to be created on an invalid input (args are
# validated before create_team), so a single memcheck pass is enough
# here — it stays fast and still catches double free / invalid free on
# the error-handling path itself.
expect_reject() {
	local desc="$1"
	shift
	local -a prog_args=("$@")
	local ok=1

	TOTAL=$((TOTAL + 1))
	local slug=$(slugify "$desc")
	local logfile="$LOG_DIR/reject_$(printf '%02d' $TOTAL)_${slug}.log"
	echo "=== REJECT: $desc ===" > "$logfile"

	echo -e "${YELLOW}[Reject $TOTAL] $desc${RESET}"
	run_under_tool "$logfile" "$DEFAULT_TIMEOUT" "memcheck" "$MEMCHECK" "${prog_args[@]}"
	[ $? -ne 0 ] && ok=0

	# A silent exit code 0 on a supposedly-invalid input is suspicious:
	# it may mean the bad input was actually accepted instead of rejected.
	if [ $ok -eq 1 ] && grep -q "return code: 0" "$logfile"; then
		echo -e "${RED}    SUSPECT: return code 0 — check if input was actually rejected (see $logfile)${RESET}"
		ok=0
	fi

	if [ $ok -eq 1 ]; then
		echo -e "${GREEN}➜ OK${RESET}"
	else
		echo -e "${RED}➜ FAILURE (see $logfile)${RESET}"
		FAILED=$((FAILED + 1))
	fi
	echo "----------------------------------------------------"
}

# expect_run "description" timeout -- args...
# Real threaded execution: run once under EACH of the three tools.
# A failure in any one of them fails the test.
expect_run() {
	local desc="$1"
	local test_timeout="$2"
	shift 2
	local -a prog_args=("$@")
	local ok=1

	TOTAL=$((TOTAL + 1))
	local slug=$(slugify "$desc")
	local logfile="$LOG_DIR/run_$(printf '%02d' $TOTAL)_${slug}.log"
	echo "=== RUN: $desc ===" > "$logfile"

	echo -e "${CYAN}[Run $TOTAL] $desc${RESET}"

	run_under_tool "$logfile" "$test_timeout" "memcheck" "$MEMCHECK" "${prog_args[@]}"
	[ $? -ne 0 ] && ok=0

	run_under_tool "$logfile" "$((test_timeout * THREAD_TOOL_MULTIPLIER))" "helgrind" "$HELGRIND" "${prog_args[@]}"
	[ $? -ne 0 ] && ok=0

	run_under_tool "$logfile" "$((test_timeout * THREAD_TOOL_MULTIPLIER))" "drd" "$DRD" "${prog_args[@]}"
	[ $? -ne 0 ] && ok=0

	if [ $ok -eq 1 ]; then
		echo -e "${GREEN}➜ OK across all 3 tools — read $logfile to verify expected burnout/completion${RESET}"
	else
		echo -e "${RED}➜ FAILURE on at least one tool (see $logfile)${RESET}"
		FAILED=$((FAILED + 1))
	fi
	echo "----------------------------------------------------"
}

echo -e "\n${BLUE}=== 1. EASY ARGUMENTS (counting, basic format) ===${RESET}\n"

expect_reject "No arguments"
expect_reject "Single argument"                         5
expect_reject "Missing arguments (5 out of 8)"         20 5000 200 200 200
expect_reject "Too many arguments"                     20 5000 200 200 200 10 0 fifo extra
expect_reject "Arguments in wrong order (scheduler 1st)" fifo 20 5000 200 200 200 10 0

echo -e "\n${BLUE}=== 2. HARD ARGUMENTS (numeric format) ===${RESET}\n"

expect_reject "Negative number"                        -20 5000 200 200 200 10 0 fifo
expect_reject "Zero coders"                            0 5000 200 200 200 10 0 fifo
expect_reject "Floating point number"                  20 5000.5 200 200 200 10 0 fifo
expect_reject "Scientific notation"                    20 5e3 200 200 200 10 0 fifo
expect_reject "Hexadecimal"                            20 0x1388 200 200 200 10 0 fifo
expect_reject "Leading space in number"                " 20" 5000 200 200 200 10 0 fifo
expect_reject "Trailing space in number"               "20 " 5000 200 200 200 10 0 fifo
expect_reject "Explicit plus sign"                     +20 5000 200 200 200 10 0 fifo
expect_reject "Digits with separator"                  20 5,000 200 200 200 10 0 fifo
expect_reject "INT_MAX overflow"                       2147483648 5000 200 200 200 10 0 fifo
expect_reject "Absurdly long number"                   99999999999999999999 5000 200 200 200 10 0 fifo
expect_reject "Empty string for numeric argument"      "" 5000 200 200 200 10 0 fifo

echo -e "\n${BLUE}=== 3. TRICKY ARGUMENTS ===${RESET}\n"

expect_reject "Number followed by text"                20abc 5000 200 200 200 10 0 fifo
expect_reject "Text followed by number"                abc20 5000 200 200 200 10 0 fifo
expect_reject "Number with attached unit"              20ms 5000 200 200 200 10 0 fifo
expect_reject "Scheduler with different case"         20 5000 200 200 200 10 0 FIFO
expect_reject "Scheduler with space"                  20 5000 200 200 200 10 0 " fifo"
expect_reject "Scheduler close but invalid"           20 5000 200 200 200 10 0 fifoo
expect_reject "Empty scheduler"                        20 5000 200 200 200 10 0 ""
expect_reject "Scheduler = lifo (not allowed yet)"    20 5000 200 200 200 10 0 lifo
expect_reject "Negative number_of_compiles_required"   20 5000 200 200 200 -1 0 fifo
expect_reject "Negative dongle_cooldown"               20 5000 200 200 200 10 -50 fifo
expect_reject "Negative time_to_burnout"               20 -5000 200 200 200 10 0 fifo

echo -e "\n${BLUE}=== 4. VALID EDGE CASES ===${RESET}\n"

expect_run "number_of_compiles_required = 0 (immediate stop)" $DEFAULT_TIMEOUT \
	5 5000 200 200 200 0 0 fifo
expect_run "dongle_cooldown = 0"                      $DEFAULT_TIMEOUT 5 5000 200 200 200 5 0 fifo
expect_run "Single coder, mandatory single dongle"    $DEFAULT_TIMEOUT 1 5000 200 200 200 5 0 fifo
expect_run "Two coders (smallest ring with 2 distinct dongles)" $DEFAULT_TIMEOUT \
	2 5000 200 200 200 5 0 fifo

echo -e "\n${BLUE}=== 5. OFFICIAL REFERENCE CASES (Easy) ===${RESET}\n"
echo -e "${YELLOW}Read the logs to manually confirm expected behavior (burnout at right time, etc.).${RESET}\n"

expect_run "[Easy] 1 single coder -> MUST burnout around t=800" $DEFAULT_TIMEOUT \
	1 800 200 200 200 10 0 fifo
expect_run "[Easy] 5 coders, wide margin fifo -> NO burnout, stop after 10 compiles each" 15 \
	5 2000 200 200 200 10 0 fifo
expect_run "[Easy] Same in edf, 7 compiles each" 15 \
	5 2000 200 200 200 7 0 edf

echo -e "\n${BLUE}=== 6. OFFICIAL REFERENCE CASES (Less easy) ===${RESET}\n"

expect_run "[Less easy] Infeasible by design -> MUST burnout around t=500, 'burned out' must be the LAST line" $DEFAULT_TIMEOUT \
	5 500 200 200 200 10 0 fifo

echo -e "\n${BLUE}=== 7. OFFICIAL REFERENCE CASES (Medium) ===${RESET}\n"

expect_run "[Medium] Cooldown 400ms, wide margin -> NO burnout, verify no 'has taken' <400ms after release" 15 \
	5 3000 200 200 200 10 400 fifo
expect_run "[Medium] High cooldown, fifo -> compare with next line in edf" 20 \
	5 3000 200 200 200 10 800 fifo
expect_run "[Medium] Same parameters in edf -> grant order may differ from fifo" 20 \
	5 3000 200 200 200 10 800 edf

echo -e "\n${BLUE}=== SUMMARY ===${RESET}"
if [ $FAILED -eq 0 ]; then
	echo -e "${GREEN}$TOTAL/$TOTAL tests passed (memcheck + helgrind + drd on threaded scenarios).${RESET}"
	echo -e "${YELLOW}Remember to manually review logs in sections 5-7 (./log_args/run_*) to validate exact behavior — a script cannot judge on its own whether burnout happens at the right timestamp or if fifo/edf order diverges correctly.${RESET}"
else
	echo -e "${RED}$FAILED/$TOTAL tests failed. Check ./log_args/ for details.${RESET}"
	exit 1
fi
