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

public:
  Game(int argc, char** argv, const GameEngineParams& params)
    : GameEngine(argv[0], params)
    , message_handler(std::make_unique<MessageHandler>())
  {
    if (argc >= 2)
      texture_file_path = argv[1];
    else
    {
      std::cerr << "You must supply a texture filename as a command line argument!" << std::endl;
      request_exit();
    }
      
    if (argc >= 4)
    {
      file_mode = EditorFileMode::NEW_OR_OVERWRITE_FILE;
      
      std::istringstream iss(argv[2]);
      RC size;
      iss >> size.r;
      iss.str(argv[3]);
      iss.clear();
      iss >> size.c;
      
      curr_texture = drawing::Texture { size };
    }
    else
      curr_texture.load(texture_file_path);
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
    tbd.add(PARAM(caret_pos.r));
    tbd.add(PARAM(caret_pos.c));
  }
  
private:
  virtual void update() override
  {
    using namespace drawing;
    
    styles::Style ui_style { Color::LightGray, Color::Black };
    
    const int nr = sh.num_rows();
    const int nc = sh.num_cols();
    const int nri = sh.num_rows_inset();
    //const int nci = sh.num_cols_inset();
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
      draw_confirm(sh, "Are you sure you want to overwrite the file \"" + texture_file_path + "\"?", overwrite_confirm_button,
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
      if (anim_ctr % 2 == 0)
        sh.write_buffer("#", screen_pos.r + caret_pos.r + 1, screen_pos.c + caret_pos.c + 1, ui_style);
      
      //draw_box_outline(sh,
      //                 screen_pos.r, screen_pos.c, sh.num_rows_inset()+1, sh.num_cols_inset()+1,
      //                 drawing::OutlineType::Line,
      //                 ui_style);
      draw_box_textured(sh,
                        screen_pos.r, screen_pos.c,
                        sh.num_rows_inset()+1, sh.num_cols_inset()+1,
                        drawing::Direction::None,
                        curr_texture);
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
        caret_pos.r--;
        if (caret_pos.r < 0)
          caret_pos.r = 0;
        if (caret_pos.r - screen_pos.r <= 0)
          screen_pos.r++;
      }
      else if (is_down)
      {
        caret_pos.r++;
        if (caret_pos.r >= static_cast<int>(curr_texture.size.r))
          caret_pos.r = curr_texture.size.r - 1;
        if (caret_pos.r - screen_pos.r >= nri)
          screen_pos.r--;
      }
      else if (is_left)
      {
        caret_pos.c--;
        if (caret_pos.c < 0)
          caret_pos.c = 0;
      }
      else if (is_right)
      {
        caret_pos.c++;
        if (caret_pos.c >= static_cast<int>(curr_texture.size.c))
          caret_pos.c = curr_texture.size.c - 1;
      }
      else if (kpd.curr_key == ' ')
      {
        undo_buffer.push({ caret_pos, curr_texture(caret_pos) });
        curr_texture.set_textel(caret_pos, textel_presets[selected_textel_preset_idx].textel);
        redo_buffer = {};
      }
      else if (str::to_lower(kpd.curr_key) == 'z')
      {
        if (!undo_buffer.empty())
        {
          auto& up = undo_buffer.top();
          redo_buffer.push({ up.first, curr_texture(up.first) });
          curr_texture.set_textel(up.first, up.second);
          undo_buffer.pop();
        }
      }
      else if (str::to_lower(kpd.curr_key) == 'y')
      {
        if (!redo_buffer.empty())
        {
          auto& up = redo_buffer.top();
          undo_buffer.push({ up.first, curr_texture(up.first) });
          curr_texture.set_textel(up.first, up.second);
          redo_buffer.pop();
        }
      }
    }
    if (str::to_lower(kpd.curr_key) == 'x' || safe_to_save)
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      {
        if (folder::exists(texture_file_path))
        {
          show_confirm_overwrite = true;
          overwrite_confirm_button = YesNoButtons::No;
        }
      }
      else
        safe_to_save = true;
        
      if (safe_to_save)
      {
        if (curr_texture.save(texture_file_path))
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
  std::string texture_file_path;
  EditorFileMode file_mode = EditorFileMode::OPEN_EXISTING_FILE;
  
  RC screen_pos { 0, 0 };
  //ttl::Rectangle m_screen_in_world;
  RC caret_pos { 0, 0 };
  int menu_r_offs = 0;
  
  bool show_menu = false;
  bool show_confirm_overwrite = false;
  
  YesNoButtons overwrite_confirm_button = YesNoButtons::No;
  bool safe_to_save = false;
  
  std::vector<TextelItem> textel_presets;
  int selected_textel_preset_idx = 0;
  
  std::unique_ptr<MessageHandler> message_handler;
  std::stack<std::pair<RC, drawing::Textel>> undo_buffer;
  std::stack<std::pair<RC, drawing::Textel>> redo_buffer;
  
  ui::TextBoxDebug tbd;
};

int main(int argc, char** argv)
{
  GameEngineParams params;
  params.enable_title_screen = false;
  params.enable_instructions_screen = false;
  params.enable_quit_confirm_screen = false;
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
