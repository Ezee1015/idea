# idea
A simple To-Do App written in C that fits my needs

## Compilation
Requirements: `ncurses`

- Available `make` commands
    - `make`: Compile
    - `./build/idea`: Run
    - `make clean`: Clean the project
    - `make install`: Install the binary
    - `make uninstall`: Uninstall the binary

## Help
Each interface can implement its own commands, but you can do `help` in the interface you're using to see the available commands. For example, you can run `./build/idea help` to see the available commands for the CLI.
