# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-09-17

### Added
- **Dynamic Value & Working Memory System (`src/fact.h`, `src/fact.c`)**:
  - Tagged union `Value` supporting `VAL_INT`, `VAL_FLOAT`, `VAL_STRING`, `VAL_BOOL`, `VAL_SYMBOL`, and `VAL_NONE`.
  - Seamless cross-type numeric comparisons (comparing integer and floating-point values).
  - Explicit lifecycle functions: `value_clone`, `value_free`, `value_equal`, `value_compare`, and `value_print`.
  - Managed `WorkingMemory` fact store with monotonic clock ticks, assertion, in-place updates, retraction, O(1) attribute lookup, and debug memory dump.
- **Rule Base & Condition Evaluation (`src/rule.h`, `src/rule.c`)**:
  - Full relational operator suite: `==` (`OP_EQ`), `!=` (`OP_NEQ`), `<` (`OP_LT`), `<=` (`OP_LTE`), `>` (`OP_GT`), `>=` (`OP_GTE`), and existence checks (`OP_EXISTS`).
  - Action types: `ACT_ASSERT` (assert facts), `ACT_RETRACT` (retract facts), and `ACT_CUSTOM` (pluggable side-effects).
  - Rule definitions with configurable integer salience (priority).
  - Definition-order preserving `RuleBase` container with named rule lookup.
- **Forward-Chaining Inference Engine (`src/engine.h`, `src/engine.c`)**:
  - Core match-resolve-act cycle with quiescence detection.
  - Four conflict resolution strategies: `STRATEGY_ORDER`, `STRATEGY_SPECIFICITY`, `STRATEGY_RECENCY`, and `STRATEGY_SALIENCE`.
  - Refractory mechanism tracking activation timestamps per rule to prevent infinite execution loops without state changes.
  - Configurable safety limit (`max_cycles`) preventing runaway rules.
- **Pluggable I/O Boundary (`src/io.h`, `src/io.c`)**:
  - Abstract `FactSourceFn` and `ActionSinkFn` function pointers for decoupled hardware and mock testing.
  - Standard `io_console_action_sink` and mock in-memory action harness (`IOMock`).
- **CLI & Worked Examples (`src/main.c`, `examples/thermostat.rules`)**:
  - Command-line interface with `--help`, `--version`, `--strategy`, and `--demo` flags.
  - Thermostat worked example demonstrating multi-cycle forward chaining and dry heat alert triggering.
- **Cross-Platform Build & Zero-Dependency Test Suite**:
  - Root `CMakeLists.txt` supporting Windows MSVC, GCC, and Clang with CTest.
  - POSIX `Makefile` supporting `DEBUG=1` and `SANITIZE=1` flags.
  - Dependency-free unit test runner (`tests/test_runner.h`) with 35 comprehensive test cases across `test_fact.c`, `test_rule.c`, and `test_engine.c`.
  - Clean `.gitignore` ignoring temporary compilation, test, and IDE artifacts.
