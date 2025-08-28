# idea
A simple To-Do App written in C that fits my needs

## Compilation
Requirements: `ncurses`, `sed`

- Available `make` commands
    - `make`: Compile
    - `./build/idea`: Run
    - `make clean`: Clean the project
    - `make install`: Install the binary
    - `make uninstall`: Uninstall the binary
    - `make test`: Run the tests
    - `make test_log`: Run and log the output of each test inside `./tests/logs`

## Help
Each interface can implement its own commands, but you can do `help` in the interface you're using to see the available commands. For example, you can run `./build/idea help` to see the available commands for the CLI.
