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

using CharT = char32_t; // char or char32_t.

class Game : public t8x::GameEngine<44, 92, CharT>
{
  template<typename CharT>
  static std::vector<t8::StyledString> format_long_glyph_disp_sstr(const Textel& textel,
    const t8::Style& dlg_style, bool uncanonicalize_fallback = true)
  {
    t8::Style style = { textel.fg_color, textel.bg_color };
      return textel.glyph.format_long<CharT>(
        textel.glyph.preferred != t8::Glyph::none32, uncanonicalize_fallback,
        style, style, dlg_style);
  }

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
    
    std::vector<t8::StyledString> disp_glyph_normal;
    std::vector<t8::StyledString> disp_glyph_shadow;
    
    template<typename CharT>
    void update_disp_strings(const t8::Style& dlg_style, bool uncanonicalize_fallback)
    {
      disp_glyph_normal = format_long_glyph_disp_sstr<CharT>(textel_normal, dlg_style, uncanonicalize_fallback);
      disp_glyph_shadow = format_long_glyph_disp_sstr<CharT>(textel_shadow, dlg_style, uncanonicalize_fallback);
    }
    
    Textel get_textel(bool shadow) const
    {
      return shadow ? textel_shadow : textel_normal;
    }
    
    const std::vector<t8::StyledString>& get_glyph_disp_sstr(bool shadow) const
    {
      return shadow ? disp_glyph_shadow : disp_glyph_normal;
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
    std::cout << "   [--force_8bit_colors_on_win_cmd]" << std::endl;
    std::cout << "   [--edit_textel_presets_as_ascii_only]" << std::endl;
    std::cout << "   [--save_textures_as_ascii_only]" << std::endl;
    std::cout << "   [--display_ascii_only]" << std::endl;
    std::cout << "   [--set_big_brush_aspect_ratio <bar>]" << std::endl;
    std::cout << "   [--set_big_brush_radius <br>]" << std::endl;
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
    std::cout << "  <bar>                      : Aspect ratio for big brushes. Default value = 1.84." << std::endl;
    std::cout << "  <br>                       : Radius for big brushes. Default value = 10.5." << std::endl;
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
      //const auto& textel = preset.get_textel(use_shadow_textels);
      auto disp_glyph = preset.get_glyph_disp_sstr(use_shadow_textels);
      auto fg_color_bracket = p_idx == selected_textel_preset_idx ? Color16::LightGray : Color16::DarkGray;
      auto N_dg = disp_glyph.size();
      if (N_dg == 5)
      {
        disp_glyph[0].style.fg_color = fg_color_bracket;
        disp_glyph[2].style.fg_color = fg_color_bracket;
        disp_glyph[4].style.fg_color = fg_color_bracket;
      }
      else if (N_dg == 4)
      {
        disp_glyph[0].style.fg_color = fg_color_bracket;
        disp_glyph[2].style.fg_color = fg_color_bracket;
        disp_glyph[3].style.fg_color = fg_color_bracket;
      }
      sh.write_buffer(disp_glyph, r + 1, nc - menu_width + 2);
      sh.write_buffer(preset.name, r + 2, nc - menu_width + 2, name_style);
      // Does not need to be qualified with t8x::drawing, but I'm not sure why.
      t8x::draw_box_outline(sh, r, nc - menu_width, 4, menu_width, t8x::OutlineType::Unicode_SingleLine, ui_style);
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
      sh.write_buffer(str::rep_str(t8::GlyphString { t8::Glyph { 0x2500, '-' } }, screen_pos.c + cursor_pos.c), screen_pos.r + cursor_pos.r + 1, 1, Color16::Red, Color16::Transparent2);

    
    if (draw_v_cursor_line)
      for (int r = 0; r < cursor_pos.r + screen_pos.r; ++r)
        sh.write_buffer(t8::Glyph { 0x2502, '|' }, r + 1, screen_pos.c + cursor_pos.c + 1, Color16::Green, Color16::Transparent2);
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
      "F : fill screen with selected preset inside current bounding box of screen.",
      "P : pick a textel from cursor and hilite the matching preset in the menu.",
      "L : show location of cursor.",
      "G : goto new cursor location.",
      "T : toggle show/hide of tracing texture.",
      "I : toggle between dark and bright textel preset modes.",
      "M : toggle show/hide of material id:s.",
      "SHIFT + E : edit existing or add new custom textel preset.",
      "E : edit Ad Hoc textel preset (the first in the list). Mat = -1.",
      "Q : quit."
    });
    t8::Color fg_key = Color16::Cyan; //{ 4, 3, 2 };
    t8::Color bg_key = Color16::Transparent2;
    dialog_keys.set_textel_pre({ 2, 0 }, 'K', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 3, 0 }, "Esc", fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 4, 0 }, "WASD", fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 4, 8 }, "arrow keys", fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 6, 0 }, "SHIFT + WASD", fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 7, 0 }, "Space", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 8, 0 }, 'Z', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 9, 0 }, "SHIFT + Z", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 10, 0 }, 'C', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 11, 0 }, 'V', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 12, 0 }, 'H', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 13, 0 }, "SHIFT + V", fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 14, 0 }, "SHIFT + H", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 15, 0 }, '-', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 16, 0 }, 'X', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 17, 0 }, 'B', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 18, 0 }, "SHIFT + B", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 19, 0 }, 'R', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 22, 0 }, "SHIFT + R", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 25, 0 }, 'F', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 26, 0 }, 'P', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 27, 0 }, 'L', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 28, 0 }, 'G', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 29, 0 }, 'T', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 30, 0 }, 'I', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 31, 0 }, 'M', fg_key, bg_key);
    dialog_keys.set_textel_str_pre({ 32, 0 }, "SHIFT + E", fg_key, bg_key);
    dialog_keys.set_textel_pre({ 33, 0 }, 'E', fg_key, bg_key);
    dialog_keys.set_textel_pre({ 34, 0 }, 'Q', fg_key, bg_key);
    dialog_keys.set_tab_selection(0);
  }
  
  void reset_goto_input()
  {
    dialog_goto = t8x::Dialog({ "Cursor Goto @"s, str::rep_char(' ', 8) + ", " + str::rep_char(' ', 8) });
    dialog_goto.emplace_text_field({ 1, 0 }, 8, t8x::TextFieldMode::Numeric, tf_style, 0);
    dialog_goto.emplace_text_field({ 1, 10 }, 8, t8x::TextFieldMode::Numeric, tf_style, 1);
    dialog_goto.set_tab_selection(0);
  }
  
  void reset_textel_editor(bool init = false)
  {
    edit_mode = EditTextelMode::EditOrAdd;
    edit_or_add = EditOrAdd::None;
    if (init)
    {
      dialog_edit_or_add = t8x::Dialog {{ "Edit or Add Custom Textel Preset?"s }};
      dialog_edit_or_add.emplace_button("Edit", btn_style, btn_frame, 0);
      dialog_edit_or_add.emplace_button("Add", btn_style, btn_frame, 1);
    }
    dialog_edit_or_add.set_button_selection(0, true);
    
    if (init)
    {
      dialog_edit_mat = t8x::Dialog({ "Enter Custom Textel Preset Index"s, "Idx:" });
      dialog_edit_mat.emplace_text_field({ 1, 5 }, 4, t8x::TextFieldMode::Numeric, tf_style, 0);
    }
    else
    {
      dialog_edit_mat.clear_text_field(0);
    }
    dialog_edit_mat.set_tab_selection(0);
    
    std::string dialog_editor_title = "Custom Textel Preset Editor (Normal)";
    if (init)
    {
      std::vector<std::string> rows = { dialog_editor_title };
      for (int i = 1; i < 3; ++i) // Required for later indexing.
        rows.emplace_back("");
      dialog_editor = t8x::Dialog(rows);
      int v_offs = 1;
      dialog_editor.emplace_label({ v_offs++, 0 }, "Textel:", dlg_style);
      dialog_editor.emplace_label({ v_offs++, 0 }, "Idx:", dlg_style);
      dialog_editor.emplace_label({ v_offs, 0 }, "Name:", dlg_style);
      dialog_editor.emplace_text_field({ v_offs++, 6 }, 16, t8x::TextFieldMode::PrintableAscii, tf_style, 0);
      if (edit_textel_presets_as_ascii_only)
      {
        dialog_editor.emplace_label({ v_offs, 0 }, "Char:", dlg_style);
        auto& tf_textel_symbol = dialog_editor.emplace_text_field({ v_offs, 6 }, 1, t8x::TextFieldMode::All, tf_style, 1);
        v_offs += tf_textel_symbol.height();
      }
      else
      {
        dialog_editor.emplace_label({ v_offs, 0 }, "Glyph:", dlg_style);
        gp_textel_symbol = &dialog_editor.emplace_glyph_picker({ ++v_offs, 3 },
                                                               tf_style, dlg_style,
                                                               { Color16::Cyan, Color16::Transparent2 },
                                                               { Color16::DarkCyan, Color16::Transparent2 },
                                                               Color16::DarkYellow, 1);
        v_offs += gp_textel_symbol->height();
      }
      dialog_editor.emplace_label({ v_offs++, 0 }, "FG Color:", dlg_style);
      const auto& cp_textel_fg = dialog_editor.emplace_color_picker({ v_offs, 3 },
                                                                    Color16::Blue, Color16::White,
                                                                    cp_params,
                                                                    2, true, '*', ' ' );
      v_offs += cp_textel_fg.height();
      dialog_editor.emplace_label({ v_offs++, 0 }, "BG Color:", dlg_style);
      const auto& cp_textel_bg = dialog_editor.emplace_color_picker({ v_offs, 3 },
                                                                    Color16::Blue, Color16::White,
                                                                    cp_params,
                                                                    3, true, '*', ' ');
      v_offs += cp_textel_bg.height();
      dialog_editor.emplace_label({ v_offs, 0 }, "Mat:", dlg_style);
      dialog_editor.emplace_text_field({ v_offs++, 5 }, 4, t8x::TextFieldMode::Numeric, tf_style, 4);
    }
    else
    {
      //dialog_editor.clear_selections();
      dialog_editor[0] = dialog_editor_title;
      dialog_editor[1] = str::rep_char(' ', 13);
      
      dialog_editor.clear_all_sstr_vec_pre();
      dialog_editor.clear_all_textel_pre();
      
      dialog_editor.clear_text_field(0);
      
      dialog_editor.clear_text_field(1);
      dialog_editor.clear_glyph_picker(1);
      dialog_editor.clear_color_picker(2);
      dialog_editor.clear_color_picker(3);
      dialog_editor.clear_text_field(4);
    }
    dialog_editor.set_tab_selection(0);
  }
  
  void reset_adhoc_textel_editor(bool init = false)
  {
    if (init)
    {
      std::vector<std::string> rows = { "Ad Hoc Textel Preset Editor" };
      dialog_editor_adhoc = t8x::Dialog(rows);
      int v_offs = 1;
      dialog_editor_adhoc.emplace_label({ v_offs++, 0 }, "Textel:", dlg_style);
      if (edit_textel_presets_as_ascii_only)
      {
        dialog_editor_adhoc.emplace_label({ v_offs, 0 }, "Char:", dlg_style);
        auto& tf_textel_symbol_adhoc = dialog_editor_adhoc.emplace_text_field({ v_offs, 6 }, 1, t8x::TextFieldMode::All, tf_style, 0);
        v_offs += tf_textel_symbol_adhoc.height();
      }
      else
      {
        dialog_editor_adhoc.emplace_label({ v_offs, 0 }, "Glyph:", dlg_style);
        gp_textel_symbol_adhoc = &dialog_editor_adhoc.emplace_glyph_picker({ ++v_offs, 3 },
                                                                           tf_style, dlg_style,
                                                                           { Color16::Cyan, Color16::Transparent2 },
                                                                           { Color16::DarkCyan, Color16::Transparent2 },
                                                                           Color16::DarkYellow, 0);
        v_offs += gp_textel_symbol_adhoc->height();
      }
      dialog_editor_adhoc.emplace_label({ v_offs++, 0 }, "FG Color:", dlg_style);
      const auto& cp_textel_fg_adhoc = dialog_editor_adhoc.emplace_color_picker({ v_offs, 3 },
                                                                                Color16::Blue, Color16::White,
                                                                                cp_params,
                                                                                1, true, '*', ' ');
      v_offs += cp_textel_fg_adhoc.height();
      dialog_editor_adhoc.emplace_label({ v_offs++, 0 }, "BG Color:", dlg_style);
      dialog_editor_adhoc.emplace_color_picker({ v_offs, 3 },
                                               Color16::Blue, Color16::White,
                                               cp_params,
                                               2, true, '*', ' ');
    }
    else
    {
      dialog_editor_adhoc.clear_text_field(0);
      dialog_editor_adhoc.clear_glyph_picker(0);
      dialog_editor_adhoc.clear_color_picker(1);
      dialog_editor_adhoc.clear_color_picker(2);
    }
    dialog_editor_adhoc.set_tab_selection(0);
  }
  
  void reload_textel_presets()
  {
    textel_presets.clear();
    custom_textel_presets.clear();
    textel_presets.emplace_back(Textel { { }, Color16::Transparent2, Color16::Transparent2, t8::texture::mat_none },
                                Textel { { }, Color16::Transparent2, Color16::Transparent2, t8::texture::mat_none },
                                "Ad Hoc [e]");
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
    textel_presets.emplace_back(Textel { { 0x2506, '|' }, Color16::DarkGreen, Color16::Green, 7 },
                                Textel { { 0x2506, '|' }, Color16::Green, Color16::DarkGreen, 7 },
                                "Grass0u");
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
      auto post_process_color_tokens = [](std::vector<std::string>& tokens)
      {
        // "1, 2, 3" -> "rgb6:[1, 2, 3]
        for (int tok_idx = 1; tok_idx <= 2; ++tok_idx)
          if (str::count_substr(tokens[tok_idx], ", ") == 2)
            tokens[tok_idx] = "rgb6:[" + tokens[tok_idx] + "]";
      };
    
      int part = 0;
      Textel textel_normal, textel_shadow;
      for (const auto& line : lines_custom_textel_presets)
      {
        if (line.empty())
          continue;
        if (part == 0)
        {
          auto tokens = str::tokenize(line, { ' ', ',' }, { '\'', '[', ']' });
          if (tokens.size() == 4)
          {
            auto glyph_tokens = str::tokenize(tokens[0], { '\'' });
            if (glyph_tokens.size() == 1)
            {
              if (glyph_tokens[0].length() == 1)
                textel_normal.glyph.parse(tokens[0], true); // Presumably ASCII character.
              else
                textel_normal.glyph.parse("[" + tokens[0] + "]", false); // Presumably Unicode + ASCII character.
            }
            else
              std::cerr << "ERROR in reload_textel_presets() : Unrecognized glyph token.\n";
            post_process_color_tokens(tokens);
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
          if (tokens.size() == 4)
          {
            auto glyph_tokens = str::tokenize(tokens[0], { '\'' });
            if (glyph_tokens.size() == 1)
            {
              if (glyph_tokens[0].length() == 1)
                textel_shadow.glyph.parse(tokens[0], true); // Presumably ASCII character.
              else
                textel_shadow.glyph.parse("[" + tokens[0] + "]", false); // Presumably Unicode + ASCII character.
            }
            else
              std::cerr << "ERROR in reload_textel_presets() : Unrecognized glyph token.\n";
            post_process_color_tokens(tokens);
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
    
    for (auto& tp : textel_presets)
      tp.update_disp_strings<CharT>(t8::Style { Color16::DarkGray, Color16::Transparent2 }, true);
  }
  
public:
  Game(int argc, char** argv, const t8x::GameEngineParams& params)
    : GameEngine(argv[0], params)
    , message_handler(std::make_unique<t8x::MessageHandler<std::string>>())
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
      else if (std::strcmp(argv[a_idx], "--force_8bit_colors_on_win_cmd") == 0)
        force_8bit_colors_on_win_cmd = true;
      else if (std::strcmp(argv[a_idx], "--edit_textel_presets_as_ascii_only") == 0)
        edit_textel_presets_as_ascii_only = true;
      else if (std::strcmp(argv[a_idx], "--save_textures_as_ascii_only") == 0)
        save_textures_as_ascii_only = true;
      else if (a_idx + 1 < argc && std::strcmp(argv[a_idx], "--set_big_brush_aspect_ratio") == 0)
        big_brush_aspect_ratio = std::stof(argv[a_idx + 1]);
      else if (a_idx + 1 < argc && std::strcmp(argv[a_idx], "--set_big_brush_radius") == 0)
        big_brush_radius = std::stof(argv[a_idx + 1]);
    }
    
    cp_params = {
      t8x::ColorPickerCursorColoring::BlackWhite,
      t8x::ColorPickerCursorBlinking::EveryOther,
      Color16::Cyan, // special_color_fg_hilite_color
      true, // enable_special_colors
      true, // enable_4bit_colors
      force_8bit_colors_on_win_cmd || !sys::is_non_wt_console(), // enable_rgb6_colors
      force_8bit_colors_on_win_cmd || !sys::is_non_wt_console()  // enable_gray24_colors
    };
    
    msg_box_drawing_args.outline_type = t8x::OutlineType::Unicode_SingleLine;
    
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
        {
          std::cerr << "ERROR: Unable to parse texture file." << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      
      if (!file_path_tracing_texture.empty())
        if (!tracing_texture.load(file_path_tracing_texture))
        {
          std::cerr << "ERROR: Unable to parse texture file." << std::endl;
          exit(EXIT_FAILURE);
        }
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
            return tp.textel_normal.glyph == curr_textel.glyph
            && tp.textel_normal.fg_color == curr_textel.fg_color
            && tp.textel_normal.bg_color == curr_textel.bg_color
            && tp.textel_normal.mat == curr_textel.mat;
          });
          if (it != textel_presets.end())
            curr_texture.set_textel(r, c, it->textel_shadow);
          else
            curr_texture.set_textel(r, c, curr_textel);
        }
      curr_texture.save(file_path_curr_texture,
                        save_textures_as_ascii_only ?
                          t8::TxGlyphEncoding::AsciiOnly :
                          t8::TxGlyphEncoding::TryUnicodePreferredAndFallbackElseAsciiOnly);
      request_exit();
    }

    tbd.add(PARAM(screen_pos.r));
    tbd.add(PARAM(screen_pos.c));
    tbd.add(PARAM(cursor_pos.r));
    tbd.add(PARAM(cursor_pos.c));
    
    reset_goto_input();
    init_keys_legend();
    
    reset_textel_editor(true);
    reset_adhoc_textel_editor(true);
  }
  
private:
  std::string fallback_to_text_field_input(char fb) const
  {
    return fb == t8::Glyph::none ? "" : std::string(1, fb);
  }
  
  // Sets the cursor and centers the view.
  void set_cursor(const RC& pos, int nri, int nci)
  {
    cursor_pos = pos;
  
    screen_pos = { nri/2 - cursor_pos.r, nci/2 - cursor_pos.c };
    
    const int min_r = std::min(0, nri - static_cast<int>(curr_texture.size.r));
    const int min_c = std::min(0, nci - static_cast<int>(curr_texture.size.c));
    screen_pos.r = math::clamp(screen_pos.r, min_r, 0);
    screen_pos.c = math::clamp(screen_pos.c, min_c, 0);
  }

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
          for (int j = -j_offs; j <= j_offs + 1; ++j)
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
        UndoItem undo;
        auto positions = t8x::filled_circle_positions(cursor_pos, big_brush_radius, big_brush_aspect_ratio);
        for (const auto& p : positions)
        {
          float anrnd = 0.f;
          if (curr_key == 'R')
          {
            int i = p.r - cursor_pos.r;
            int j = p.c - cursor_pos.c;
            auto dist = math::length(2.f*i, static_cast<float>(j));
            auto nrnd = rnd::randn(0.f, dist);
            anrnd = std::abs(nrnd);
          }
          if (anrnd < 0.1f)
          {
            undo.emplace_back(p, curr_texture(p));
            curr_texture.set_textel(p, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
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
          else
          {
            selected_textel_preset_idx = 0;
            textel_presets[0].textel_normal.glyph = curr_textel.glyph;
            textel_presets[0].textel_normal.fg_color = curr_textel.fg_color;
            textel_presets[0].textel_normal.bg_color = curr_textel.bg_color;
            textel_presets[0].textel_shadow = textel_presets[0].textel_normal;
            textel_presets[0].update_disp_strings<CharT>({ Color16::DarkGray, Color16::Transparent2 }, false);
            // Make sure to propagate the picked textel to the Ad Hoc editor.
            reset_adhoc_textel_editor();
          }
        }
      }
      else if (str::to_lower(curr_key) == 'l')
        message_handler->add_message(static_cast<float>(get_real_time_s()),
                                     "Cursor @ " + cursor_pos.str(),
                                     t8x::MessageHandlerLevel::Guide);
      else if (str::to_lower(curr_key) == 'g')
      {
        if (!math::toggle(show_goto_pos))
          reset_goto_input();
      }
      else if (curr_key == 'E')
      {
        if (!math::toggle(show_textel_editor))
          reset_textel_editor();
      }
      else if (curr_key == 'e')
      {
        if (!math::toggle(show_adhoc_textel_editor))
          reset_adhoc_textel_editor();
        else
        {
          edit_textel_preset_adhoc = &textel_presets[0];
          edit_textel_normal = edit_textel_preset_adhoc->textel_normal;
          if (edit_textel_presets_as_ascii_only)
          {
            const auto fb = edit_textel_normal.glyph.fallback;
            dialog_editor_adhoc.set_text_field_input(0, fallback_to_text_field_input(fb));
          }
          else
            dialog_editor_adhoc.set_glyph_picker_glyph(0, edit_textel_normal.glyph);
          dialog_editor_adhoc.set_color_picker_color(1, edit_textel_normal.fg_color);
          dialog_editor_adhoc.set_color_picker_color(2, edit_textel_normal.bg_color);
        }
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
        if (curr_texture.save(file_path_curr_texture,
                              save_textures_as_ascii_only ?
                                t8::TxGlyphEncoding::AsciiOnly :
                                t8::TxGlyphEncoding::TryUnicodePreferredAndFallbackElseAsciiOnly))
        {
          message_handler->add_message(static_cast<float>(get_real_time_s()),
                                       "Your work was successfully saved.",
                                       t8x::MessageHandlerLevel::Guide);
                                       
          is_modified = false;
        }
        else
          message_handler->add_message(static_cast<float>(get_real_time_s()),
                                       "An error occurred while saving your work!",
                                       t8x::MessageHandlerLevel::Fatal);
                                       
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
    t8x::TextBoxDrawingArgsAlign tbd_args;
    tbd_args.v_align = t8x::VerticalAlignment::TOP;
    tbd_args.base.box_style = { Color16::Blue, Color16::Yellow };
    tbd_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
    tbd_args.framed_mode = true;
    tbd.draw(sh, tbd_args);
#endif

    auto curr_key = get_char_key(kpdp.transient);
    auto curr_special_key = get_special_key(kpdp.transient);
    bool allow_editing = true;
      
    if (!show_confirm_overwrite && show_menu)
    {
      // Does not need to be qualified with t8x::drawing, but I'm not sure why.
      t8x::draw_box_outline(sh, 0, nc - menu_width, nr, menu_width, t8x::OutlineType::Unicode_SingleLine, ui_style);
    }
  
    if (is_modified)
      sh.write_buffer("*", 0, 0, Color16::Red, Color16::White);
    draw_frame(sh, Color16::White);
    
    message_handler->update(sh, static_cast<float>(get_real_time_s()), msg_box_drawing_args);
    
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
                                         t8x::MessageHandlerLevel::Guide);
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
              set_cursor(pos, nri, nci);
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
        tb_args.base.box_style = dlg_style;
        tb_args.base.box_padding_lr = 1;
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        dialog_goto.draw(sh, tb_args, cursor_anim_ctr);
        
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
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
        tb_args.base.box_style = dlg_style;
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.base.box_padding_lr = 1;
        dialog_keys.draw(sh, tb_args, cursor_anim_ctr);
        
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
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
              {
                edit_mode = EditTextelMode::EditEnterMat;
                edit_or_add = EditOrAdd::Edit;
              }
              else if (sel_btn_text == "Add")
              {
                int last_valid_idx = stlutils::sizeI(custom_textel_presets) - 1;
                dialog_editor[2] = "Idx: " + std::to_string(last_valid_idx + 1);
                edit_mode = EditTextelMode::EditTextelNormal;
                edit_or_add = EditOrAdd::Add;
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = dlg_style;
            tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
            tb_args.base.box_padding_lr = 1;
            dialog_edit_or_add.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
            
          // #FIXME: Rename to EditEnterCustomPresetIdx
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
                                             t8x::MessageHandlerLevel::Guide);
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
                                               t8x::MessageHandlerLevel::Guide);
                  edit_textel_preset = &custom_textel_presets[ctp_idx];
                  edit_textel_normal = edit_textel_preset->textel_normal;
                  edit_textel_shadow = edit_textel_preset->textel_shadow;
                  edit_textel_name = edit_textel_preset->name;
                  dialog_editor[2] = "Idx: " + std::to_string(ctp_idx);
                  dialog_editor.set_text_field_input(0, edit_textel_name);
                  if (edit_textel_presets_as_ascii_only)
                  {
                    const auto fb = edit_textel_normal.glyph.fallback;
                    dialog_editor.set_text_field_input(1, fallback_to_text_field_input(fb));
                  }
                  else
                    dialog_editor.set_glyph_picker_glyph(1, edit_textel_normal.glyph);
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
                                               t8x::MessageHandlerLevel::Guide,
                                               3.f);
                  dialog_edit_mat.clear_text_field(0);
                }
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = dlg_style;
            tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
            tb_args.base.box_padding_lr = 1;
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
            if (edit_textel_presets_as_ascii_only)
              edit_textel_normal.glyph = dialog_editor.text_field_empty(1) ? ' ' : dialog_editor.get_text_field_input(1)[0];
            else
              edit_textel_normal.glyph = dialog_editor.get_glyph_picker_glyph(1);
            edit_textel_normal.fg_color = dialog_editor.get_color_picker_color(2);
            edit_textel_normal.bg_color = dialog_editor.get_color_picker_color(3);
            //edit_textel_preset->textel_normal = edit_textel_normal;
            if (edit_textel_presets_as_ascii_only)
              dialog_editor.set_textel_pre({ 1, 8 }, edit_textel_normal.glyph, edit_textel_normal.fg_color, edit_textel_normal.bg_color);
            else
            {
              //edit_textel_preset->update_disp_strings<CharT>({ Color16::DarkCyan, Color16::Transparent2 }, false);
              //dialog_editor.set_sstr_vec_pre({ 1, 8 }, edit_textel_preset->get_glyph_disp_sstr(false));
              auto disp_glyph_normal = format_long_glyph_disp_sstr<CharT>(edit_textel_normal, { Color16::DarkCyan, Color16::Transparent2 }, false);
              dialog_editor.set_sstr_vec_pre({ 1, 8 }, disp_glyph_normal);
            }
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              if (dialog_editor.text_field_empty(0))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel preset name.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if ((edit_textel_presets_as_ascii_only && dialog_editor.text_field_empty(1))
                    || (!edit_textel_presets_as_ascii_only && edit_textel_normal.glyph.fully_empty()))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel " + (edit_textel_presets_as_ascii_only ? "character"s : "glyph"s) + ".",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if (!edit_textel_presets_as_ascii_only && !dialog_editor.glyph_picker_valid(1))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "Unicode textel glyphs need an ASCII fallback.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if (dialog_editor.text_field_empty(4))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel material.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else
              {
                edit_textel_normal.glyph.try_canonicalize_from_fallback();
                edit_textel_normal.mat = std::stoi(dialog_editor.get_text_field_input(4));
                if (edit_or_add == EditOrAdd::Edit && edit_textel_preset != nullptr)
                {
                  edit_textel_preset->name = edit_textel_name;
                  edit_textel_preset->textel_normal = edit_textel_normal;
                  
                  // Fill out dialog widgets with shadow textel preset data.
                  if (edit_textel_presets_as_ascii_only)
                  {
                    const auto fb = edit_textel_shadow.glyph.fallback;
                    dialog_editor.set_text_field_input(1, fallback_to_text_field_input(fb));
                  }
                  else
                    dialog_editor.set_glyph_picker_glyph(1, edit_textel_shadow.glyph);
                  dialog_editor.set_color_picker_color(2, edit_textel_shadow.fg_color);
                  dialog_editor.set_color_picker_color(3, edit_textel_shadow.bg_color);
                  dialog_editor.set_text_field_input(4, std::to_string(edit_textel_shadow.mat));
                }
                dialog_editor[0] = "Custom Textel Preset Editor (Shadow)    ";
                if (edit_textel_presets_as_ascii_only)
                {
                  dialog_editor[1] = "          ( )";
                  dialog_editor.set_textel_pre({ 1, 11 }, edit_textel_normal.glyph, edit_textel_normal.fg_color, edit_textel_normal.bg_color);
                }
                else
                {
                  const int c_max_glyph_disp_width = 12;
                  //const auto& normal_disp_sstr_vec = edit_textel_preset->get_glyph_disp_sstr(false);
                  auto disp_glyph_normal = format_long_glyph_disp_sstr<CharT>(edit_textel_normal, { Color16::DarkCyan, Color16::Transparent2 }, true);
                  dialog_editor.set_sstr_vec_pre({ 1, 8 }, disp_glyph_normal);
                  dialog_editor[1] = str::rep_char(' ', 11 + c_max_glyph_disp_width)
                    + "("
                    + str::rep_char(' ', t8::get_sstr_vec_width(disp_glyph_normal))
                    + ")";
                  dialog_editor.set_sstr_vec_pre({ 1, 12 + c_max_glyph_disp_width }, disp_glyph_normal);
                }
                dialog_editor.set_tab_selection(0);
                edit_mode = EditTextelMode::EditTextelShadow;
              }
            }
            else if (curr_special_key == t8::SpecialKey::Escape)
            {
              reset_textel_editor();
              show_textel_editor = false;
            }
            
            t8x::TextBoxDrawingArgsAlign tb_args;
            tb_args.base.box_style = dlg_style;
            tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
            tb_args.base.box_padding_lr = 1;
            tb_args.v_align_offs = -2;
            dialog_editor.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
            
          case EditTextelMode::EditTextelShadow:
          {
            dialog_editor.update(curr_key, curr_special_key);
            edit_textel_name = dialog_editor.get_text_field_input(0);
            if (edit_textel_presets_as_ascii_only)
              edit_textel_shadow.glyph = dialog_editor.text_field_empty(1) ? ' ' : dialog_editor.get_text_field_input(1)[0];
            else
              edit_textel_shadow.glyph = dialog_editor.get_glyph_picker_glyph(1);
            edit_textel_shadow.fg_color = dialog_editor.get_color_picker_color(2);
            edit_textel_shadow.bg_color = dialog_editor.get_color_picker_color(3);
            //edit_textel_preset->textel_shadow = edit_textel_shadow;
            if (edit_textel_presets_as_ascii_only)
              dialog_editor.set_textel_pre({ 1, 8 }, edit_textel_shadow.glyph, edit_textel_shadow.fg_color, edit_textel_shadow.bg_color);
            else
            {
              //edit_textel_preset->update_disp_strings<CharT>({ Color16::DarkCyan, Color16::Transparent2 }, false);
              //dialog_editor.set_sstr_vec_pre({ 1, 8 }, edit_textel_preset->get_glyph_disp_sstr(true));
              auto disp_glyph_shadow = format_long_glyph_disp_sstr<CharT>(edit_textel_shadow, { Color16::DarkCyan, Color16::Transparent2 }, false);
              dialog_editor.set_sstr_vec_pre({ 1, 8 }, disp_glyph_shadow);
            }
            if (curr_special_key == t8::SpecialKey::Enter)
            {
              if (dialog_editor.text_field_empty(0))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel preset name.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if ((edit_textel_presets_as_ascii_only && dialog_editor.text_field_empty(1))
                    || (!edit_textel_presets_as_ascii_only && edit_textel_shadow.glyph.fully_empty()))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel " + (edit_textel_presets_as_ascii_only ? "character"s : "glyph"s) + ".",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if (!edit_textel_presets_as_ascii_only && !dialog_editor.glyph_picker_valid(1))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "Unicode textel glyphs need an ASCII fallback.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else if (dialog_editor.text_field_empty(4))
              {
                message_handler->add_message(static_cast<float>(get_real_time_s()),
                                             "You must enter a textel material.",
                                             t8x::MessageHandlerLevel::Guide);
              }
              else
              {
                edit_textel_shadow.glyph.try_canonicalize_from_fallback();
                edit_textel_shadow.mat = std::stoi(dialog_editor.get_text_field_input(4));
                if (edit_or_add == EditOrAdd::Edit && edit_textel_preset != nullptr)
                {
                  edit_textel_preset->name = edit_textel_name;
                  edit_textel_preset->textel_shadow = edit_textel_shadow;
                }
                else if (edit_or_add == EditOrAdd::Add)
                  custom_textel_presets.emplace_back(edit_textel_normal, edit_textel_shadow, edit_textel_name);
                
                {
                  if (!edit_textel_presets_as_ascii_only && gp_textel_symbol != nullptr)
                    gp_textel_symbol->push_recent();
                
                  std::vector<std::string> lines_custom_textel_presets;
                  for (const auto& ctp : custom_textel_presets)
                  {
                    // '%', Magenta, Cyan, 28
                    // '%', DarkMagenta, DarkCyan, 28
                    // Magic Stone
                    lines_custom_textel_presets.emplace_back(ctp.textel_normal.glyph.str(false) + ", "
                      + ctp.textel_normal.fg_color.str() + ", "
                      + ctp.textel_normal.bg_color.str() + ", "
                      + std::to_string(ctp.textel_normal.mat));
                    lines_custom_textel_presets.emplace_back(ctp.textel_shadow.glyph.str(false) + ", "
                      + ctp.textel_shadow.fg_color.str() + ", "
                      + ctp.textel_shadow.bg_color.str() + ", "
                      + std::to_string(ctp.textel_shadow.mat));
                    lines_custom_textel_presets.emplace_back(ctp.name);
                  }
                  if (TextIO::write_file(filepath_custom_textel_presets, lines_custom_textel_presets))
                    message_handler->add_message(static_cast<float>(get_real_time_s()),
                                                 "Successfully wrote to custom textel presets file!",
                                                 t8x::MessageHandlerLevel::Guide);
                  else
                    message_handler->add_message(static_cast<float>(get_real_time_s()),
                                                 "Unable to write to custom textel presets file!",
                                                 t8x::MessageHandlerLevel::Fatal);
                                                 
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
            tb_args.base.box_style = dlg_style;
            tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
            tb_args.base.box_padding_lr = 1;
            tb_args.v_align_offs = -2;
            dialog_editor.draw(sh, tb_args, cursor_anim_ctr);
            break;
          }
        }
        
        t8x::TextBoxDrawingArgsAlign tb_args;
        tb_args.base.box_style = { Color16::LightGray, Color16::DarkBlue };
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.base.box_padding_lr = 1;
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
        tb_ui_help_edit_textel[static_cast<int>(edit_mode)].draw(sh, tb_args);
      }
      else if (show_adhoc_textel_editor)
      {
        allow_editing = false;
        dialog_editor_adhoc.update(curr_key, curr_special_key);
        if (edit_textel_presets_as_ascii_only)
          edit_textel_normal.glyph = dialog_editor_adhoc.text_field_empty(0) ? ' ' : dialog_editor_adhoc.get_text_field_input(0)[0];
        else
          edit_textel_normal.glyph = dialog_editor_adhoc.get_glyph_picker_glyph(0);
        edit_textel_normal.fg_color = dialog_editor_adhoc.get_color_picker_color(1);
        edit_textel_normal.bg_color = dialog_editor_adhoc.get_color_picker_color(2);
        if (edit_textel_presets_as_ascii_only)
          dialog_editor_adhoc.set_textel_pre({ 1, 8 }, edit_textel_normal.glyph, edit_textel_normal.fg_color, edit_textel_normal.bg_color);
        else
        {
          const auto& disp_sstr_vec = format_long_glyph_disp_sstr<CharT>(edit_textel_normal,
            { Color16::DarkCyan, Color16::Transparent2 }, false);
          dialog_editor_adhoc.set_sstr_vec_pre({ 1, 8 }, disp_sstr_vec);
        }
        
        if (curr_special_key == t8::SpecialKey::Enter)
        {
          if (!edit_textel_presets_as_ascii_only && !dialog_editor_adhoc.glyph_picker_valid(0))
          {
            message_handler->add_message(static_cast<float>(get_real_time_s()),
                                         "Unicode textel glyphs need an ASCII fallback.",
                                         t8x::MessageHandlerLevel::Guide);
          }
          else
          {
            edit_textel_preset_adhoc = &textel_presets[0];
            edit_textel_normal.glyph.try_canonicalize_from_fallback();
            edit_textel_preset_adhoc->textel_normal = edit_textel_normal;
            edit_textel_preset_adhoc->textel_shadow = edit_textel_normal;
            edit_textel_preset_adhoc->update_disp_strings<CharT>({ Color16::DarkGray, Color16::Transparent2 }, true);
            
            reset_adhoc_textel_editor();
            show_adhoc_textel_editor = false;
          }
        }
        else if (curr_special_key == t8::SpecialKey::Escape)
        {
          reset_adhoc_textel_editor();
          show_adhoc_textel_editor = false;
        }
        
        t8x::TextBoxDrawingArgsAlign tb_args;
        tb_args.base.box_style = dlg_style;
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.base.box_padding_lr = 1;
        tb_args.v_align_offs = -2;
        dialog_editor_adhoc.draw(sh, tb_args, cursor_anim_ctr);
        
        tb_args.base.box_style.fg_color = Color16::LightGray;
        tb_args.base.outline_type = t8x::OutlineType::Unicode_SingleLine;
        tb_args.v_align_offs = 0;
        tb_args.v_align = t8x::VerticalAlignment::BOTTOM;
        tb_args.h_align = t8x::HorizontalAlignment::RIGHT;
        tb_ui_help_edit_adhoc.draw(sh, tb_args);
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
  bool show_adhoc_textel_editor = false;
  bool show_materials = false;
  
  t8x::YesNoButtons overwrite_confirm_button = t8x::YesNoButtons::No;
  bool safe_to_save = false;
  
  std::vector<TextelItem> textel_presets; // Including custom textel presets.
  int selected_textel_preset_idx = 0;
  std::vector<TextelItem> custom_textel_presets;
  
  std::unique_ptr<t8x::MessageHandler<std::string>> message_handler;
  t8x::MessageBoxDrawingArgs msg_box_drawing_args;
  using UndoItem = std::vector<std::pair<t8::RC, Textel>>;
  std::stack<UndoItem> undo_buffer;
  std::stack<UndoItem> redo_buffer;
  bool is_modified = false;
  
  bool draw_vert_coords = false;
  bool draw_horiz_coords = false;
  bool draw_vert_coord_line = false;
  bool draw_horiz_coord_line = false;
  
  bool use_shadow_textels = false;
  
  float big_brush_aspect_ratio = 1.84f; // Measured on huge font on MacOS Terminal.
  float big_brush_radius = 10.5f; // Good radius that creates a fairly symmetrically circurlar brush stroke.
  
  t8x::TextBoxDebug tbd { str::Adjustment::Left };
  
  t8::Style dlg_style { Color16::White, Color16::DarkBlue };
  t8::ButtonStyle btn_style { Color16::White, Color16::DarkBlue, Color16::Blue };
  t8::PromptStyle tf_style { Color16::White, Color16::DarkBlue, Color16::White, Color16::Cyan };
  t8x::ButtonFrame btn_frame = t8x::ButtonFrame::SquareBrackets;
  
  t8x::TextBox<std::string> tb_ui_help_goto {{
      "UI Help"s,
      "Type coordinates using number keys.",
      "Erase characters by pressing [BACKSPACE].",
      "Toggle coordinate input by pressing [TAB].",
      "Press [ENTER] when done.",
      "Press [ESCAPE] to cancel."
    }};
  t8x::Dialog<std::string> dialog_goto;
  
  t8x::TextBox<std::string> tb_ui_help_keys {{
    "UI Help"s,
    "Press [ESCAPE] or [K] to close."
  }};
  t8x::Dialog<std::string> dialog_keys;
  
  enum class EditTextelMode { EditOrAdd, EditEnterMat, EditTextelNormal, EditTextelShadow };
  enum class EditOrAdd { None, Edit, Add };
  std::array<t8x::TextBox<std::string>, 4> tb_ui_help_edit_textel
  {
    {
      { { "UI Help"s, "Use arrow keys or [TAB] to select button,", "Then press [ENTER] when done.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Type in the textel preset idx using the number keys.", "First custom textel preset starts at idx = 0.", "Erase characters by pressing [BACKSPACE].", "Press [ENTER] when done.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Navigate widgets using [TAB].", "Erase characters with [BACKSPACE].", "Use arrow keys to select a color.", "Press [ENTER] when you're ready to edit the shadow textel.", "Press [ESCAPE] to cancel." } },
      { { "UI Help"s, "Navigate widgets using [TAB].", "Erase characters with [BACKSPACE].", "Use arrow keys to select a color.", "Press [ENTER] when you're ready to save the textel preset.", "Press [ESCAPE] to cancel." } }
    }
  };
  EditTextelMode edit_mode = EditTextelMode::EditOrAdd;
  EditOrAdd edit_or_add = EditOrAdd::None;
  t8x::Dialog<std::string> dialog_edit_or_add;
  t8x::Dialog<std::string> dialog_edit_mat;
  t8x::Dialog<std::string> dialog_editor;
  t8x::TextField tf_textel_symbol { 1, t8x::TextFieldMode::All, tf_style, 1 };
  t8x::GlyphPicker* gp_textel_symbol = nullptr;
  t8x::GlyphPicker* gp_textel_symbol_adhoc = nullptr;
  bool force_8bit_colors_on_win_cmd = false;
  bool edit_textel_presets_as_ascii_only = false;
  bool save_textures_as_ascii_only = false;
  t8x::ColorPickerParams cp_params;
  t8x::TextField tf_textel_mat { 4, t8x::TextFieldMode::Numeric, tf_style, 4 };
  TextelItem* edit_textel_preset = nullptr;
  Textel edit_textel_normal;
  Textel edit_textel_shadow;
  TextelItem* edit_textel_preset_adhoc = nullptr;
  std::string edit_textel_name;
  
  t8x::TextBox<std::string> tb_ui_help_edit_adhoc {{
    "UI Help"s,
    "Navigate widgets using [TAB].",
    "Erase characters with [BACKSPACE].",
    "Use arrow keys to select a color.",
    "Press [ENTER] to submit the changes.",
    "Press [ESCAPE] to cancel."
  }};
  t8x::Dialog<std::string> dialog_editor_adhoc;
  t8x::TextField tf_textel_symbol_adhoc { 1, t8x::TextFieldMode::All, tf_style, 0 };
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
    else if (std::strcmp(argv[a_idx], "--display_ascii_only") == 0)
      params.ascii_fallback_policy = t8::AsciiFallbackPolicy::FORCE_ASCII;
  }

  Game game(argc, argv, params);

  game.init();
  game.generate_data();
  game.run();

  return EXIT_SUCCESS;
}
