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
You can see all the available options with `./build/idea -h` or run the TUI version with only running `./build/idea`

### TUI maps
- `j`: Move the cursor down
- `k`: Move the cursor up
- `[1-9]`: Number of times to repeat the command (command multiplier)
- `[ESCAPE]`: Clear the command multiplier
- `[SPACE]`: Toggle select
- `V`: Enter visual mode
- `J`: Move the selected ToDos down
- `K`: Move the selected ToDos up
- `g`: Move the cursor and the selected items to the top
- `G`: Move the cursor and the selected items to the bottom
- `d`: Delete the selected ToDos
- `q`: Exit idea
- `u`: Unselect all the selected items
- `:`: Enter command mode

### TUI commands
- `:q`: Exit the application without saving the changes
- `:wq`: Exit and save the changes
- `:help`, `:h`: See help

## Help
Each interface can implement its own commands, but you can do `help` in the interface you're using to see the available commands. For example, you can run `./build/idea help` to see the available commands for the CLI.
