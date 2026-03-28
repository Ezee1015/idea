<div align="center">
  <img src="./resources/icon/icon_192x192.png" height="90" />
  <h1>idea</h1>
</div>
<br>

A simple To-Do App written in C that fits my needs

> [!NOTE]
> This project is a work in progress so anything can change or break without notice

## Compilation

### Requirements:
- `ncurses`
- `sed`

For the scripts:
- `inotify-tools`
- `base64`
- `imagemagick` (`convert`)
- `gettext` or `gettext-base` (for `envsubst`)

### Available `make` commands
- `make`: Compile
- `make clean`: Clean the project
- `make install`: Install the binary into `/usr/local/bin/`
- `make uninstall`: Uninstall the binary from the system
- `make test`: Run the tests. For more options, such as memory leak checking, multi-thread, and output logging, see `./build/tests -h`

> When running `make install` it will install the idea scripts too. You can see more information about each script inside the `scripts` directory

## Run
This program has 2 interfaces: CLI or TUI.
- You can see all the available CLI options with `./build/idea -h`
- or run the TUI version by only running `./build/idea` and typing `:help` (you can change pages with `h` and `l`, and increase or decrease the number of items shown with `+` or `-`)

> When executing idea through the CLI, you can specify the `-m` flag to pass multiple instructions to a single instance.
>
> Examples:
> ```bash
> $ idea add todo # Single instruction
> $ idea -m "list" "add todo" "list" "reminders" # Multiple instructions
> ```

## Note taking system

> To integrate idea with Neovim for note taking: [idea.lua](https://github.com/Ezee1015/dotfiles/blob/main/configs/nvim/lua/idea.lua)

### Tags
List of tags separated by spaces with the `tags:` keyword at the start of the line.

### Tasks
A task is a line that starts with one of the following tasks statuses:
- `- [ ]`: a task that has not started yet
- `- [x]`: a complete task
- `- [?]`: a tasks with a question attached
- `- [-]`: in progress / paused
- `- [~]`: removed / won't do

### Reminders
A reminder is a line that starts with the keyword `reminder:` and the template is the next: `reminder: [year]-[month]-[day] [name]`

### Note example:
```md
# Operating System group project

tags: university operating\ systems project

reminder: 2025-04-19 First checkpoint
reminder: 2025-05-24 Second checkpoint
reminder: 2025-06-21 Third checkpoint
reminder: 2025-07-12 Live test of the project at university and oral defense

---

## Work

- [x] 1: Organize work in the group
      reminder: 2025-03-30 Video call to organize the work

    - [x] 1.1: Install VM
    - [-] 1.2: Read the [docs](https://docs.utnso.com.ar/)
    - [x] 1.3: Register group for the project on the utnso website

- [ ] 2: Create the protocol of communication
      reminder: 2025-03-31 Do task #2

    - [?] 2.1: Investigate about the handshake in C
          Question: What should the program do if the handshake fails? How can it happen?
    - [ ] 2.2: Code the utils for the different modules
    - [ ] 2.3: Test the code with various messages

## Checkpoint
For the checkpoint, remember to execute this commands:

> `git tag -a checkpoint-{number} -m "Checkpoint {number}"`
> `git push origin checkpoint-{number}`

## Live test and oral defense

- The live test is done by the group with the scripts given by the professors.
- The oral defense is individual

# HTML Tip

<style> marquee { color: var(--green); } </style>
<marquee behavior="alternate">You can even embed HTML (only inline) for the output HTML file!</marquee>
<script>console.log("Hello from JS!")</script>

<img src="https://docs.utnso.com.ar/img/logo.gif" alt="Tux">
```
