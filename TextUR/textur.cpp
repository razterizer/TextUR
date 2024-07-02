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
#include <Core/Rand.h>

#include <iostream>

enum class EditorFileMode { NEW_OR_OVERWRITE_FILE, OPEN_EXISTING_FILE };


class Game : public GameEngine<>
{
public:
  Game(int argc, char** argv, const GameEngineParams& params)
    : GameEngine(argv[0], params)
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
  }
  
private:
  virtual void update() override
  {
    using namespace drawing;
    
    styles::Style ui_style { Color::LightGray, Color::Black };
    
    if (show_menu)
      draw_box_outline(sh, 0, sh.num_cols() - 10, sh.num_rows() - 1, 10 - 1, drawing::OutlineType::Line, ui_style);
  
    draw_frame(sh, Color::White);
    
    // Caret
    if (anim_ctr % 2 == 0)
      sh.write_buffer("#", caret_pos.r+1, caret_pos.c+1, ui_style);
      
    //draw_box_outline(sh,
    //                 screen_pos.r, screen_pos.c, sh.num_rows_inset()+1, sh.num_cols_inset()+1,
    //                 drawing::OutlineType::Line,
    //                 ui_style);
    draw_box_textured(sh,
                      screen_pos.r, screen_pos.c,
                      sh.num_rows_inset()+1, sh.num_cols_inset()+1,
                      drawing::Direction::None,
                      curr_texture);
                      
    // Keypresses:
    if (kpd.curr_key == ' ')
      math::toggle(show_menu);
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
  RC caret_pos { 0, 0 };
  
  bool show_menu = false;
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
