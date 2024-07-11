# TextUR

Text-based texture UI and Rasterizer.

A terminal-based editor for text-based textures or images.
This is a tool for generating textures to be used with [`DungGine`](https://github.com/razterizer/DungGine/).

## Command Line Arguments

 * Load existing texture : `./textel -f <filename>`.
 * Create new texture or overwrite existing texture : `./textel -f <filename> -s <num_rows> <num_cols>`.
 * Trace over another texture : `./textel -f <main_texture_filename> -t <trace_texture_filename>`.
 * Convert texture made up of bright textels from the textel presets in TextUR to a corresponding dark texture which then can be used for rendering shadows in e.g. `DungGine`.

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
 * `T` : toggle show/hide of tracing texture.
 * `I` : toggle inverted textels (i.e. toggle between dark and bright textel presets).
 * `Q` : quit.

## Screenshots

<img width="564" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/1628f6b5-0956-4c56-b307-13a5a4dea5e7">
<img width="562" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/a8bf9850-5499-49fe-8aea-a98cd00f0f01">
<img width="567" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/850cc9cd-2b12-441a-a888-6a55c746044a">
<img width="563" alt="image" src="https://github.com/razterizer/TextUR/assets/32767250/583a0ee8-8a0f-487a-844e-1cee131fc3ee">
