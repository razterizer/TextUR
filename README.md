# TextUR

Text-based texture UI and Rasterizer.

A terminal-based editor for text-based textures or images.
This is a tool for generating textures to be used with [`DungGine`](https://github.com/razterizer/DungGine/).

## Command Line Arguments

 * Load existing texture : `./textel -f <filename>`.
 * Create new texture or overwrite existing texture : `./textel -f <filename> -s <num_rows> <num_cols>`.
 * Trace over another texture : `./textel -f <main_texture_filename> -t <trace_texture_filename>`.
 * Convert texture made up of bright textels from the textel presets in TextUR to a corresponding dark texture which then can be used for rendering shadows in e.g. `DungGine`. The program exits when conversion is completed : 
`./textel -f <source_texture_filename> -c <target_texture_filename>`.
## Keys

 * WASD or arrow keys to navigate the cursor or selecting a textel preset.
 * Space : enter selected textel preset under cursor.
 * `Z` : undo.
 * `SHIFT + Z` : redo.
 * `C` : clear textel under cursor.
 * `V` : toggle drawing of vertical coordinates.
 * `H` : toggle drawing of horizontal coordinates.
 * `-` : toggle hide/show textel presets menu.
 * `X` : export (save) work to current file.
 * `B` : brush-stroke. Forms a circle, filled with the currently selected textel preset.
 * `L` : show location of cursor.
 * `G` : goto new cursor location. Press backspace to clear the last digit, press tab to toggle between R and C coordinate fields and press enter to confirm. Pressing `G` again toggles the input box.
 * `T` : toggle show/hide of tracing texture.
 * `I` : toggle inverted textels (i.e. toggle between dark and bright textel presets).
 * `Q` : quit.

## Screenshots

<img width="564" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/1628f6b5-0956-4c56-b307-13a5a4dea5e7">
<img width="562" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/a8bf9850-5499-49fe-8aea-a98cd00f0f01">
<img width="564" alt="image" src="https://github.com/user-attachments/assets/a33490ae-3921-431d-8d15-477f2566b279">
<img width="563" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/583a0ee8-8a0f-487a-844e-1cee131fc3ee">
<img width="568" alt="image" src="https://github.com/user-attachments/assets/0286226c-a18c-48b6-98d3-adc183e51bc2">
<img width="562" alt="image" src="https://github.com/user-attachments/assets/18ddb12d-acec-476d-81f8-6798624ede9d">
<img width="567" alt="image" src="https://github.com/user-attachments/assets/4d79d619-63f6-435b-a2c4-3477006c7980">



## Build & Run Instructions

There are two options on dealing with repo dependencies:
### Repo Dependencies Option 1

Run the following command from `<my_source_code_dir>`.
```sh
./TextUR/fetch-dependencies ./TextUR/dependencies
```
This will make sure you are running the latest stable versions that work with `TextUR`.

This script was created by [Thibaut Buchert](https://github.com/thibautbuchert).

### Repo Dependencies Option 2

You need the following header-only libraries that I've made:
* https://github.com/razterizer/Core
* https://github.com/razterizer/Termin8or

Make sure the folder structure looks like this:
```
<my_source_code_dir>/lib/Core/                   ; Core repo workspace/checkout goes here.
<my_source_code_dir>/lib/Termin8or/              ; Termin8or repo workspace/checkout goes here.
<my_source_code_dir>TextUR/                      ; TextUR repo workspace/checkout goes here.
```

These repos are not guaranteed to all the time work with the latest version of `TextUR`. If you want the more stable aproach then look at Option 1 instead.

### Windows (not yet implemented)

**NOTE: WIP here. No VS solution file yet.**
Then just open `<my_source_code_dir>/TextUR/TextUR/TextUR.sln` and build and run. That's it!

### MacOS

Goto `<my_source_code_dir>/TextUR/TextUR/` and build with `./build.sh`.

Then run by typing `./bin/textur` and apply the appropriate command line arguments (see beginning of README.md).

### Linux (Ubuntu)

Goto `<my_source_code_dir>/TextUR/TextUR/` and build with `./build.sh`.

Then run by typing `./bin/textur` and apply the appropriate command line arguments (see beginning of README.md).
