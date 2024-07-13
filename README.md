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

 * `WASD` (lower case) or arrow keys : navigates the cursor or selects a textel preset in the textel menu. When in the textel menu, left and right (or `A` and `D`) scrolls from material to material for quicker navigation among the different textel presets.
 * `SHIFT + WASD` (upper case) keys : scrolls the texture page-wise.
 * Space : enter selected textel preset under cursor.
 * `Z` : undo.
 * `SHIFT + Z` : redo.
 * `C` : clear textel under cursor.
 * `V` : toggle drawing of vertical coordinates.
 * `H` : toggle drawing of horizontal coordinates.
 * `SHIFT + V` : toggle drawing of vertical guide line from the horizontal coordinate axis.
 * `SHIFT + H` : toggle drawing of horizontal guide line from the vertical coordinate axis.
 * `-` : toggle hide/show textel presets menu.
 * `X` : export (save) work to current file.
 * `B` : brush-stroke. Forms a circle, filled with the currently selected textel preset.
 * `SHIFT + B` : big brush-stroke.
 * `L` : show location of cursor.
 * `G` : goto new cursor location. Press backspace to clear the last digit, press tab to toggle between R and C coordinate fields and press enter to confirm. Pressing `G` again toggles the input box.
 * `T` : toggle show/hide of tracing texture.
 * `I` : toggle inverted textels (i.e. toggle between dark and bright textel presets).
 * `M` : toggle show/hide of material id:s. 
 * `Q` : quit.

## Custom Textel Presets

Add a file named `custom_textel_presets`.
The file format looks like this:
```
'<normal-char>', <normal-fg-color>, <normal-bg-color>, <normal-material>
'<shadow-char>', <shadow-fg-color>, <shadow-bg-color>, <shadow-material>
<textel-preset-name>
'<normal-char>', <normal-fg-color>, <normal-bg-color>, <normal-material>
'<shadow-char>', <shadow-fg-color>, <shadow-bg-color>, <shadow-material>
<textel-preset-name>
...etc...
```
So e.g.
```
'%', Magenta, Cyan, 24
'%', DarkMagenta, DarkCyan, 24
Magic Stone
```
produces the following textel preset at the end of the list:

<img width="210" alt="image" src="https://github.com/user-attachments/assets/30343240-befd-4242-a60c-1cbe4f72b992">

Look in the source code for which material number that is appropriate to use for your custom presets.

## Screenshots

<img width="564" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/1628f6b5-0956-4c56-b307-13a5a4dea5e7">
<img width="562" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/a8bf9850-5499-49fe-8aea-a98cd00f0f01">
<img width="564" alt="image" src="https://github.com/user-attachments/assets/a33490ae-3921-431d-8d15-477f2566b279">
<img width="568" alt="image" src="https://github.com/user-attachments/assets/0286226c-a18c-48b6-98d3-adc183e51bc2">
<img width="562" alt="image" src="https://github.com/user-attachments/assets/18ddb12d-acec-476d-81f8-6798624ede9d">
<img width="567" alt="image" src="https://github.com/user-attachments/assets/eee16100-d7f4-4ea5-936b-1826b185a6e3">
<img width="567" alt="image" src="https://github.com/user-attachments/assets/4d79d619-63f6-435b-a2c4-3477006c7980">
<img width="565" alt="image" src="https://github.com/user-attachments/assets/cdbd8aa9-40ab-4eaa-85f5-73f864206e4b">


Bright mode textels (normal mode):

<img width="565" alt="image" src="https://github.com/user-attachments/assets/71eac9f5-a9d2-4c61-a6c4-b4bbaea4ea02">

Dark mode textels (shadow mode):

<img width="564" alt="image" src="https://github.com/user-attachments/assets/dc68fc8a-bb14-45b6-8c4f-f701034ffc37">

Dark mode texture (shadow texture):

<img width="562" alt="image" src="https://github.com/user-attachments/assets/ad90f507-8797-45ac-8781-47be1ed6ecdc">

Unsaved changes indicator:

<img width="130" alt="image" src="https://github.com/user-attachments/assets/25d7d238-6d35-441e-96cd-463edf94973b">

Confirmation screen for attempting to quit while there are unsaved changes:

<img width="565" alt="image" src="https://github.com/user-attachments/assets/1b5e0b9a-41c8-4098-a0b0-77771cf32456">

Custom textel "Magic Stone":

<img width="210" alt="image" src="https://github.com/user-attachments/assets/30343240-befd-4242-a60c-1cbe4f72b992">



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
