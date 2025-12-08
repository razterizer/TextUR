//
//  main.cpp
//  TextUR
//
//  Created by Rasmus Anthin on 2024-07-01.
//

#include <Termin8or/sys/GameEngine.h>
#include <Termin8or/screen/ScreenUtils.h>
#include <Termin8or/drawing/Drawing.h>
#include <Termin8or/drawing/Texture.h>
#include <Termin8or/geom/RC.h>
#include <Termin8or/ui/MessageHandler.h>
#include <Termin8or/ui/UI.h>
#include <Core/Rand.h>

#include <iostream>
#include <stack>

using namespace std::string_literals;
using Color16 = t8::Color16;
using RC = t8::RC;
using Textel = t8::Textel;
using Texture = t8x::Texture;

enum class EditorFileMode { NEW_OR_OVERWRITE_FILE, OPEN_EXISTING_FILE };


class Game : public t8x::GameEngine<44, 92>
{
  struct TextelItem
  {
    TextelItem(Textel tn, Textel ts, std::string a_name)
      : textel_normal(std::move(tn))
      , textel_shadow(std::move(ts))
      , name(std::move(a_name))
    {}
    
    Textel textel_normal;
    Textel textel_shadow;
    std::string name;
    
    Textel get_textel(bool shadow) const
    {
      return shadow ? textel_shadow : textel_normal;
    }
  };
  
  void show_help() const
  {
    std::cout << "textur --help |" << std::endl;
    std::cout << "   -f <filepath_texture>" << std::endl;
    std::cout << "   [-s <rows> <cols>]" << std::endl;
    std::cout << "   [-t <filepath_tracing_texture>]" << std::endl;
    std::cout << "   [-c <filepath_dark_texture>]" << std::endl;
    std::cout << "   [--log_mode (record | replay)]" << std::endl;
    std::cout << "   [--suppress_tty_output]" << std::endl;
    std::cout << "   [--suppress_tty_input]" << std::endl;
    std::cout << std::endl;
    std::cout << "  -f                         : Specifies the source file to (create and) edit." << std::endl;
    std::cout << "  <filepath_texture>         : Filepath for texture to edit. If file does not yet exist," << std:: endl;
    std::cout << "                               then you need to supply the -s argument as well." << std::endl;
    std::cout << "  -s                         : Specifies the size of a new texture." << std::endl;
    std::cout << "                             : If <filepath_texture> already exists, then it will be overwritten." << std::endl;
    std::cout << "  -t                         : Specifies a tracing texture." << std::endl;
    std::cout << "  <filepath_tracing_texture> : Filepath to tracing texture. Allows you to do animations." << std::endl;
    std::cout << "  -c                         : Specifies a file to convert the current light mode texture" << std::endl;
    std::cout << "                               <filepath_texture> to a dark mode texture." << std::endl;
    std::cout << "  <filepath_dark_texture>    : The destination filepath to the generated dark mode texture." << std::endl;
    std::cout << std::endl;
    std::cout << "  Press 'K' in editor for list of supported key presses." << std::endl;
    exit(EXIT_SUCCESS);
  }

  void draw_menu(const t8::Style& ui_style, const int menu_width)
  {
    //const int nr = sh.num_rows();
    const int nri = sh.num_rows_inset();
    const int nc = sh.num_cols();
  
    int r = menu_r_offs;
    const int num_textel_presets = static_cast<int>(textel_presets.size());
    for (int p_idx = 0; p_idx < num_textel_presets; ++p_idx)
    {
      auto name_style = ui_style;
      if (p_idx == selected_textel_preset_idx)
      {
        name_style.fg_color = Color16::Cyan;
        if (r + 2 >= nri)
          menu_r_offs -= 3;
        else if (r < 0)
          menu_r_offs += 3;
      }
      const auto& preset = textel_presets[p_idx];
      const auto& textel = preset.get_textel(use_shadow_textels);
      sh.write_buffer(textel.str(), r + 1, nc - menu_width + 2, textel.get_style());
      sh.write_buffer(preset.name, r + 2, nc - menu_width + 2, name_style);
      // Does not need to be qualified with t8x::drawing, but I'm not sure why.
      t8x::draw_box_outline(sh, r, nc - menu_width, 4, menu_width, t8x::OutlineType::Line, ui_style);
      r += 3;
    }
  }
  
  void draw_coord_sys(bool draw_v_coords, bool draw_h_coords,
                      bool draw_v_cursor_line, bool draw_h_cursor_line,
                      int nc, int menu_width)
  {
    static const bool persist = true;
    if (draw_v_coords)
    {
      const int str_max_len = curr_texture.size.r == 0 ? 0 : static_cast<int>(1 + std::log10(std::max(1, curr_texture.size.r - 1)));
      for (int r = 0; r < curr_texture.size.r; ++r)
        sh.write_buffer(str::adjust_str(std::to_string(r), str::Adjustment::Right, str_max_len), screen_pos.r + r + 1, persist ? 1 : screen_pos.c + 1, Color16::Red);
    }
    
    if (draw_h_coords)
    {
      const int str_max_len = curr_texture.size.c == 0 ? 0 : static_cast<int>(1 + std::log10(std::max(1, curr_texture.size.c - 1)));
      int num_cols = curr_texture.size.c;
      if (show_menu && curr_texture.size.c > nc - menu_width)
        num_cols = nc - menu_width - screen_pos.c;
      for (int c = 0; c < num_cols; ++c)
      {
        auto str = str::adjust_str(std::to_string(c), str::Adjustment::Right, str_max_len);
        for (int r = 0; r < str_max_len; ++r)
          sh.write_buffer(std::string(1, str[r]), persist ? r + 1 : screen_pos.r + r + 1, screen_pos.c + c + 1, Color16::Green);
      }
    }
    
    if (draw_h_cursor_line)
      sh.write_buffer(str::rep_char('-', screen_pos.c + cursor_pos.c), screen_pos.r + cursor_pos.r + 1, 1, Color16::Red, Color16::Transparent2);

    
    if (draw_v_cursor_line)
      for (int r = 0; r < cursor_pos.r + screen_pos.r; ++r)
        sh.write_buffer(std::string(1, '|'), r + 1, screen_pos.c + cursor_pos.c + 1, Color16::Green, Color16::Transparent2);
  }
  
  void init_keys_legend()
  {
    int padding = 2; // #FIXME: No source of truth yet.
    int dialog_ext_padding = 8;
    auto nc = sh.num_cols_inset() - padding - dialog_ext_padding;
    std::string title = " Keys: ";
    auto hw = (nc - str::lenI(title))/2 - 2;
    dialog_keys = t8x::Dialog({ str::rep_char('=', hw) + title + str::rep_char('=', hw),
      "",
      "K : open or close this window.",
      "Esc : close windows.",
      "WASD or arrow keys : cursor navigation or textel preset selection (menu).",
      "  In textel menu, a or left / d or right scrolls from material to material.",
      "SHIFT + WASD : scrolls the texture page-wise.",
      "Space : insert selected textel preset under cursor.",
      "Z : undo.",
      "SHIFT + Z : redo.",
      "C : clear textel under cursor.",
      "V : toggle drawing of vertical coordinates.",
      "H : toggle drawing of horizontal coordinates.",
      "SHIFT + V : toggle drawing of vertical guide line from the horiz coord axis.",
      "SHIFT + H : toggle drawing of horizontal guide line from the vert coord axis.",
      "- : toggle hide/show textel presets menu.",
      "X : export (save) work to current file.",
      "B : Circle-shaped brush stroke, filled with selected textel preset.",
      "SHIFT + B : big brush-stroke.",
      "R : randomized brush-stroke.",
      "  Same as the B key, but fills the circle with textels according to a",
      "  normal distribution. You can re-generate until you get the desired result.",
      "SHIFT + R : randomized big brush-stroke.",
      "  Same as the SHIFT + B key, but fills the circle with textels according to a",
      "  normal distribution. You can re-generate until you get the desired result.",
      "F : fill screen. Fills the texture with the currently selected textel preset",
      "  where the bounding boxof the screen is currently located over the texture.",
      "P : pick a textel from cursor and hilite the matching preset in the menu.",
      "L : show location of cursor.",
      "G : goto new cursor location.",
      "T : toggle show/hide of tracing texture.",
      "I : toggle between dark and bright textel presets.",
      "M : toggle show/hide of material id:s.",
      "Q : quit."
    });
    //dialog_keys.set_textel_pre(const RC& local_pos, char ch, Color fg_color, Color bg_color)
    dialog_keys.set_tab_order(0);
  }
  
  void reset_goto_input()
  {
    dialog_goto = t8x::Dialog({ "Cursor Goto @"s, str::rep_char(' ', 8) + ", " + str::rep_char(' ', 8) });
    dialog_goto.add_text_field({ 1, 0 }, tf_goto_r);
    dialog_goto.add_text_field({ 1, 10 }, tf_goto_c);
    dialog_goto.set_tab_order(0);
  }
  
  void reset_textel_editor()
  {
    edit_mode = EditTextelMode::EditOrAdd;
    dialog_edit_or_add = t8x::Dialog {{ "Edit or Add Custom Textel Preset?"s }};
    dialog_edit_or_add.add_button(btn_edit);
    dialog_edit_or_add.add_button(btn_add);
    dialog_edit_or_add.set_button_selection(0, true);
    dialog_edit_mat = t8x::Dialog({ "Enter Custom Textel Preset Index"s, "Idx:" + str::rep_char(' ', 4) });
    dialog_edit_mat.add_text_field({ 1, 5 }, tf_textel_idx);
    dialog_edit_mat.set_tab_order(0);
    dialog_editor = t8x::Dialog({ "Custom Textel Preset Editor (Normal)    "s, "Textel:", "Idx:", "Name:", "Char:",
      "FG Color:", "", "", "", "", "", "", "", "",
      "BG Color:", "", "", "", "", "", "", "", "",
      "Mat:" });
    dialog_editor.add_text_field({ 3, 6 }, tf_textel_name);
    dialog_editor.add_text_field({ 4, 6 }, tf_textel_symbol);
    dialog_editor.add_color_picker({ 6, 3 }, cp_textel_fg);
    dialog_editor.add_color_picker({ 6 + cp_textel_fg.height() + 1, 3 }, cp_textel_bg);
    dialog_editor.add_text_field({ 6 + cp_textel_fg.height() + 1 + cp_textel_bg.height(), 5 }, tf_textel_mat);
    dialog_editor.set_tab_order(0);
  }
  
  void reload_textel_presets()
  {
    textel_presets.clear();
    custom_textel_presets.clear();
    textel_presets.emplace_back(Textel { ' ', Color16::Default, Color16::Black, 0 },
                                Textel { ' ', Color16::Default, Color16::Black, 0 },
                                "Void");
    textel_presets.emplace_back(Textel { '~', Color16::DarkCyan, Color16::Cyan, 2 },
                                Textel { '~', Color16::Cyan, Color16::DarkCyan, 2 },
                                "Water0");
    textel_presets.emplace_back(Textel { '*', Color16::White, Color16::Cyan, 2 },
                                Textel { '*', Color16::LightGray, Color16::DarkCyan, 2 },
                                "Water1");
    textel_presets.emplace_back(Textel { '~', Color16::Cyan, Color16::DarkCyan, 2 },
                                Textel { '~', Color16::DarkCyan, Color16::Blue, 2 },
                                "Water2");
    textel_presets.emplace_back(Textel { '*', Color16::LightGray, Color16::DarkCyan, 2 },
                                Textel { '*', Color16::DarkGray, Color16::Blue, 2 },
                                "Water3");
    textel_presets.emplace_back(Textel { '~', Color16::DarkBlue, Color16::Blue, 2 },
                                Textel { '~', Color16::Blue, Color16::DarkBlue, 2 },
                                "Water4");
    textel_presets.emplace_back(Textel { '*', Color16::White, Color16::Blue, 2 },
                                Textel { '*', Color16::LightGray, Color16::DarkBlue, 2 },
                                "Water5");
    textel_presets.emplace_back(Textel { '~', Color16::Blue, Color16::DarkBlue, 2 },
                                Textel { '~', Color16::DarkBlue, Color16::Black, 2 },
                                "Water6");
    textel_presets.emplace_back(Textel { '*', Color16::LightGray, Color16::DarkBlue, 2 },
                                Textel { '*', Color16::DarkGray, Color16::Black, 2 },
                                "Water7");
    textel_presets.emplace_back(Textel { ':', Color16::DarkYellow, Color16::Yellow, 3 },
                                Textel { ':', Color16::Yellow, Color16::DarkYellow, 3 },
                                "Sand0");
    textel_presets.emplace_back(Textel { '.', Color16::DarkYellow, Color16::Yellow, 3 },
                                Textel { '.', Color16::Yellow, Color16::DarkYellow, 3 },
                                "Sand1");
    textel_presets.emplace_back(Textel { '.', Color16::DarkGray, Color16::LightGray, 22 },
                                Textel { '.', Color16::LightGray, Color16::DarkGray, 22 },
                                "Gravel0");
    textel_presets.emplace_back(Textel { ':', Color16::DarkGray, Color16::LightGray, 22 },
                                Textel { ':', Color16::LightGray, Color16::DarkGray, 22 },
                                "Gravel1");
    textel_presets.emplace_back(Textel { '.', Color16::Black, Color16::LightGray, 22 },
                                Textel { '.', Color16::Black, Color16::DarkGray, 22 },
                                "Gravel2");
    textel_presets.emplace_back(Textel { ':', Color16::Black, Color16::LightGray, 22 },
                                Textel { ':', Color16::Black, Color16::DarkGray, 22 },
                                "Gravel3");
    textel_presets.emplace_back(Textel { '8', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { '8', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone0");
    textel_presets.emplace_back(Textel { 'o', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'o', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone1");
    textel_presets.emplace_back(Textel { 'O', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'O', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone2");
    textel_presets.emplace_back(Textel { 'b', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'b', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone3");
    textel_presets.emplace_back(Textel { 'B', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'B', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone4");
    textel_presets.emplace_back(Textel { 'p', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'p', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone5");
    textel_presets.emplace_back(Textel { 'P', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'P', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone6");
    textel_presets.emplace_back(Textel { 'q', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'q', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone7");
    textel_presets.emplace_back(Textel { '6', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { '6', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone8");
    textel_presets.emplace_back(Textel { '9', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { '9', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone9");
    textel_presets.emplace_back(Textel { 'c', Color16::DarkGray, Color16::LightGray, 4 },
                                Textel { 'c', Color16::LightGray, Color16::DarkGray, 4 },
                                "Stone10");
    textel_presets.emplace_back(Textel { '8', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { '8', Color16::DarkGray, Color16::Black, 4 },
                                "Stone11");
    textel_presets.emplace_back(Textel { 'o', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'o', Color16::DarkGray, Color16::Black, 4 },
                                "Stone12");
    textel_presets.emplace_back(Textel { 'O', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'O', Color16::DarkGray, Color16::Black, 4 },
                                "Stone13");
    textel_presets.emplace_back(Textel { 'b', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'b', Color16::DarkGray, Color16::Black, 4 },
                                "Stone14");
    textel_presets.emplace_back(Textel { 'B', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'B', Color16::DarkGray, Color16::Black, 4 },
                                "Stone15");
    textel_presets.emplace_back(Textel { 'p', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'p', Color16::DarkGray, Color16::Black, 4 },
                                "Stone16");
    textel_presets.emplace_back(Textel { 'P', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'P', Color16::DarkGray, Color16::Black, 4 },
                                "Stone17");
    textel_presets.emplace_back(Textel { 'q', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'q', Color16::DarkGray, Color16::Black, 4 },
                                "Stone18");
    textel_presets.emplace_back(Textel { '6', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { '6', Color16::DarkGray, Color16::Black, 4 },
                                "Stone19");
    textel_presets.emplace_back(Textel { '9', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { '9', Color16::DarkGray, Color16::Black, 4 },
                                "Stone20");
    textel_presets.emplace_back(Textel { 'c', Color16::LightGray, Color16::DarkGray, 4 },
                                Textel { 'c', Color16::DarkGray, Color16::Black, 4 },
                                "Stone21");
    textel_presets.emplace_back(Textel { '^', Color16::DarkGray, Color16::LightGray, 13 },
                                Textel { '^', Color16::LightGray, Color16::DarkGray, 13 },
                                "Mountain0");
    textel_presets.emplace_back(Textel { '^', Color16::LightGray, Color16::White, 13 },
                                Textel { '^', Color16::DarkGray, Color16::LightGray, 13 },
                                "Mountain1");
    textel_presets.emplace_back(Textel { 'W', Color16::DarkRed, Color16::Red, 14 },
                                Textel { 'W', Color16::Red, Color16::DarkRed, 14 },
                                "Lava");
    textel_presets.emplace_back(Textel { 'C', Color16::DarkYellow, Color16::Yellow, 15 },
                                Textel { 'C', Color16::Yellow, Color16::DarkYellow, 15 },
                                "Cave0");
    textel_presets.emplace_back(Textel { 'U', Color16::DarkYellow, Color16::Yellow, 15 },
                                Textel { 'U', Color16::Yellow, Color16::DarkYellow, 15 },
                                "Cave1");
    textel_presets.emplace_back(Textel { 'S', Color16::DarkRed, Color16::Green, 16 },
                                Textel { 'S', Color16::Red, Color16::DarkGreen, 16 },
                                "Swamp0");
    textel_presets.emplace_back(Textel { 'B', Color16::DarkRed, Color16::Green, 16 },
                                Textel { 'B', Color16::Red, Color16::DarkGreen, 16 },
                                "Swamp1");
    textel_presets.emplace_back(Textel { 'C', Color16::DarkRed, Color16::Green, 16 },
                                Textel { 'C', Color16::Red, Color16::DarkGreen, 16 },
                                "Swamp2");
    textel_presets.emplace_back(Textel { 'P', Color16::DarkRed, Color16::Green, 16 },
                                Textel { 'P', Color16::Red, Color16::DarkGreen, 16 },
                                "Swamp3");
    textel_presets.emplace_back(Textel { 'S', Color16::Green, Color16::DarkRed, 16 },
                                Textel { 'S', Color16::DarkGreen, Color16::DarkRed, 16 },
                                "Swamp4");
    textel_presets.emplace_back(Textel { 'B', Color16::Green, Color16::DarkRed, 16 },
                                Textel { 'B', Color16::DarkGreen, Color16::DarkRed, 16 },
                                "Swamp5");
    textel_presets.emplace_back(Textel { 'C', Color16::Green, Color16::DarkRed, 16 },
                                Textel { 'C', Color16::DarkGreen, Color16::DarkRed, 16 },
                                "Swamp6");
    textel_presets.emplace_back(Textel { 'P', Color16::Green, Color16::DarkRed, 16 },
                                Textel { 'P', Color16::DarkGreen, Color16::DarkRed, 16 },
                                "Swamp7");
    textel_presets.emplace_back(Textel { '~', Color16::DarkGreen, Color16::Green, 17 },
                                Textel { '~', Color16::Green, Color16::DarkGreen, 17 },
                                "Poison0");
    textel_presets.emplace_back(Textel { 'o', Color16::DarkGreen, Color16::Green, 17 },
                                Textel { 'o', Color16::Green, Color16::DarkGreen, 17 },
                                "Poison1");
    textel_presets.emplace_back(Textel { '~', Color16::Magenta, Color16::Cyan, 24 },
                                Textel { '~', Color16::DarkMagenta, Color16::DarkCyan, 24 },
                                "Acid0");
    textel_presets.emplace_back(Textel { 'o', Color16::Magenta, Color16::Cyan, 24 },
                                Textel { 'o', Color16::DarkMagenta, Color16::DarkCyan, 24 },
                                "Acid1");
    textel_presets.emplace_back(Textel { '~', Color16::LightGray, Color16::Black, 26 },
                                Textel { '~', Color16::DarkGray, Color16::Black, 26 },
                                "Tar");
    textel_presets.emplace_back(Textel { '#', Color16::DarkYellow, Color16::Green, 18 },
                                Textel { '#', Color16::Yellow, Color16::DarkGreen, 18 },
                                "Path");
    textel_presets.emplace_back(Textel { 'M', Color16::DarkGray, Color16::LightGray, 19 },
                                Textel { 'M', Color16::LightGray, Color16::DarkGray, 19 },
                                "Mine");
    textel_presets.emplace_back(Textel { '|', Color16::DarkGreen, Color16::Green, 7 },
                                Textel { '|', Color16::Green, Color16::DarkGreen, 7 },
                                "Grass0");
    textel_presets.emplace_back(Textel { '.', Color16::DarkGreen, Color16::Green, 7 },
                                Textel { '.', Color16::Green, Color16::DarkGreen, 7 },
                                "Grass1");
    textel_presets.emplace_back(Textel { ':', Color16::DarkGreen, Color16::Green, 7 },
                                Textel { ':', Color16::Green, Color16::DarkGreen, 7 },
                                "Grass2");
    textel_presets.emplace_back(Textel { '/', Color16::DarkGreen, Color16::Green, 7 },
                                Textel { '/', Color16::Green, Color16::DarkGreen, 7 },
                                "Grass3");
    textel_presets.emplace_back(Textel { '\\', Color16::DarkGreen, Color16::Green, 7 },
                                Textel { '\\', Color16::Green, Color16::DarkGreen, 7 },
                                "Grass4");
    textel_presets.emplace_back(Textel { '|', Color16::DarkYellow, Color16::Green, 7 },
                                Textel { '|', Color16::Yellow, Color16::DarkGreen, 7 },
                                "Grass5");
    textel_presets.emplace_back(Textel { '.', Color16::DarkYellow, Color16::Green, 7 },
                                Textel { '.', Color16::Yellow, Color16::DarkGreen, 7 },
                                "Grass6");
    textel_presets.emplace_back(Textel { ':', Color16::DarkYellow, Color16::Green, 7 },
                                Textel { ':', Color16::Yellow, Color16::DarkGreen, 7 },
                                "Grass7");
    textel_presets.emplace_back(Textel { '/', Color16::DarkYellow, Color16::Green, 7 },
                                Textel { '/', Color16::Yellow, Color16::DarkGreen, 7 },
                                "Grass8");
    textel_presets.emplace_back(Textel { '\\', Color16::DarkYellow, Color16::Green, 7 },
                                Textel { '\\', Color16::Yellow, Color16::DarkGreen, 7 },
                                "Grass9");
    textel_presets.emplace_back(Textel { '&', Color16::DarkYellow, Color16::Green, 8 },
                                Textel { '&', Color16::Yellow, Color16::DarkGreen, 8 },
                                "Shrub0");
    textel_presets.emplace_back(Textel { '@', Color16::DarkGray, Color16::Green, 8 },
                                Textel { '@', Color16::LightGray, Color16::DarkGreen, 8 },
                                "Shrub1");
    textel_presets.emplace_back(Textel { '*', Color16::DarkGreen, Color16::Green, 8 },
                                Textel { '*', Color16::Green, Color16::DarkGreen, 8 },
                                "Shrub2");
    textel_presets.emplace_back(Textel { 'T', Color16::DarkRed, Color16::Green, 9 },
                                Textel { 'T', Color16::Red, Color16::DarkGreen, 9 },
                                "Tree0");
    textel_presets.emplace_back(Textel { 'Y', Color16::DarkRed, Color16::Green, 9 },
                                Textel { 'Y', Color16::Red, Color16::DarkGreen, 9 },
                                "Tree1");
    textel_presets.emplace_back(Textel { '_', Color16::Default, Color16::LightGray, 1 },
                                Textel { '_', Color16::Black, Color16::DarkGray, 1 },
                                "Tile0");
    textel_presets.emplace_back(Textel { '_', Color16::White, Color16::DarkGray, 1 },
                                Textel { '_', Color16::LightGray, Color16::Black, 1 },
                                "Tile1");
    textel_presets.emplace_back(Textel { '_', Color16::LightGray, Color16::White, 1 },
                                Textel { '_', Color16::White, Color16::LightGray, 1 },
                                "Tile2");
    textel_presets.emplace_back(Textel { 'L', Color16::Default, Color16::LightGray, 1 },
                                Textel { 'L', Color16::Black, Color16::DarkGray, 1 },
                                "Tile3");
    textel_presets.emplace_back(Textel { 'L', Color16::White, Color16::DarkGray, 1 },
                                Textel { 'L', Color16::LightGray, Color16::Black, 1 },
                                "Tile4");
    textel_presets.emplace_back(Textel { 'L', Color16::LightGray, Color16::White, 1 },
                                Textel { 'L', Color16::White, Color16::LightGray, 1 },
                                "Tile5");
    textel_presets.emplace_back(Textel { 'H', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { 'H', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry0");
    textel_presets.emplace_back(Textel { 'M', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { 'M', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry1");
    textel_presets.emplace_back(Textel { 'W', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { 'W', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry2");
    textel_presets.emplace_back(Textel { '=', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { '=', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry3");
    textel_presets.emplace_back(Textel { '#', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { '#', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry4");
    textel_presets.emplace_back(Textel { '@', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { '@', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry5");
    textel_presets.emplace_back(Textel { 'O', Color16::LightGray, Color16::DarkGray, 5 },
                                Textel { 'O', Color16::DarkGray, Color16::Black, 5 },
                                "Masonry6");
    textel_presets.emplace_back(Textel { 'I', Color16::White, Color16::LightGray, 25 },
                                Textel { 'I', Color16::LightGray, Color16::DarkGray, 25 },
                                "Column0");
    textel_presets.emplace_back(Textel { '=', Color16::White, Color16::LightGray, 25 },
                                Textel { '=', Color16::LightGray, Color16::DarkGray, 25 },
                                "Column1");
    textel_presets.emplace_back(Textel { '#', Color16::DarkRed, Color16::Red, 6 },
                                Textel { '#', Color16::Red, Color16::DarkRed, 6 },
                                "Brick");
    textel_presets.emplace_back(Textel { 'W', Color16::DarkRed, Color16::Yellow, 11 },
                                Textel { 'W', Color16::Yellow, Color16::DarkRed, 11 },
                                "Wood0");
    textel_presets.emplace_back(Textel { 'E', Color16::DarkRed, Color16::Yellow, 11 },
                                Textel { 'E', Color16::Yellow, Color16::DarkRed, 11 },
                                "Wood1");
    textel_presets.emplace_back(Textel { 'Z', Color16::DarkRed, Color16::Yellow, 11 },
                                Textel { 'Z', Color16::Yellow, Color16::DarkRed, 11 },
                                "Wood2");
    textel_presets.emplace_back(Textel { 'X', Color16::DarkBlue, Color16::Cyan, 12 },
                                Textel { 'X', Color16::Cyan, Color16::DarkBlue, 12 },
                                "Ice");
    textel_presets.emplace_back(Textel { '=', Color16::DarkGray, Color16::LightGray, 10 },
                                Textel { '=', Color16::LightGray, Color16::DarkGray, 10 },
                                "Metal");
    textel_presets.emplace_back(Textel { 'S', Color16::White, Color16::LightGray, 21 },
                                Textel { 'S', Color16::LightGray, Color16::DarkGray, 21 },
                                "Silver");
    textel_presets.emplace_back(Textel { 'G', Color16::DarkYellow, Color16::Yellow, 20 },
                                Textel { 'G', Color16::Yellow, Color16::DarkYellow, 20 },
                                "Gold");
    textel_presets.emplace_back(Textel { '@', Color16::White, Color16::DarkGray, 23 },
                                Textel { '@', Color16::LightGray, Color16::Black, 23 },
                                "Skull");
    textel_presets.emplace_back(Textel { '+', Color16::White, Color16::DarkGray, 23 },
                                Textel { '+', Color16::LightGray, Color16::Black, 23 },
                                "Bone0");
    textel_presets.emplace_back(Textel { '|', Color16::White, Color16::DarkGray, 23 },
                                Textel { '|', Color16::LightGray, Color16::Black, 23 },
                                "Bone1");
    textel_presets.emplace_back(Textel { '-', Color16::White, Color16::DarkGray, 23 },
                                Textel { '-', Color16::LightGray, Color16::Black, 23 },
                                "Bone2");
    textel_presets.emplace_back(Textel { '/', Color16::White, Color16::DarkGray, 23 },
                                Textel { '/', Color16::LightGray, Color16::Black, 23 },
                                "Bone3");
    textel_presets.emplace_back(Textel { '\\', Color16::White, Color16::DarkGray, 23 },
                                Textel { '\\', Color16::LightGray, Color16::Black, 23 },
                                "Bone4");
    textel_presets.emplace_back(Textel { '%', Color16::Red, Color16::Yellow, 27 },
                                Textel { '%', Color16::DarkRed, Color16::DarkYellow, 27 },
                                "Rope");
    
    std::vector<std::string> lines_custom_textel_presets;
    if (TextIO::read_file(filepath_custom_textel_presets, lines_custom_textel_presets))
    {
      int part = 0;
      Textel textel_normal, textel_shadow;
      for (const auto& line : lines_custom_textel_presets)
      {
        if (line.empty())
          continue;
        if (part == 0)
        {
          auto tokens = str::tokenize(line, { ' ', ',' }, { '\'', '[', ']' });
          if (tokens.size() == 4 && tokens[0].length() == 1)
          {
            textel_normal.ch = tokens[0][0];
            // "1, 2, 3" -> "rgb6:[1, 2, 3]
            if (str::count_substr(tokens[1], ", ") == 2)
              tokens[1] = "rgb6:[" + tokens[1] + "]";
            textel_normal.fg_color.parse(tokens[1]);
            textel_normal.bg_color.parse(tokens[2]);
            textel_normal.mat = std::atoi(tokens[3].c_str());
          }
          else
            std::cerr << "Unable to parse normal textel." << std::endl;
          part = 1;
        }
        else if (part == 1)
        {
          auto tokens = str::tokenize(line, { ' ', ',' }, { '\'', '[', ']' });
          if (tokens.size() == 4 && tokens[0].length() == 1)
          {
            textel_shadow.ch = tokens[0][0];
            // "1, 2, 3" -> "rgb6:[1, 2, 3]
            if (str::count_substr(tokens[1], ", ") == 2)
              tokens[1] = "rgb6:[" + tokens[1] + "]";
            textel_shadow.fg_color.parse(tokens[1]);
            textel_shadow.bg_color.parse(tokens[2]);
            textel_shadow.mat = std::atoi(tokens[3].c_str());
          }
          else
            std::cerr << "Unable to parse shadow textel." << std::endl;
          part = 2;
        }
        else if (part == 2)
        {
          textel_presets.emplace_back(textel_normal, textel_shadow, line);
          custom_textel_presets.emplace_back(textel_normal, textel_shadow, line);
          part = 0;
        }
      }
    }
  }
  
public:
  Game(int argc, char** argv, const t8x::GameEngineParams& params)
    : GameEngine(argv[0], params)
    , message_handler(std::make_unique<t8x::MessageHandler>())
  {
    GameEngine::set_anim_rate(0, 5);
    GameEngine::set_anim_rate(1, 6);
    
    auto bin_folder = get_exe_folder();
#ifndef _WIN32
    const char* xcode_env = std::getenv("RUNNING_FROM_XCODE");
    if (xcode_env != nullptr)
      bin_folder = folder::join_path({ bin_folder, "../../../../../../../../Documents/xcode/TextUR/TextUR/bin" }); // #FIXME: Find a better solution!
#endif
    filepath_custom_textel_presets = folder::join_path({ bin_folder, "custom_textel_presets" });
  
    RC size;
  
    for (int a_idx = 1; a_idx < argc; ++a_idx)
    {
      if (std::strcmp(argv[a_idx], "--help") == 0)
        show_help();

      if (a_idx + 1 < argc && std::strcmp(argv[a_idx], "-f") == 0) // file
      {
        file_path_curr_texture = argv[a_idx + 1];
        file_path_bright_texture = file_path_curr_texture;
      }
      else if (a_idx + 2 < argc && std::strcmp(argv[a_idx], "-s") == 0) // size
      {
        file_mode = EditorFileMode::NEW_OR_OVERWRITE_FILE;
        
        std::istringstream iss(argv[a_idx + 1]);
        iss >> size.r;
        iss.str(argv[a_idx + 2]);
        iss.clear();
        iss >> size.c;
      }
      else if (a_idx + 1 < argc && std::strcmp(argv[a_idx], "-t") == 0) // trace
        file_path_tracing_texture = argv[a_idx + 1];
      else if (a_idx + 1 < argc && std::strcmp(argv[a_idx], "-c") == 0) // convert to new texture
      {
        file_path_curr_texture = argv[a_idx + 1];
        convert = true;
      }
    }
  
    if (file_path_curr_texture.empty())
    {
      std::cerr << "ERROR: You must supply a texture filename as a command line argument!" << std::endl;
      show_help();
      request_exit();
    }
      
    if (convert)
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      {
        std::cerr << "ERROR: You cannot use the size flag (-s) together with the conversion flag (-c)!" << std::endl;
        request_exit();
      }
      else if (file_path_bright_texture.empty())
      {
        std::cerr << "ERROR: When using the conversion flag (-c) you need to also specify the source file with the -f flag!" << std::endl;
        request_exit();
      }
    }
    else
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
        curr_texture = Texture { size };
      else
      {
        if (!curr_texture.load(file_path_curr_texture))
          exit(EXIT_FAILURE);
      }
      
      if (!file_path_tracing_texture.empty())
        tracing_texture.load(file_path_tracing_texture);
    }
  }
  
  virtual void generate_data() override
  {
    reload_textel_presets();
                                
    if (convert)
    {
      bright_texture.load(file_path_bright_texture); // source
      curr_texture = Texture { bright_texture.size }; // target
      for (int r = 0; r < bright_texture.size.r; ++r)
        for (int c = 0; c < bright_texture.size.c; ++c)
        {
          const auto& curr_textel = bright_texture(r, c);
          auto it = stlutils::find_if(textel_presets, [&curr_textel](const auto& tp)
          {
            return tp.textel_normal.ch == curr_textel.ch
            && tp.textel_normal.fg_color == curr_textel.fg_color
            && tp.textel_normal.bg_color == curr_textel.bg_color
            && tp.textel_normal.mat == curr_textel.mat;
          });
          if (it != textel_presets.end())
            curr_texture.set_textel(r, c, it->textel_shadow);
          else
            curr_texture.set_textel(r, c, curr_textel);
        }
      curr_texture.save(file_path_curr_texture);
      request_exit();
    }

    tbd.add(PARAM(screen_pos.r));
    tbd.add(PARAM(screen_pos.c));
    tbd.add(PARAM(cursor_pos.r));
    tbd.add(PARAM(cursor_pos.c));
    
    reset_goto_input();
    init_keys_legend();
    
    reset_textel_editor();
  }
  
private:
  void handle_editor_key_presses(char curr_key, t8::SpecialKey curr_special_key,
                                 int nri, int nci, t8::RC& cursor_pos)
  {
    if (curr_key == '-')
      math::toggle(show_menu);
      
    bool is_up = curr_special_key == t8::SpecialKey::Up || curr_key == 'w';
    bool is_down = curr_special_key == t8::SpecialKey::Down || curr_key == 's';
    bool is_left = curr_special_key == t8::SpecialKey::Left || curr_key == 'a';
    bool is_right = curr_special_key == t8::SpecialKey::Right || curr_key == 'd';
    if (show_menu)
    {
      if (is_up)
      {
        selected_textel_preset_idx--;
        if (selected_textel_preset_idx == -1)
        {
          selected_textel_preset_idx = static_cast<int>(textel_presets.size()) - 1;
          menu_r_offs = nri - 3*static_cast<int>(textel_presets.size());
        }
      }
      else if (is_down)
      {
        selected_textel_preset_idx++;
        if (selected_textel_preset_idx == static_cast<int>(textel_presets.size()))
        {
          selected_textel_preset_idx = 0;
          menu_r_offs = 0;
        }
      }
      else if (is_left)
      {
        int curr_mat = textel_presets[selected_textel_preset_idx].textel_normal.mat;
        for (int idx = selected_textel_preset_idx - 1; idx >= 0; --idx)
          if (textel_presets[idx].textel_normal.mat != curr_mat)
          {
            selected_textel_preset_idx = idx;
            menu_r_offs = -3*selected_textel_preset_idx;
            break;
          }
      }
      else if (is_right)
      {
        int curr_mat = textel_presets[selected_textel_preset_idx].textel_normal.mat;
        for (int idx = selected_textel_preset_idx + 1; idx < static_cast<int>(textel_presets.size()); ++idx)
          if (textel_presets[idx].textel_normal.mat != curr_mat)
          {
            selected_textel_preset_idx = idx;
            menu_r_offs = -3*selected_textel_preset_idx;
            break;
          }
      }
    }
    else
    {
      if (is_up)
      {
        cursor_pos.r--;
        if (cursor_pos.r < 0)
          cursor_pos.r = 0;
        if (cursor_pos.r + screen_pos.r < 0)
          screen_pos.r++;
      }
      else if (is_down)
      {
        cursor_pos.r++;
        if (cursor_pos.r >= static_cast<int>(curr_texture.size.r))
          cursor_pos.r = curr_texture.size.r - 1;
        if (cursor_pos.r + screen_pos.r >= nri)
          screen_pos.r--;
      }
      else if (is_left)
      {
        cursor_pos.c--;
        if (cursor_pos.c < 0)
          cursor_pos.c = 0;
        if (cursor_pos.c + screen_pos.c < 0)
          screen_pos.c++;
      }
      else if (is_right)
      {
        cursor_pos.c++;
        if (cursor_pos.c >= static_cast<int>(curr_texture.size.c))
          cursor_pos.c = curr_texture.size.c - 1;
        if (cursor_pos.c + screen_pos.c >= nci)
          screen_pos.c--;
      }
      else if (curr_key == 'W')
      {
        cursor_pos.r -= nri;
        if (cursor_pos.r < 0)
          cursor_pos.r = 0;
        while (cursor_pos.r + screen_pos.r < 0)
          screen_pos.r++;
      }
      else if (curr_key == 'S')
      {
        cursor_pos.r += nri;
        if (cursor_pos.r >= static_cast<int>(curr_texture.size.r))
          cursor_pos.r = curr_texture.size.r - 1;
        while (cursor_pos.r + screen_pos.r >= nri)
          screen_pos.r--;
      }
      else if (curr_key == 'A')
      {
        cursor_pos.c -= nci;
        if (cursor_pos.c < 0)
          cursor_pos.c = 0;
        while (cursor_pos.c + screen_pos.c < 0)
          screen_pos.c++;
      }
      else if (curr_key == 'D')
      {
        cursor_pos.c += nci;
        if (cursor_pos.c >= static_cast<int>(curr_texture.size.c))
          cursor_pos.c = curr_texture.size.c - 1;
        while (cursor_pos.c + screen_pos.c >= nci)
          screen_pos.c--;
      }
      else if (curr_key == ' ')
      {
        undo_buffer.push({ { cursor_pos, curr_texture(cursor_pos) } });
        curr_texture.set_textel(cursor_pos, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
        redo_buffer = {};
        is_modified = true;
      }
      else if (curr_key == 'z')
      {
        if (!undo_buffer.empty())
        {
          const auto& upv = undo_buffer.top();
          UndoItem item;
          item.reserve(upv.size());
          for (const auto& up : upv)
            item.emplace_back(up.first, curr_texture(up.first));
          redo_buffer.push(item);
          for (const auto& up : upv)
            curr_texture.set_textel(up.first, up.second);
          undo_buffer.pop();
          is_modified = true;
        }
      }
      else if (curr_key == 'Z')
      {
        if (!redo_buffer.empty())
        {
          const auto& upv = redo_buffer.top();
          UndoItem item;
          item.reserve(upv.size());
          for (const auto& up : upv)
            item.emplace_back(up.first, curr_texture(up.first));
          undo_buffer.push(item);
          for (const auto& up : upv)
            curr_texture.set_textel(up.first, up.second);
          redo_buffer.pop();
          is_modified = true;
        }
      }
      else if (curr_key == 'h')
        math::toggle(draw_horiz_coords);
      else if (curr_key == 'v')
        math::toggle(draw_vert_coords);
      else if (curr_key == 'H')
        math::toggle(draw_horiz_coord_line);
      else if (curr_key == 'V')
        math::toggle(draw_vert_coord_line);
      else if (str::to_lower(curr_key) == 'c')
      {
        undo_buffer.push({ { cursor_pos, curr_texture(cursor_pos) } });
        curr_texture.set_textel(cursor_pos, Textel {});
        redo_buffer = {};
        is_modified = true;
      }
      else if (curr_key == 'b' || curr_key == 'r')
      {
        UndoItem undo;
        for (int i = -2; i <= 2; ++i)
        {
          int j_offs = std::abs(i) == 2 ? 2 : 4;
          for (int j = -j_offs; j <= j_offs; ++j)
          {
            RC pos = cursor_pos + RC { i, j };
            auto dist = math::length(2.f*i, static_cast<float>(j));
            auto nrnd = rnd::randn(0.f, dist);
            auto anrnd = std::abs(nrnd);
            if (curr_key == 'b' || (curr_key == 'r' && anrnd < 0.1f))
            {
              undo.emplace_back(pos, curr_texture(pos));
              curr_texture.set_textel(pos, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
            }
          }
        }
        undo_buffer.push(undo);
        redo_buffer = {};
        is_modified = true;
      }
      else if (curr_key == 'B' || curr_key == 'R')
      {
        //                 ~~~~~
        //              ~~~~~~~~~~~
        //            ~~~~~~~~~~~~~~~
        //           ~~~~~~~~~~~~~~~~~
        //          ~~~~~~~~~~~~~~~~~~~
        //          ~~~~~~~~~*~~~~~~~~~
        //          ~~~~~~~~~~~~~~~~~~~
        //           ~~~~~~~~~~~~~~~~~
        //            ~~~~~~~~~~~~~~~
        //              ~~~~~~~~~~~
        //                 ~~~~~
        UndoItem undo;
        for (int i = -5; i <= 5; ++i)
        {
          int j_offs = 0;
          switch (std::abs(i))
          {
            case 0:
            case 1:
              j_offs = 9;
              break;
            case 2: j_offs = 8; break;
            case 3: j_offs = 7; break;
            case 4: j_offs = 5; break;
            case 5: j_offs = 2; break;
          }
          for (int j = -j_offs; j <= j_offs; ++j)
          {
            RC pos = cursor_pos + RC { i, j };
            auto dist = math::length(2.f*i, static_cast<float>(j));
            auto nrnd = rnd::randn(0.f, dist);
            auto anrnd = std::abs(nrnd);
            if (curr_key == 'B' || (curr_key == 'R' && anrnd < 0.1f))
            {
              undo.emplace_back(pos, curr_texture(pos));
              curr_texture.set_textel(pos, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
            }
          }
        }
        undo_buffer.push(undo);
        redo_buffer = {};
        is_modified = true;
      }
      else if (str::to_lower(curr_key) == 'f')
      {
        UndoItem undo;
        const auto& selected_textel = textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels);
        for (int i = 0; i < nri; ++i)
        {
          for (int j = 0; j < nci; ++j)
          {
            RC pos = RC { i, j } - screen_pos;
            undo.emplace_back(pos, curr_texture(pos));
            curr_texture.set_textel(pos, selected_textel);
          }
        }
        undo_buffer.push(undo);
        redo_buffer = {};
        is_modified = true;
      }
      else if (str::to_lower(curr_key) == 'p')
      {
        const auto& curr_textel = curr_texture(cursor_pos);
        auto idx_normal = stlutils::find_if_idx(textel_presets,
          [&curr_textel](const auto& tp) { return tp.get_textel(false) == curr_textel; });
        if (0 <= idx_normal)
        {
          selected_textel_preset_idx = idx_normal;
          menu_r_offs = -3*selected_textel_preset_idx;
        }
        else
        {
          auto idx_shadow = stlutils::find_if_idx(textel_presets,
          [&curr_textel](const auto& tp) { return tp.get_textel(true) == curr_textel; });
          if (0 <= idx_shadow)
          {
            selected_textel_preset_idx = idx_shadow;
            menu_r_offs = -3*selected_textel_preset_idx;
          }
        }
      }
      else if (str::to_lower(curr_key) == 'l')
        message_handler->add_message(static_cast<float>(get_real_time_s()),
                                     "Cursor @ " + cursor_pos.str(),
                                     t8x::MessageHandler::Level::Guide);
      else if (str::to_lower(curr_key) == 'g')
      {
        if (!math::toggle(show_goto_pos))
          reset_goto_input();
      }
      else if (str::to_lower(curr_key) == 'e')
      {
        if (!math::toggle(show_textel_editor))
          reset_textel_editor();
      }
      else if (str::to_lower(curr_key) == 't')
        math::toggle(show_tracing);
      else if (str::to_lower(curr_key) == 'm')
        math::toggle(show_materials);
    }
    
    if (str::to_lower(curr_key) == 'i')
        math::toggle(use_shadow_textels);
    else if (str::to_lower(curr_key) == 'x' || safe_to_save)
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      {
        if (folder::exists(file_path_curr_texture))
        {
          show_confirm_overwrite = true;
          overwrite_confirm_button = t8x::YesNoButtons::No;
        }
        else
          safe_to_save = true;
      }
      else
        safe_to_save = true;
        
      if (safe_to_save)
      {
        if (curr_texture.save(file_path_curr_texture))
        {
          message_handler->add_message(static_cast<float>(get_real_time_s()),
                                       "Your work was successfully saved.",
                                       t8x::MessageHandler::Level::Guide);
                                       
          is_modified = false;
        }
        else
          message_handler->add_message(static_cast<float>(get_real_time_s()),
                                       "An error occurred while saving your work!",
                                       t8x::MessageHandler::Level::Fatal);
                                       
        safe_to_save = false;
        show_confirm_overwrite = false;
      }
    }
    else if (str::to_lower(curr_key) == 'k')
      math::toggle(show_keys_legend);
  }

  virtual void update() override
  {
    t8::Style ui_style { Color16::LightGray, Color16::Black };
    
    int cursor_anim_ctr = get_anim_count(1) % 2 == 0;
    
    const int nr = sh.num_rows();
    const int nc = sh.num_cols();
    const int nri = sh.num_rows_inset();
    const int nci = sh.num_cols_inset();
    const int menu_width = 15;

//#define SHOW_DEBUG_WINDOW
#ifdef SHOW_DEBUG_WINDOW
    TextBoxDrawingArgsAlign tbd_args;
    tbd_args.v_align = VerticalAlignment::TOP;
    tbd_args.base.box_style = { Color16::Blue, Color16::Yellow };
    tbd_args.framed_mode = true;
    tbd.calc_pre_draw(str::Adjustment::Left);
    tbd.draw(sh, tbd_args);
#endif

    auto curr_key = get_char_key(kpdp.transient);
    auto curr_special_key = get_special_key(kpdp.transient);
    bool allow_editing = true;
      
    if (!show_confirm_overwrite && show_menu)
    {
      // Does not need to be qualified with t8x::drawing, but I'm not sure why.
      t8x::draw_box_outline(sh, 0, nc - menu_width, nr, menu_width, t8x::OutlineType::Line, ui_style);
    }
  
    if (is_modified)
      sh.write_buffer("*", 0, 0, Color16::Red, Color16::White);
    draw_frame(sh, Color16::White);
    
    message_handler->update(sh, static_cast<float>(get_real_time_s()));
    
    if (show_confirm_overwrite)
    {
      bg_color = Color16::DarkCyan;
      draw_confirm(sh, { "Are you sure you want to overwrite the file \"" + file_path_curr_texture + "\"?" },
                   overwrite_confirm_button,
                   { Color16::Black, Color16::DarkCyan },
                   { Color16::Black, Color16::DarkCyan, Color16::Cyan },
                   { Color16::White, Color16::DarkCyan });
      if (curr_special_key == t8::SpecialKey::Left)
        overwrite_confirm_button = t8x::YesNoButtons::Yes;
      else if (curr_special_key == t8::SpecialKey::Right)
        overwrite_confirm_button = t8x::YesNoButtons::No;
      
      if (curr_special_key == t8::SpecialKey::Enter)
      {
        if (overwrite_confirm_button == t8x::YesNoButtons::Yes)
          safe_to_save = true;
        else
          show_confirm_overwrite = false;
      }
    }
    else
    {
      if (show_menu)
        draw_menu(ui_style, menu_width);
      else if (show_goto_pos)
      {
        allow_editing = false;
        dialog_goto.update(curr_key, curr_special_key);
        if (curr_special_key == t8::SpecialKey::Enter)
        {
          if (dialog_goto.text_field_empty(0) || dialog_goto.text_field_empty(1))
          {
            message_handler->add_message(static_cast<float>(get_real_time_s()),
                                         "You must type both row and col coordinates.",
                                         t8x::MessageHandler::Level::Guide);
          }
          else
          {
            std::istringstream iss(dialog_goto.get_text_field_input(0));
            RC pos;
            iss >> pos.r;
            iss.str(dialog_goto.get_text_field_input(1));
            iss.clear();
            iss >> pos.c;
            if (math::in_range(pos.r, 0, curr_texture.size.r, Range::ClosedOpen)
                && math::in_range(pos.c, 0, curr_texture.size.c, Range::ClosedOpen))
            {
              cursor_pos = pos;
              screen_pos = { nr/2 - cursor_pos.r, nc/2 - cursor_pos.c };
            }
            reset_goto_input();
            show_goto_pos = false;
          }
        }
        else if (curr_special_key == t8::SpecialKey::Escape)
        {
          reset_goto_input();
          show_goto_pos = false;
        }

        t8x::TextBoxDrawingArgsAlign tb_args;
        tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
        tb_args.base.box_padding_lr = 1;
        dialog_goto.calc_pre_draw(str::Adjustment::Left);
        dialog_goto.draw(sh, tb_args, cursor_anim_ctr);
        
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
        tb_ui_help_goto.calc_pre_draw(str::Adjustment::Left);
        tb_ui_help_goto.draw(sh, tb_args);
      }
      else if (show_keys_legend)
      {
        allow_editing = false;
        dialog_keys.update(curr_key, curr_special_key);
        if (curr_special_key == t8::SpecialKey::Escape || str::to_lower(curr_key) == 'k')
          show_keys_legend = false;
          
        t8x::TextBoxDrawingArgsAlign tb_args;
        tb_args.v_align = t8x::VerticalAlignment::TOP;
        tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
        tb_args.base.box_padding_lr = 1;
        dialog_keys.calc_pre_draw(str::Adjustment::Left);
        dialog_keys.draw(sh, tb_args, cursor_anim_ctr);
        
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
        tb_ui_help_keys.calc_pre_draw(str::Adjustment::Left);
        tb_ui_help_keys.draw(sh, tb_args);
      }
      else if (show_textel_editor)
      {
        allow_editing = false;
        switch (edit_mode)
        {
          case EditTextelMode::EditOrAdd:
          {
            // +-----------------------------------+
            // | Edit or Add Custom Textel Preset? |
            // | [Edit]                      [Add] |
            // +-----------------------------------+
            dialog_edit_or_add.update(curr_key, curr_special_key);
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              auto sel_btn_text = dialog_edit_or_add.get_selected_button_text();
              if (sel_btn_text == "Edit")
                edit_mode = EditTextelMode::EditEnterMat;
              else if (sel_btn_text == "Add")
              {
                int last_valid_idx = stlutils::sizeI(custom_textel_presets) - 1;
                dialog_editor[2] = "Idx: " + std::to_string(last_valid_idx + 1);
                edit_mode = EditTextelMode::EditTextelNormal;
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
            tb_args.base.box_padding_lr = 1;
            dialog_edit_or_add.calc_pre_draw(str::Adjustment::Left);
            dialog_edit_or_add.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
            
          case EditTextelMode::EditEnterMat:
          {
            // +----------------------------------+
            // | Enter Custom Textel Preset Index |
            // | Idx: ____                        |
            // +----------------------------------+
            dialog_edit_mat.update(curr_key, curr_special_key);
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              if (dialog_edit_mat.text_field_empty(0))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must type a valid custom textel preset id.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else
              {
                std::istringstream iss(dialog_edit_mat.get_text_field_input(0));
                int ctp_idx;
                iss >> ctp_idx;
                if (stlutils::in_range(custom_textel_presets, ctp_idx))
                {
                  message_handler->add_message(static_cast<float>(get_real_time_s()),
                                               "Successfully loaded custom textel preset " + std::to_string(ctp_idx) + "!",
                                               t8x::MessageHandler::Level::Guide);
                  edit_textel_preset = &custom_textel_presets[ctp_idx];
                  edit_textel_normal = edit_textel_preset->textel_normal;
                  edit_textel_shadow = edit_textel_preset->textel_shadow;
                  edit_textel_name = edit_textel_preset->name;
                  dialog_editor[2] = "Idx: " + std::to_string(ctp_idx);
                  dialog_editor.set_text_field_input(0, edit_textel_name);
                  dialog_editor.set_text_field_input(1, std::string(1, edit_textel_normal.ch));
                  dialog_editor.set_color_picker_color(2, edit_textel_normal.fg_color);
                  dialog_editor.set_color_picker_color(3, edit_textel_normal.bg_color);
                  dialog_editor.set_text_field_input(4, std::to_string(edit_textel_normal.mat));
                  edit_mode = EditTextelMode::EditTextelNormal;
                }
                else
                {
                  int last_valid_idx = stlutils::sizeI(custom_textel_presets) - 1;
                  message_handler->add_message(static_cast<float>(get_real_time_s()),
                                               "Unable to find custom textel preset: " + std::to_string(ctp_idx) + "!\n" + (last_valid_idx == -1 ? "There are no custom textel presets to edit!" : "\nLast valid index is: " + std::to_string(last_valid_idx) + "."),
                                               t8x::MessageHandler::Level::Guide,
                                               3.f);
                  dialog_edit_mat.clear_text_field_input(0);
                }
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
            tb_args.base.box_padding_lr = 1;
            dialog_edit_mat.calc_pre_draw(str::Adjustment::Left);
            dialog_edit_mat.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
            
          case EditTextelMode::EditTextelNormal:
          {
            // +--------------------------------------+
            // | Custom Textel Preset Editor (Normal) |
            // | Textel:                              |
            // | Idx:                                 |
            // | Name: Magic Stone_____               |
            // | Char: _                              |
            // | FG Color:                            |
            // |    ________________                  |
            // | BG Color:                            |
            // |    ________________                  |
            // | Mat: ____                            |
            // +--------------------------------------+
            dialog_editor.update(curr_key, curr_special_key);
            edit_textel_name = dialog_editor.get_text_field_input(0);
            edit_textel_normal.ch = dialog_editor.text_field_empty(1) ? ' ' : dialog_editor.get_text_field_input(1)[0];
            edit_textel_normal.fg_color = dialog_editor.get_color_picker_color(2);
            edit_textel_normal.bg_color = dialog_editor.get_color_picker_color(3);
            dialog_editor.set_textel_pre({ 1, 8 }, edit_textel_normal.ch, edit_textel_normal.fg_color, edit_textel_normal.bg_color);
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              if (dialog_editor.text_field_empty(0))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel preset name.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else if (dialog_editor.text_field_empty(1))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel character.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else if (dialog_editor.text_field_empty(4))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel material.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else
              {
                edit_textel_normal.mat = std::stoi(dialog_editor.get_text_field_input(4));
                if (edit_textel_preset != nullptr)
                {
                  edit_textel_preset->name = edit_textel_name;
                  edit_textel_preset->textel_normal = edit_textel_normal;
                }
                dialog_editor.set_text_field_input(0, edit_textel_name);
                dialog_editor.set_text_field_input(1, std::string(1, edit_textel_shadow.ch));
                dialog_editor.set_color_picker_color(2, edit_textel_shadow.fg_color);
                dialog_editor.set_color_picker_color(3, edit_textel_shadow.bg_color);
                dialog_editor.set_text_field_input(4, std::to_string(edit_textel_normal.mat));
                dialog_editor[0] = "Custom Textel Preset Editor (Shadow)    ";
                dialog_editor[1] = "Textel:   ( )";
                dialog_editor.set_textel_pre({ 1, 11 }, edit_textel_normal.ch, edit_textel_normal.fg_color, edit_textel_normal.bg_color);
                dialog_editor.set_tab_order(0);
                edit_mode = EditTextelMode::EditTextelShadow;
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
            tb_args.base.box_padding_lr = 1;
            tb_args.v_align_offs = -2;
            dialog_editor.calc_pre_draw(str::Adjustment::Left);
            dialog_editor.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
            
          case EditTextelMode::EditTextelShadow:
          {
            dialog_editor.update(curr_key, curr_special_key);
            edit_textel_name = dialog_editor.get_text_field_input(0);
            edit_textel_shadow.ch = dialog_editor.text_field_empty(1) ? ' ' : dialog_editor.get_text_field_input(1)[0];
            edit_textel_shadow.fg_color = dialog_editor.get_color_picker_color(2);
            edit_textel_shadow.bg_color = dialog_editor.get_color_picker_color(3);
            dialog_editor.set_textel_pre({ 1, 8 }, edit_textel_shadow.ch, edit_textel_shadow.fg_color, edit_textel_shadow.bg_color);
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              if (dialog_editor.text_field_empty(0))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel preset name.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else if (dialog_editor.text_field_empty(1))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel character.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else if (dialog_editor.text_field_empty(4))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel material.",
                                             t8x::MessageHandler::Level::Guide);
              }
              else
              {
                edit_textel_shadow.mat = std::stoi(dialog_editor.get_text_field_input(4));
                if (edit_textel_preset != nullptr)
                {
                  edit_textel_preset->name = edit_textel_name;
                  edit_textel_preset->textel_shadow = edit_textel_shadow;
                }
                else
                  custom_textel_presets.emplace_back(edit_textel_normal, edit_textel_shadow, edit_textel_name);
                
                {
                  std::vector<std::string> lines_custom_textel_presets;
                  for (const auto& ctp : custom_textel_presets)
                  {
                    // '%', Magenta, Cyan, 28
                    // '%', DarkMagenta, DarkCyan, 28
                    // Magic Stone
                    lines_custom_textel_presets.emplace_back("'"s + ctp.textel_normal.ch + "', "
                      + ctp.textel_normal.fg_color.str() + ", "
                      + ctp.textel_normal.bg_color.str() + ", "
                      + std::to_string(ctp.textel_normal.mat));
                    lines_custom_textel_presets.emplace_back("'"s + ctp.textel_shadow.ch + "', "
                      + ctp.textel_shadow.fg_color.str() + ", "
                      + ctp.textel_shadow.bg_color.str() + ", "
                      + std::to_string(ctp.textel_shadow.mat));
                    lines_custom_textel_presets.emplace_back(ctp.name);
                  }
                  if (TextIO::write_file(filepath_custom_textel_presets, lines_custom_textel_presets))
                    message_handler->add_message(static_cast<float>(get_real_time_s()),
                                                 "Successfully wrote to custom textel presets file!",
                                                 t8x::MessageHandler::Level::Guide);
                  else
                    message_handler->add_message(static_cast<float>(get_real_time_s()),
                                                 "Unable to write to custom textel presets file!",
                                                 t8x::MessageHandler::Level::Fatal);
                                                 
                  reload_textel_presets();
                }
                reset_textel_editor();
                show_textel_editor = false;
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = { Color16::White, Color16::DarkBlue };
            tb_args.base.box_padding_lr = 1;
            tb_args.v_align_offs = -2;
            dialog_editor.calc_pre_draw(str::Adjustment::Left);
            dialog_editor.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
        }
        
        t8x::TextBoxDrawingArgsAlign tb_args;
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.base.box_padding_lr = 1;
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
        tb_ui_help_edit_textel[static_cast<int>(edit_mode)].calc_pre_draw(str::Adjustment::Left);
        tb_ui_help_edit_textel[static_cast<int>(edit_mode)].draw(sh, tb_args);
      }
      
      // Caret
      if (get_anim_count(0) % 2 == 0 && (!show_menu || screen_pos.c + cursor_pos.c + 1 < nc - menu_width))
        sh.write_buffer("#", screen_pos.r + cursor_pos.r + 1, screen_pos.c + cursor_pos.c + 1, ui_style);
      
      draw_coord_sys(draw_vert_coords, draw_horiz_coords, draw_vert_coord_line, draw_horiz_coord_line, nc, menu_width);
      
      int box_width_curr = curr_texture.size.c;
      int box_width_tracing = tracing_texture.size.c;
      if (show_menu)
      {
        if (curr_texture.size.c > nc - menu_width)
          box_width_curr = nc - menu_width - screen_pos.c;
        if (tracing_texture.size.c > nc - menu_width)
          box_width_tracing = nc - menu_width - screen_pos.c;
      }
      if (show_materials)
      {
        t8x::draw_box_texture_materials(sh,
                                   screen_pos.r, screen_pos.c,
                                   curr_texture.size.r + 2, box_width_curr + 2,
                                   curr_texture);
      }
      else
      {
        // Does not need to be qualified with t8x::drawing, but I'm not sure why.
        t8x::draw_box_textured(sh,
                          screen_pos.r, screen_pos.c,
                          curr_texture.size.r + 2, box_width_curr + 2,
                          t8x::SolarDirection::Zenith,
                          curr_texture);
      }
      if (show_tracing && !tracing_texture.empty())
      {
        // Does not need to be qualified with t8x::drawing, but I'm not sure why.
        t8x::draw_box_textured(sh,
                          screen_pos.r, screen_pos.c,
                          tracing_texture.size.r + 2, box_width_tracing + 2,
                          t8x::SolarDirection::Zenith,
                          tracing_texture);
      }
    }
                      
    if (allow_editing)
      handle_editor_key_presses(curr_key, curr_special_key, nri, nci, cursor_pos);
    
    GameEngine::enable_quit_confirm_screen(is_modified);
  }
  
  virtual void draw_title() override
  {
    //::draw_title(sh, font_data, color_schemes, text);
  }
  
  virtual void draw_instructions() override
  {
    //::draw_instructions(sh, 0);
  }
  
  std::string filepath_custom_textel_presets;
    
  t8::Texture curr_texture;
  t8::Texture tracing_texture;
  t8::Texture bright_texture;
  std::string file_path_curr_texture;
  std::string file_path_tracing_texture;
  std::string file_path_bright_texture;
  bool convert = false;
  EditorFileMode file_mode = EditorFileMode::OPEN_EXISTING_FILE;
  
  t8::RC screen_pos { 0, 0 };
  t8::RC cursor_pos { 0, 0 };
  int menu_r_offs = 0;
  
  bool show_menu = false;
  bool show_confirm_overwrite = false;
  bool show_tracing = true;
  bool show_goto_pos = false;
  bool show_keys_legend = false;
  bool show_textel_editor = false;
  bool show_materials = false;
  
  t8x::YesNoButtons overwrite_confirm_button = t8x::YesNoButtons::No;
  bool safe_to_save = false;
  
  std::vector<TextelItem> textel_presets; // Including custom textel presets.
  int selected_textel_preset_idx = 0;
  std::vector<TextelItem> custom_textel_presets;
  
  std::unique_ptr<t8x::MessageHandler> message_handler;
  using UndoItem = std::vector<std::pair<t8::RC, Textel>>;
  std::stack<UndoItem> undo_buffer;
  std::stack<UndoItem> redo_buffer;
  bool is_modified = false;
  
  bool draw_vert_coords = false;
  bool draw_horiz_coords = false;
  bool draw_vert_coord_line = false;
  bool draw_horiz_coord_line = false;
  
  bool use_shadow_textels = false;
  
  t8x::TextBoxDebug tbd;
  
  t8::ButtonStyle btn_style { Color16::White, Color16::DarkBlue, Color16::Blue };
  t8::PromptStyle tf_style { Color16::White, Color16::DarkBlue, Color16::White };
  t8x::ButtonFrame btn_frame = t8x::ButtonFrame::SquareBrackets;
  
  t8x::TextBox tb_ui_help_goto {{
      "UI Help"s,
      "Type coordinates using number keys.",
      "Erase characters by pressing [BACKSPACE].",
      "Toggle coordinate input by pressing [TAB].",
      "Press [ENTER] when done.",
      "Press [ESCAPE] to cancel."
    }};
  t8x::Dialog dialog_goto;
  t8x::TextField tf_goto_r { 8, t8x::TextFieldMode::Numeric, tf_style, 0 };
  t8x::TextField tf_goto_c { 8, t8x::TextFieldMode::Numeric, tf_style, 1 };
  
  t8x::TextBox tb_ui_help_keys {{
    "UI Help"s,
    "Press [ESCAPE] or [K] to close."
  }};
  t8x::Dialog dialog_keys;
  
  enum class EditTextelMode { EditOrAdd, EditEnterMat, EditTextelNormal, EditTextelShadow };
  std::array<t8x::TextBox, 4> tb_ui_help_edit_textel
  {
    {
      { { "UI Help"s, "Use arrow keys or [TAB] to select button,", "Then press [ENTER] when done.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Type in the textel preset idx using the number keys.", "First custom textel preset starts at idx = 0.", "Erase characters by pressing [BACKSPACE].", "Press [ENTER] when done.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Navigate widgets using [TAB].", "Erase characters with [BACKSPACE].", "Use arrow keys to select a color.", "Press [ENTER] when you're ready to edit the shadow textel.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Navigate widgets using [TAB].", "Erase characters with [BACKSPACE].", "Use arrow keys to select a color.", "Press [ENTER] when you're ready to save the textel preset.", "Press [ESCAPE] to cancel." } }
    }
  };
  EditTextelMode edit_mode = EditTextelMode::EditOrAdd;
  t8x::Dialog dialog_edit_or_add;
  t8x::Button btn_edit { "Edit", btn_style, btn_frame, 0 };
  t8x::Button btn_add { "Add", btn_style, btn_frame, 1 };
  t8x::Dialog dialog_edit_mat;
  t8x::TextField tf_textel_idx { 4, t8x::TextFieldMode::Numeric, tf_style, 0 };
  t8x::Dialog dialog_editor;
  t8x::TextField tf_textel_name = { 16, t8x::TextFieldMode::AlphaNumeric, tf_style, 0 };
  t8x::TextField tf_textel_symbol = { 1, t8x::TextFieldMode::All, tf_style, 1 };
  t8x::ColorPickerParams cp_params
  {
    t8x::ColorPickerCursorColoring::BlackWhite,
    t8x::ColorPickerCursorBlinking::EveryOther,
    Color16::Cyan, // special_color_fg_hilite_color
    true, // enable_special_colors
    true, // enable_4bit_colors
    true, // enable_rgb6_colors
    true  // enable_gray24_colors
  };
  t8x::ColorPicker cp_textel_fg = { Color16::Blue, Color16::White,
    cp_params,
    2, true, '*', ' ' };
  t8x::ColorPicker cp_textel_bg = { Color16::Blue, Color16::White,
    cp_params,
    3, true, '*', ' ' };
  t8x::TextField tf_textel_mat = { 4, t8x::TextFieldMode::Numeric, tf_style, 4 };
  TextelItem* edit_textel_preset = nullptr;
  Textel edit_textel_normal;
  Textel edit_textel_shadow;
  std::string edit_textel_name;
};

int main(int argc, char** argv)
{
  t8x::GameEngineParams params;
  params.enable_title_screen = false;
  params.enable_instructions_screen = false;
  params.enable_quit_confirm_screen = false;
  params.quit_confirm_unsaved_changes = true;
  params.enable_hiscores = false;
  params.enable_pause = false;
  params.screen_bg_color_default = Color16::Black;
  params.screen_bg_color_title = Color16::DarkYellow;
  params.screen_bg_color_instructions = Color16::Black;
  
  for (int a_idx = 1; a_idx < argc; ++a_idx)
  {
    if (strcmp(argv[a_idx],  "--suppress_tty_output") == 0)
      params.suppress_tty_output = true;
    else if (strcmp(argv[a_idx], "--suppress_tty_input") == 0)
      params.suppress_tty_input = true;
    else if (a_idx + 1 < argc && strcmp(argv[a_idx], "--log_mode") == 0)
    {
      if (strcmp(argv[a_idx + 1], "record") == 0)
        params.log_mode = LogMode::Record;
      else if (strcmp(argv[a_idx + 1], "replay") == 0)
        params.log_mode = LogMode::Replay;
      params.xcode_log_path = "../../../../../../../../Documents/xcode/TextUR/TextUR/bin/";
    }
  }

  Game game(argc, argv, params);

  game.init();
  game.generate_data();
  game.run();

  return EXIT_SUCCESS;
}
