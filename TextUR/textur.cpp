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
      std::cerr << "You must supply a texture filename as a command line argument!";
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
  }
  
  virtual void generate_data() override
  {
    font_data_path = ASCII_Fonts::get_path_to_font_data(get_exe_folder());
    std::cout << font_data_path << std::endl;
    
    color_schemes.emplace_back();
    auto& cs = color_schemes.emplace_back();
    cs.internal.bg_color = Color::DarkCyan;
    cs.side_h.bg_color = Color::DarkBlue;
    
    font_data = ASCII_Fonts::load_font_data(font_data_path);
  }
  
private:
  virtual void update() override
  {
    draw_frame(sh, Color::White);
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
