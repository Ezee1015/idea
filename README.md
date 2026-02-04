# idea
A simple To-Do App written in C that fits my needs

> [!NOTE]
> This project is a work in progress so anything can change or break without notice

## Compilation
Requirements: `ncurses`, `sed`

- Available `make` commands
    - `make`: Compile
    - `make clean`: Clean the project
    - `make install`: Install the binary into `/usr/local/bin/`
    - `make uninstall`: Uninstall the binary from the system
    - `make test`: Run the tests. For more options, such as memory leak checking and output logging, see `./build/tests -h`

## Run
This program has 2 interfaces: CLI or TUI.
- You can see all the available CLI options with `./build/idea -h`
- or run the TUI version by only running `./build/idea` and typing `:help` (you can change pages with `h` and `l`, and increase or decrease the number of items shown with `+` or `-`)
