# Tests for idea

- `tests/tests`: Is the file that describes every test
- `tests/states/initial`: Contains some initial states for the tests
- `tests/states/final`: Contains the expected final state for some tests (not every test: some test should fail and return to the initial state, and others should have the same state as the initial)
- `tests/src`: Contains the source code for the testing app
- `tests/logs`: Contains the logs when running `./build/tests -l`
