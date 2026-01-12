# idea
A simple To-Do App written in C that fits my needs

## Compilation
Requirements: `ncurses`, `sed`

- Available `make` commands
    - `make`: Compile
    - `make clean`: Clean the project
    - `make install`: Install the binary into `/usr/local/bin/`
    - `make uninstall`: Uninstall the binary from the system
    - `make test`: Run the tests. For more options, such as memory leak checking and output logging, see `./build/tests -h`


## Run
You can see all the available CLI options with `./build/idea -h` or run the TUI version by only running `./build/idea` and typing `:help` (you can change pages with `h` and `l`, and increase or decrease the number of items show with `+` or `-`)

## Help
Each interface can implement its own commands, but you can do `help` in the interface you're using to see the available commands. For example, you can run `./build/idea help` to see the available commands for the CLI.
