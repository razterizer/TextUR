# Release Notes

## 1.3.3.11
- Better shaped big brush strokes using mathematical algorithm rather than hard hacked pattern.
- Feature: Added command line arguments that allows you to configure the size and aspect ratio of big brush strokes.

## 1.3.2.10
- Bugfix: picked non-preset textel didn't properly propagate to the Ad Hoc textel editor.

## 1.3.1.9
- Making sure the shadow textel of the Ad Hoc preset is set to the same values as the normal textel to avoid certain problems that may arise.
- Adding missing UI help (guide) for the edit Ad Hoc textel command ('e').

## 1.3.0.8
- Added Ad Hoc textel feature. This textel preset is at the top of the list and can be quickly edited with 'e' whereas 'E' works like "any-case" 'e' before, i.e. edit or add a custom textel preset.
- Added key press legend ('k').
- Only supporting 4-bit colors on Windows cmd.exe unless explicitly setting command line argument `--force_8bit_colors_on_win_cmd`.

## 1.2.0.7
- TextUR now works with Termin8or version 2.0.0.2.
- Now uses full 256 color support by Termin8or 2.0.0.2.
- Uses new redesigned UI widget ColorPicker with all 256 colors (Termin8or 2.0.0.2).
- To make the new ColorPicker widgets fit into the textel edit dialogs, I thus had to increase the size of the TextUR window/screen/area/canvas (or whatever is best to call it).
- Indented help text.
- Now the help text is printed out if we didn't supply any command line arguments.
- If `-f <filename>` (with no `-s` argument supplied) fails to find the file, we simply exit.

## 1.1.2.6
- Renaming extension for example textures from .tex to .tx so that github doesn't think I have a lot of LaTeX or TeX files in this repo :P.

## 1.1.1.5
- Added unblocking instructions in readme for macos and windows and added additional chmodding instructions in readme for linux.

## 1.1.0.4
- Oops. Another copy-paste typo error. Getting sloppy in the morning.
- Trying building two different binaries for linux.
- One which uses glibc2.38 (e.g. ubuntu latest), and another one that uses glibc2.35 (e.g. ubuntu wsl).

## 1.0.3.3
- Oops.

## 1.0.2.2
- Fixed bug in textfield locations for dialog_goto UI dialog. I guess it was due to altered coordinate origin some time ago.

## 1.0.1.1
- Fixed partial redraw for linux and wsl. So now the rendering should l…

## 1.0.0.0
- Added new script retag_release.sh to xcode project.

