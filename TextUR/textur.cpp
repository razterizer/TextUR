//
//  main.cpp
//  TextUR
//
//  Created by Rasmus Anthin on 2024-07-01.
//

#include <Termin8or/GameEngine.h>
#include <Termin8or/Screen.h>
#include <Termin8or/Drawing.h>
#include <Termin8or/Texture.h>
#include <Termin8or/RC.h>
#include <Termin8or/ASCII_Fonts.h>
#include <Termin8or/MessageHandler.h>
#include <Termin8or/UI.h>
#include <Core/Rand.h>

#include <iostream>
#include <stack>

enum class EditorFileMode { NEW_OR_OVERWRITE_FILE, OPEN_EXISTING_FILE };


class Game : public GameEngine<>
{
  struct TextelItem
  {
    TextelItem(drawing::Textel t, std::string a_name)
      : textel(std::move(t))
      , name(std::move(a_name))
    {}
    
    drawing::Textel textel;
    std::string name;
  };

  void draw_menu(const styles::Style& ui_style, const int menu_width)
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
        name_style.fg_color = Color::Cyan;
        if (r + 2 >= nri)
          menu_r_offs -= 3;
        else if (r < 0)
          menu_r_offs += 3;
      }
      const auto& preset = textel_presets[p_idx];
      const auto& textel = preset.textel;
      sh.write_buffer(textel.str(), r + 1, nc - menu_width + 2, textel.get_style());
      sh.write_buffer(preset.name, r + 2, nc - menu_width + 2, name_style);
      draw_box_outline(sh, r, nc - menu_width, 4 - 1, menu_width - 1, drawing::OutlineType::Line, ui_style);
      r += 3;
    }
  }
  
  void draw_coord_sys(bool draw_v_coords, bool draw_h_coords, int nc, int menu_width)
  {
    if (draw_v_coords)
    {
      const int str_max_len = curr_texture.size.r == 0 ? 0 : static_cast<int>(1 + std::log10(std::max(1, curr_texture.size.r - 1)));
      for (int r = 0; r < curr_texture.size.r; ++r)
        sh.write_buffer(str::adjust_str(std::to_string(r), str::Adjustment::Right, str_max_len), screen_pos.r + r + 1, screen_pos.c + 1, Color::Red);
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
          sh.write_buffer(std::string(1, str[r]), screen_pos.r + r + 1, screen_pos.c + c + 1, Color::Green);
      }
    }
  }

public:
  Game(int argc, char** argv, const GameEngineParams& params)
    : GameEngine(argv[0], params)
    , message_handler(std::make_unique<MessageHandler>())
  {
    RC size;
  
    for (int a_idx = 1; a_idx < argc; ++a_idx)
    {
      if (std::strcmp(argv[a_idx], "-f") == 0 && a_idx + 1 < argc)
        file_path_curr_texture = argv[a_idx + 1];
      else if (std::strcmp(argv[a_idx], "-s") == 0 && a_idx + 2 < argc)
      {
        file_mode = EditorFileMode::NEW_OR_OVERWRITE_FILE;
        
        std::istringstream iss(argv[a_idx + 1]);
        iss >> size.r;
        iss.str(argv[a_idx + 2]);
        iss.clear();
        iss >> size.c;
      }
      else if (std::strcmp(argv[a_idx], "-t") == 0 && a_idx + 1 < argc)
        file_path_tracing_texture = argv[a_idx + 1];
    }
  
    if (file_path_curr_texture.empty())
    {
      std::cerr << "You must supply a texture filename as a command line argument!" << std::endl;
      request_exit();
    }
      
    if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      curr_texture = drawing::Texture { size };
    else
      curr_texture.load(file_path_curr_texture);
      
    if (!file_path_tracing_texture.empty())
      tracing_texture.load(file_path_tracing_texture);
  }
  
  virtual void generate_data() override
  {
    font_data_path = ASCII_Fonts::get_path_to_font_data(get_exe_folder());
    //std::cout << font_data_path << std::endl;
    
    color_schemes.emplace_back();
    auto& cs = color_schemes.emplace_back();
    cs.internal.bg_color = Color::DarkCyan;
    cs.side_h.bg_color = Color::DarkBlue;
    
    font_data = ASCII_Fonts::load_font_data(font_data_path);
    
    textel_presets.emplace_back(drawing::Textel { ' ', Color::Default, Color::Black, 0 }, "Void");
    textel_presets.emplace_back(drawing::Textel { '_', Color::Default, Color::LightGray, 1 }, "Tile");
    textel_presets.emplace_back(drawing::Textel { '~', Color::DarkBlue, Color::Blue, 2 }, "Water0");
    textel_presets.emplace_back(drawing::Textel { '*', Color::White, Color::Blue, 2 }, "Water1");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkYellow, Color::Yellow, 3 }, "Sand0");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkYellow, Color::Yellow, 3 }, "Sand1");
    textel_presets.emplace_back(drawing::Textel { '8', Color::DarkGray, Color::LightGray, 4 }, "Stone0");
    textel_presets.emplace_back(drawing::Textel { 'o', Color::DarkGray, Color::LightGray, 4 }, "Stone1");
    textel_presets.emplace_back(drawing::Textel { 'H', Color::DarkGray, Color::LightGray, 5 }, "Masonry");
    textel_presets.emplace_back(drawing::Textel { '#', Color::DarkRed, Color::Red, 6 }, "Brick");
    textel_presets.emplace_back(drawing::Textel { '|', Color::DarkGreen, Color::Green, 7 }, "Grass0");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkGreen, Color::Green, 7 }, "Grass1");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkGreen, Color::Green, 7 }, "Grass2");
    textel_presets.emplace_back(drawing::Textel { '/', Color::DarkGreen, Color::Green, 7 }, "Grass3");
    textel_presets.emplace_back(drawing::Textel { '\\', Color::DarkGreen, Color::Green, 7 }, "Grass4");
    textel_presets.emplace_back(drawing::Textel { '&', Color::DarkYellow, Color::Green, 8 }, "Shrub0");
    textel_presets.emplace_back(drawing::Textel { '@', Color::DarkGray, Color::Green, 8 }, "Shrub1");
    textel_presets.emplace_back(drawing::Textel { '*', Color::DarkGreen, Color::Green, 8 }, "Shrub2");
    textel_presets.emplace_back(drawing::Textel { 'T', Color::DarkRed, Color::Green, 9 }, "Tree0");
    textel_presets.emplace_back(drawing::Textel { 'Y', Color::DarkRed, Color::Green, 9 }, "Tree1");
    textel_presets.emplace_back(drawing::Textel { '=', Color::DarkGray, Color::LightGray, 10 }, "Metal");
    textel_presets.emplace_back(drawing::Textel { 'W', Color::DarkRed, Color::Yellow, 11 }, "Wood0");
    textel_presets.emplace_back(drawing::Textel { 'E', Color::DarkRed, Color::Yellow, 11 }, "Wood1");
    textel_presets.emplace_back(drawing::Textel { 'Z', Color::DarkRed, Color::Yellow, 11 }, "Wood2");
    textel_presets.emplace_back(drawing::Textel { 'X', Color::DarkBlue, Color::Cyan, 12 }, "Ice");
    textel_presets.emplace_back(drawing::Textel { '^', Color::DarkGray, Color::LightGray, 13 }, "Mountain");
    textel_presets.emplace_back(drawing::Textel { 'W', Color::DarkRed, Color::Red, 14 }, "Lava");
    textel_presets.emplace_back(drawing::Textel { 'C', Color::DarkYellow, Color::Yellow, 15 }, "Cave");
    textel_presets.emplace_back(drawing::Textel { 'S', Color::DarkRed, Color::Green, 16 }, "Swamp");
    textel_presets.emplace_back(drawing::Textel { '~', Color::DarkGreen, Color::Green, 17 }, "Poison");
    textel_presets.emplace_back(drawing::Textel { '#', Color::DarkYellow, Color::Green, 18 }, "Path");
    textel_presets.emplace_back(drawing::Textel { 'M', Color::DarkGray, Color::LightGray, 19 }, "Mine");
    textel_presets.emplace_back(drawing::Textel { 'G', Color::DarkYellow, Color::Yellow, 20 }, "Gold");

    tbd.add(PARAM(screen_pos.r));
    tbd.add(PARAM(screen_pos.c));
    tbd.add(PARAM(cursor_pos.r));
    tbd.add(PARAM(cursor_pos.c));
  }
  
private:
  virtual void update() override
  {
    using namespace drawing;
    
    styles::Style ui_style { Color::LightGray, Color::Black };
    
    const int nr = sh.num_rows();
    const int nc = sh.num_cols();
    const int nri = sh.num_rows_inset();
    const int nci = sh.num_cols_inset();
    const int menu_width = 12;

//#define SHOW_DEBUG_WINDOW
#ifdef SHOW_DEBUG_WINDOW
    tbd.calc_pre_draw(str::Adjustment::Left);
    tbd.draw(sh, ui::VerticalAlignment::TOP, ui::HorizontalAlignment::CENTER, { Color::Blue, Color::Yellow }, true, true, 0, 0, std::nullopt, drawing::OutlineType::Line, true);
#endif
      
    if (!show_confirm_overwrite && show_menu)
      draw_box_outline(sh, 0, nc - menu_width, nr - 1, menu_width - 1, drawing::OutlineType::Line, ui_style);
  
    draw_frame(sh, Color::White);
    
    if (show_confirm_overwrite)
    {
      bg_color = Color::DarkCyan;
      draw_confirm(sh, "Are you sure you want to overwrite the file \"" + file_path_curr_texture + "\"?", overwrite_confirm_button,
                   { Color::Black, Color::DarkCyan },
                   { Color::Black, Color::DarkCyan, Color::Cyan },
                   { Color::White, Color::DarkCyan });
      if (kpd.curr_special_key == keyboard::SpecialKey::Left)
        overwrite_confirm_button = YesNoButtons::Yes;
      else if (kpd.curr_special_key == keyboard::SpecialKey::Right)
        overwrite_confirm_button = YesNoButtons::No;
      
      if (kpd.curr_special_key == keyboard::SpecialKey::Enter)
      {
        if (overwrite_confirm_button == YesNoButtons::Yes)
          safe_to_save = true;
        else
          show_confirm_overwrite = false;
      }
    }
    else
    {
      if (show_menu)
        draw_menu(ui_style, menu_width);
        
      message_handler->update(sh, static_cast<float>(sim_time_s));
      
      // Caret
      if (anim_ctr % 2 == 0 && (!show_menu || screen_pos.c + cursor_pos.c + 1 < nc - menu_width))
        sh.write_buffer("#", screen_pos.r + cursor_pos.r + 1, screen_pos.c + cursor_pos.c + 1, ui_style);
      
      int box_width_curr = curr_texture.size.c + 1;
      int box_width_tracing = tracing_texture.size.c + 1;
      if (show_menu)
      {
        if (curr_texture.size.c > nc - menu_width)
          box_width_curr = nc - menu_width - screen_pos.c + 1;
        if (tracing_texture.size.c > nc - menu_width)
          box_width_tracing = nc - menu_width - screen_pos.c + 1;
      }
      draw_box_textured(sh,
                        screen_pos.r, screen_pos.c,
                        curr_texture.size.r+1, box_width_curr,
                        drawing::Direction::None,
                        curr_texture);
      if (show_tracing && (tracing_texture.size.r > 0 || tracing_texture.size.c > 0))
      {
        draw_box_textured(sh,
                          screen_pos.r, screen_pos.c,
                          tracing_texture.size.r+1, box_width_tracing,
                          drawing::Direction::None,
                          tracing_texture);
      }
      
      draw_coord_sys(draw_vert_coords, draw_horiz_coords, nc, menu_width);
    }
                      
    // Keypresses:
    if (kpd.curr_key == '-')
      math::toggle(show_menu);
      
    bool is_up = kpd.curr_special_key == keyboard::SpecialKey::Up || str::to_lower(kpd.curr_key) == 'w';
    bool is_down = kpd.curr_special_key == keyboard::SpecialKey::Down || str::to_lower(kpd.curr_key) == 's';
    bool is_left = kpd.curr_special_key == keyboard::SpecialKey::Left || str::to_lower(kpd.curr_key) == 'a';
    bool is_right = kpd.curr_special_key == keyboard::SpecialKey::Right || str::to_lower(kpd.curr_key) == 'd';
    if (show_menu)
    {
      if (is_up)
      {
        selected_textel_preset_idx--;
        if (selected_textel_preset_idx == -1)
          selected_textel_preset_idx = static_cast<int>(textel_presets.size()) - 1;
      }
      else if (is_down)
      {
        selected_textel_preset_idx++;
        if (selected_textel_preset_idx == static_cast<int>(textel_presets.size()))
          selected_textel_preset_idx = 0;
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
      else if (kpd.curr_key == ' ')
      {
        undo_buffer.push({ cursor_pos, curr_texture(cursor_pos) });
        curr_texture.set_textel(cursor_pos, textel_presets[selected_textel_preset_idx].textel);
        redo_buffer = {};
      }
      else if (kpd.curr_key == 'z')
      {
        if (!undo_buffer.empty())
        {
          auto& up = undo_buffer.top();
          redo_buffer.push({ up.first, curr_texture(up.first) });
          curr_texture.set_textel(up.first, up.second);
          undo_buffer.pop();
        }
      }
      else if (kpd.curr_key == 'Z')
      {
        if (!redo_buffer.empty())
        {
          auto& up = redo_buffer.top();
          undo_buffer.push({ up.first, curr_texture(up.first) });
          curr_texture.set_textel(up.first, up.second);
          redo_buffer.pop();
        }
      }
      else if (str::to_lower(kpd.curr_key) == 'h')
        math::toggle(draw_horiz_coords);
      else if (str::to_lower(kpd.curr_key) == 'v')
        math::toggle(draw_vert_coords);
      else if (str::to_lower(kpd.curr_key) == 'c')
      {
        undo_buffer.push({ cursor_pos, curr_texture(cursor_pos) });
        curr_texture.set_textel(cursor_pos, Textel {});
        redo_buffer = {};
      }
      else if (str::to_lower(kpd.curr_key) == 'b')
      {
        undo_buffer.push({ cursor_pos, curr_texture(cursor_pos) });
        for (int i = -2; i <= 2; ++i)
        {
          int j_offs = std::abs(i) == 2 ? 2 : 4;
          for (int j = -j_offs; j <= j_offs; ++j)
          {
            RC offs { i, j };
            undo_buffer.push({ cursor_pos + offs, curr_texture(cursor_pos + offs) });
            curr_texture.set_textel(cursor_pos + offs, textel_presets[selected_textel_preset_idx].textel);
          }
        }
        redo_buffer = {};
      }
      else if (str::to_lower(kpd.curr_key) == 'l')
        message_handler->add_message(static_cast<float>(sim_time_s),
                                     "Cursor @ " + cursor_pos.str(),
                                     MessageHandler::Level::Guide);
      else if (str::to_lower(kpd.curr_key) == 't')
        math::toggle(show_tracing);
    }
    if (str::to_lower(kpd.curr_key) == 'x' || safe_to_save)
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      {
        if (folder::exists(file_path_curr_texture))
        {
          show_confirm_overwrite = true;
          overwrite_confirm_button = YesNoButtons::No;
        }
      }
      else
        safe_to_save = true;
        
      if (safe_to_save)
      {
        if (curr_texture.save(file_path_curr_texture))
          message_handler->add_message(static_cast<float>(sim_time_s),
                                       "Your work was successfully saved.",
                                       MessageHandler::Level::Guide);
        else
          message_handler->add_message(static_cast<float>(sim_time_s),
                                       "An error occurred while saving your work!",
                                       MessageHandler::Level::Fatal);
                                       
        safe_to_save = false;
        show_confirm_overwrite = false;
      }
    }
  }
  
  virtual void draw_title() override
  {
    //::draw_title(sh, font_data, color_schemes, text);
  }
  
  virtual void draw_instructions() override
  {
    //::draw_instructions(sh, 0);
  }
    
  std::vector<ASCII_Fonts::ColorScheme> color_schemes;
  
  std::string font_data_path;
  
  ASCII_Fonts::FontDataColl font_data;
  
  drawing::Texture curr_texture;
  drawing::Texture tracing_texture;
  std::string file_path_curr_texture;
  std::string file_path_tracing_texture;
  EditorFileMode file_mode = EditorFileMode::OPEN_EXISTING_FILE;
  
  RC screen_pos { 0, 0 };
  RC cursor_pos { 0, 0 };
  int menu_r_offs = 0;
  
  bool show_menu = false;
  bool show_confirm_overwrite = false;
  bool show_tracing = true;
  
  YesNoButtons overwrite_confirm_button = YesNoButtons::No;
  bool safe_to_save = false;
  
  std::vector<TextelItem> textel_presets;
  int selected_textel_preset_idx = 0;
  
  std::unique_ptr<MessageHandler> message_handler;
  std::stack<std::pair<RC, drawing::Textel>> undo_buffer;
  std::stack<std::pair<RC, drawing::Textel>> redo_buffer;
  
  bool draw_vert_coords = true;
  bool draw_horiz_coords = true;
  
  ui::TextBoxDebug tbd;
};

int main(int argc, char** argv)
{
  GameEngineParams params;
  params.enable_title_screen = false;
  params.enable_instructions_screen = false;
  params.enable_quit_confirm_screen = true;
  params.enable_hiscores = false;
  params.screen_bg_color_default = Color::Black;
  params.screen_bg_color_title = Color::DarkYellow;
  params.screen_bg_color_instructions = Color::Black;

  Game game(argc, argv, params);

  game.init();
  game.generate_data();
  game.run();

  return EXIT_SUCCESS;
}
