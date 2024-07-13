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
    TextelItem(drawing::Textel tn, drawing::Textel ts, std::string a_name)
      : textel_normal(std::move(tn))
      , textel_shadow(std::move(ts))
      , name(std::move(a_name))
    {}
    
    drawing::Textel textel_normal;
    drawing::Textel textel_shadow;
    std::string name;
    
    drawing::Textel get_textel(bool shadow) const
    {
      return shadow ? textel_shadow : textel_normal;
    }
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
      const auto& textel = preset.get_textel(use_shadow_textels);
      sh.write_buffer(textel.str(), r + 1, nc - menu_width + 2, textel.get_style());
      sh.write_buffer(preset.name, r + 2, nc - menu_width + 2, name_style);
      draw_box_outline(sh, r, nc - menu_width, 4 - 1, menu_width - 1, drawing::OutlineType::Line, ui_style);
      r += 3;
    }
  }
  
  void draw_coord_sys(bool draw_v_coords, bool draw_h_coords, int nc, int menu_width)
  {
    static const bool persist = true;
    if (draw_v_coords)
    {
      const int str_max_len = curr_texture.size.r == 0 ? 0 : static_cast<int>(1 + std::log10(std::max(1, curr_texture.size.r - 1)));
      for (int r = 0; r < curr_texture.size.r; ++r)
        sh.write_buffer(str::adjust_str(std::to_string(r), str::Adjustment::Right, str_max_len), screen_pos.r + r + 1, persist ? 1 : screen_pos.c + 1, Color::Red);
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
          sh.write_buffer(std::string(1, str[r]), persist ? r + 1 : screen_pos.r + r + 1, screen_pos.c + c + 1, Color::Green);
      }
    }
  }
  
  void reset_goto_input()
  {
    tb_goto = ui::TextBox({ "Cursor Goto @", str::rep_char(' ', 8) + ", " + str::rep_char(' ', 8) });
    goto_tab = 0;
    goto_caret_idx = { 0, 0 };
  }
  
public:
  Game(int argc, char** argv, const GameEngineParams& params)
    : GameEngine(argv[0], params)
    , message_handler(std::make_unique<MessageHandler>())
  {
    RC size;
  
    for (int a_idx = 1; a_idx < argc; ++a_idx)
    {
      if (std::strcmp(argv[a_idx], "-f") == 0 && a_idx + 1 < argc) // file
      {
        file_path_curr_texture = argv[a_idx + 1];
        file_path_bright_texture = file_path_curr_texture;
      }
      else if (std::strcmp(argv[a_idx], "-s") == 0 && a_idx + 2 < argc) // size
      {
        file_mode = EditorFileMode::NEW_OR_OVERWRITE_FILE;
        
        std::istringstream iss(argv[a_idx + 1]);
        iss >> size.r;
        iss.str(argv[a_idx + 2]);
        iss.clear();
        iss >> size.c;
      }
      else if (std::strcmp(argv[a_idx], "-t") == 0 && a_idx + 1 < argc) // trace
        file_path_tracing_texture = argv[a_idx + 1];
      else if (std::strcmp(argv[a_idx], "-c") == 0 && a_idx + 1 < argc) // convert to new texture
      {
        file_path_curr_texture = argv[a_idx + 1];
        convert = true;
      }
    }
  
    if (file_path_curr_texture.empty())
    {
      std::cerr << "ERROR: You must supply a texture filename as a command line argument!" << std::endl;
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
        curr_texture = drawing::Texture { size };
      else
        curr_texture.load(file_path_curr_texture);
      
      if (!file_path_tracing_texture.empty())
        tracing_texture.load(file_path_tracing_texture);
    }
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
    
    textel_presets.emplace_back(drawing::Textel { ' ', Color::Default, Color::Black, 0 },
                                drawing::Textel { ' ', Color::Default, Color::Black, 0 },
                                "Void");
    textel_presets.emplace_back(drawing::Textel { '_', Color::Default, Color::LightGray, 1 },
                                drawing::Textel { '_', Color::Black, Color::DarkGray, 1 },
                                "Tile0");
    textel_presets.emplace_back(drawing::Textel { '_', Color::White, Color::DarkGray, 1 },
                                drawing::Textel { '_', Color::LightGray, Color::Black, 1 },
                                "Tile1");
    textel_presets.emplace_back(drawing::Textel { '_', Color::LightGray, Color::White, 1 },
                                drawing::Textel { '_', Color::White, Color::LightGray, 1 },
                                "Tile2");
    textel_presets.emplace_back(drawing::Textel { 'L', Color::Default, Color::LightGray, 1 },
                                drawing::Textel { 'L', Color::Black, Color::DarkGray, 1 },
                                "Tile3");
    textel_presets.emplace_back(drawing::Textel { 'L', Color::White, Color::DarkGray, 1 },
                                drawing::Textel { 'L', Color::LightGray, Color::Black, 1 },
                                "Tile4");
    textel_presets.emplace_back(drawing::Textel { 'L', Color::LightGray, Color::White, 1 },
                                drawing::Textel { 'L', Color::White, Color::LightGray, 1 },
                                "Tile5");
    textel_presets.emplace_back(drawing::Textel { '~', Color::DarkBlue, Color::Blue, 2 },
                                drawing::Textel { '~', Color::Blue, Color::DarkBlue, 2 },
                                "Water0");
    textel_presets.emplace_back(drawing::Textel { '*', Color::White, Color::Blue, 2 },
                                drawing::Textel { '*', Color::LightGray, Color::DarkBlue, 2 },
                                "Water1");
    textel_presets.emplace_back(drawing::Textel { '~', Color::DarkCyan, Color::Cyan, 2 },
                                drawing::Textel { '~', Color::Cyan, Color::DarkCyan, 2 },
                                "Water2");
    textel_presets.emplace_back(drawing::Textel { '*', Color::White, Color::Cyan, 2 },
                                drawing::Textel { '*', Color::LightGray, Color::DarkCyan, 2 },
                                "Water3");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkYellow, Color::Yellow, 3 },
                                drawing::Textel { ':', Color::Yellow, Color::DarkYellow, 3 },
                                "Sand0");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkYellow, Color::Yellow, 3 },
                                drawing::Textel { '.', Color::Yellow, Color::DarkYellow, 3 },
                                "Sand1");
    textel_presets.emplace_back(drawing::Textel { '8', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { '8', Color::LightGray, Color::DarkGray, 4 },
                                "Stone0");
    textel_presets.emplace_back(drawing::Textel { 'o', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'o', Color::LightGray, Color::DarkGray, 4 },
                                "Stone1");
    textel_presets.emplace_back(drawing::Textel { 'O', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'O', Color::LightGray, Color::DarkGray, 4 },
                                "Stone2");
    textel_presets.emplace_back(drawing::Textel { 'b', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'b', Color::LightGray, Color::DarkGray, 4 },
                                "Stone3");
    textel_presets.emplace_back(drawing::Textel { 'B', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'B', Color::LightGray, Color::DarkGray, 4 },
                                "Stone4");
    textel_presets.emplace_back(drawing::Textel { 'p', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'p', Color::LightGray, Color::DarkGray, 4 },
                                "Stone5");
    textel_presets.emplace_back(drawing::Textel { 'P', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'P', Color::LightGray, Color::DarkGray, 4 },
                                "Stone6");
    textel_presets.emplace_back(drawing::Textel { 'q', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'q', Color::LightGray, Color::DarkGray, 4 },
                                "Stone7");
    textel_presets.emplace_back(drawing::Textel { '6', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { '6', Color::LightGray, Color::DarkGray, 4 },
                                "Stone8");
    textel_presets.emplace_back(drawing::Textel { '9', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { '9', Color::LightGray, Color::DarkGray, 4 },
                                "Stone9");
    textel_presets.emplace_back(drawing::Textel { 'c', Color::DarkGray, Color::LightGray, 4 },
                                drawing::Textel { 'c', Color::LightGray, Color::DarkGray, 4 },
                                "Stone10");
    textel_presets.emplace_back(drawing::Textel { 'H', Color::DarkGray, Color::LightGray, 5 },
                                drawing::Textel { 'H', Color::LightGray, Color::DarkGray, 5 },
                                "Masonry");
    textel_presets.emplace_back(drawing::Textel { '#', Color::DarkRed, Color::Red, 6 },
                                drawing::Textel { '#', Color::Red, Color::DarkRed, 6 },
                                "Brick");
    textel_presets.emplace_back(drawing::Textel { '|', Color::DarkGreen, Color::Green, 7 },
                                drawing::Textel { '|', Color::Green, Color::DarkGreen, 7 },
                                "Grass0");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkGreen, Color::Green, 7 },
                                drawing::Textel { '.', Color::Green, Color::DarkGreen, 7 },
                                "Grass1");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkGreen, Color::Green, 7 },
                                drawing::Textel { ':', Color::Green, Color::DarkGreen, 7 },
                                "Grass2");
    textel_presets.emplace_back(drawing::Textel { '/', Color::DarkGreen, Color::Green, 7 },
                                drawing::Textel { '/', Color::Green, Color::DarkGreen, 7 },
                                "Grass3");
    textel_presets.emplace_back(drawing::Textel { '\\', Color::DarkGreen, Color::Green, 7 },
                                drawing::Textel { '\\', Color::Green, Color::DarkGreen, 7 },
                                "Grass4");
    textel_presets.emplace_back(drawing::Textel { '|', Color::DarkYellow, Color::Green, 7 },
                                drawing::Textel { '|', Color::Yellow, Color::DarkGreen, 7 },
                                "Grass5");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkYellow, Color::Green, 7 },
                                drawing::Textel { '.', Color::Yellow, Color::DarkGreen, 7 },
                                "Grass6");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkYellow, Color::Green, 7 },
                                drawing::Textel { ':', Color::Yellow, Color::DarkGreen, 7 },
                                "Grass7");
    textel_presets.emplace_back(drawing::Textel { '/', Color::DarkYellow, Color::Green, 7 },
                                drawing::Textel { '/', Color::Yellow, Color::DarkGreen, 7 },
                                "Grass8");
    textel_presets.emplace_back(drawing::Textel { '\\', Color::DarkYellow, Color::Green, 7 },
                                drawing::Textel { '\\', Color::Yellow, Color::DarkGreen, 7 },
                                "Grass9");
    textel_presets.emplace_back(drawing::Textel { '&', Color::DarkYellow, Color::Green, 8 },
                                drawing::Textel { '&', Color::Yellow, Color::DarkGreen, 8 },
                                "Shrub0");
    textel_presets.emplace_back(drawing::Textel { '@', Color::DarkGray, Color::Green, 8 },
                                drawing::Textel { '@', Color::LightGray, Color::DarkGreen, 8 },
                                "Shrub1");
    textel_presets.emplace_back(drawing::Textel { '*', Color::DarkGreen, Color::Green, 8 },
                                drawing::Textel { '*', Color::Green, Color::DarkGreen, 8 },
                                "Shrub2");
    textel_presets.emplace_back(drawing::Textel { 'T', Color::DarkRed, Color::Green, 9 },
                                drawing::Textel { 'T', Color::Red, Color::DarkGreen, 9 },
                                "Tree0");
    textel_presets.emplace_back(drawing::Textel { 'Y', Color::DarkRed, Color::Green, 9 },
                                drawing::Textel { 'Y', Color::Red, Color::DarkGreen, 9 },
                                "Tree1");
    textel_presets.emplace_back(drawing::Textel { '=', Color::DarkGray, Color::LightGray, 10 },
                                drawing::Textel { '=', Color::LightGray, Color::DarkGray, 10 },
                                "Metal");
    textel_presets.emplace_back(drawing::Textel { 'W', Color::DarkRed, Color::Yellow, 11 },
                                drawing::Textel { 'W', Color::Yellow, Color::DarkRed, 11 },
                                "Wood0");
    textel_presets.emplace_back(drawing::Textel { 'E', Color::DarkRed, Color::Yellow, 11 },
                                drawing::Textel { 'E', Color::Yellow, Color::DarkRed, 11 },
                                "Wood1");
    textel_presets.emplace_back(drawing::Textel { 'Z', Color::DarkRed, Color::Yellow, 11 },
                                drawing::Textel { 'Z', Color::Yellow, Color::DarkRed, 11 },
                                "Wood2");
    textel_presets.emplace_back(drawing::Textel { 'X', Color::DarkBlue, Color::Cyan, 12 },
                                drawing::Textel { 'X', Color::Cyan, Color::DarkBlue, 12 },
                                "Ice");
    textel_presets.emplace_back(drawing::Textel { '^', Color::DarkGray, Color::LightGray, 13 },
                                drawing::Textel { '^', Color::LightGray, Color::DarkGray, 13 },
                                "Mountain0");
    textel_presets.emplace_back(drawing::Textel { '^', Color::LightGray, Color::White, 13 },
                                drawing::Textel { '^', Color::DarkGray, Color::LightGray, 13 },
                                "Mountain1");
    textel_presets.emplace_back(drawing::Textel { 'W', Color::DarkRed, Color::Red, 14 },
                                drawing::Textel { 'W', Color::Red, Color::DarkRed, 14 },
                                "Lava");
    textel_presets.emplace_back(drawing::Textel { 'C', Color::DarkYellow, Color::Yellow, 15 },
                                drawing::Textel { 'C', Color::Yellow, Color::DarkYellow, 15 },
                                "Cave0");
    textel_presets.emplace_back(drawing::Textel { 'U', Color::DarkYellow, Color::Yellow, 15 },
                                drawing::Textel { 'U', Color::Yellow, Color::DarkYellow, 15 },
                                "Cave1");
    textel_presets.emplace_back(drawing::Textel { 'S', Color::DarkRed, Color::Green, 16 },
                                drawing::Textel { 'S', Color::Red, Color::DarkGreen, 16 },
                                "Swamp");
    textel_presets.emplace_back(drawing::Textel { '~', Color::DarkGreen, Color::Green, 17 },
                                drawing::Textel { '~', Color::Green, Color::DarkGreen, 17 },
                                "Poison0");
    textel_presets.emplace_back(drawing::Textel { 'o', Color::DarkGreen, Color::Green, 17 },
                                drawing::Textel { 'o', Color::Green, Color::DarkGreen, 17 },
                                "Poison1");
    textel_presets.emplace_back(drawing::Textel { '#', Color::DarkYellow, Color::Green, 18 },
                                drawing::Textel { '#', Color::Yellow, Color::DarkGreen, 18 },
                                "Path");
    textel_presets.emplace_back(drawing::Textel { 'M', Color::DarkGray, Color::LightGray, 19 },
                                drawing::Textel { 'M', Color::LightGray, Color::DarkGray, 19 },
                                "Mine");
    textel_presets.emplace_back(drawing::Textel { 'G', Color::DarkYellow, Color::Yellow, 20 },
                                drawing::Textel { 'G', Color::Yellow, Color::DarkYellow, 20 },
                                "Gold");
    textel_presets.emplace_back(drawing::Textel { 'S', Color::White, Color::LightGray, 21 },
                                drawing::Textel { 'S', Color::LightGray, Color::DarkGray, 21 },
                                "Silver");
    textel_presets.emplace_back(drawing::Textel { '.', Color::DarkGray, Color::LightGray, 22 },
                                drawing::Textel { '.', Color::LightGray, Color::DarkGray, 22 },
                                "Gravel0");
    textel_presets.emplace_back(drawing::Textel { ':', Color::DarkGray, Color::LightGray, 22 },
                                drawing::Textel { ':', Color::LightGray, Color::DarkGray, 22 },
                                "Gravel1");
    textel_presets.emplace_back(drawing::Textel { '.', Color::Black, Color::LightGray, 22 },
                                drawing::Textel { '.', Color::Black, Color::DarkGray, 22 },
                                "Gravel2");
    textel_presets.emplace_back(drawing::Textel { ':', Color::Black, Color::LightGray, 22 },
                                drawing::Textel { ':', Color::Black, Color::DarkGray, 22 },
                                "Gravel3");
    textel_presets.emplace_back(drawing::Textel { '@', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '@', Color::LightGray, Color::Black, 23 },
                                "Skull");
    textel_presets.emplace_back(drawing::Textel { '+', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '+', Color::LightGray, Color::Black, 23 },
                                "Bone0");
    textel_presets.emplace_back(drawing::Textel { '|', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '|', Color::LightGray, Color::Black, 23 },
                                "Bone1");
    textel_presets.emplace_back(drawing::Textel { '-', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '-', Color::LightGray, Color::Black, 23 },
                                "Bone2");
    textel_presets.emplace_back(drawing::Textel { '/', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '/', Color::LightGray, Color::Black, 23 },
                                "Bone3");
    textel_presets.emplace_back(drawing::Textel { '\\', Color::White, Color::DarkGray, 23 },
                                drawing::Textel { '\\', Color::LightGray, Color::Black, 23 },
                                "Bone4");
    
    std::vector<std::string> lines_custom_textel_presets;
    if (TextIO::read_file(folder::join_path({ get_exe_folder(), "custom_textel_presets" }), lines_custom_textel_presets))
    {
      int part = 0;
      drawing::Textel textel_normal, textel_shadow;
      for (const auto& line : lines_custom_textel_presets)
      {
        if (part == 0)
        {
          auto tokens = str::tokenize(line, { ' ', ',' }, { '\'' });
          if (tokens.size() == 4 && tokens[0].length() == 1)
          {
            textel_normal.ch = tokens[0][0];
            textel_normal.fg_color = color::string2color(tokens[1]);
            textel_normal.bg_color = color::string2color(tokens[2]);
            textel_normal.mat = std::atoi(tokens[3].c_str());
          }
          else
            std::cerr << "Unable to parse normal textel." << std::endl;
          part = 1;
        }
        else if (part == 1)
        {
          auto tokens = str::tokenize(line, { ' ', ',' }, { '\'' });
          if (tokens.size() == 4 && tokens[0].length() == 1)
          {
            textel_shadow.ch = tokens[0][0];
            textel_shadow.fg_color = color::string2color(tokens[1]);
            textel_shadow.bg_color = color::string2color(tokens[2]);
            textel_shadow.mat = std::atoi(tokens[3].c_str());
          }
          else
            std::cerr << "Unable to parse shadow textel." << std::endl;
          part = 2;
        }
        else if (part == 2)
        {
          textel_presets.emplace_back(textel_normal, textel_shadow, line);
          part = 0;
        }
      }
    }
                                
    if (convert)
    {
      bright_texture.load(file_path_bright_texture); // source
      curr_texture = drawing::Texture { bright_texture.size }; // target
      for (int r = 0; r < bright_texture.size.r; ++r)
        for (int c = 0; c < bright_texture.size.c; ++c)
        {
          const auto& curr_textel = bright_texture(r, c);
          auto it = stlutils::find_if(textel_presets, [&curr_textel](const auto& tp)
          {
            return tp.textel_normal.ch == curr_textel.ch
            && tp.textel_normal.fg_color == curr_textel.fg_color
            && tp.textel_normal.bg_color == curr_textel.bg_color;
          });
          if (it != textel_presets.end())
          {
            curr_texture.set_textel(r, c, it->textel_shadow);
          }
        }
      curr_texture.save(file_path_curr_texture);
      request_exit();
    }

    tbd.add(PARAM(screen_pos.r));
    tbd.add(PARAM(screen_pos.c));
    tbd.add(PARAM(cursor_pos.r));
    tbd.add(PARAM(cursor_pos.c));
    
    reset_goto_input();
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
    const int menu_width = 15;

//#define SHOW_DEBUG_WINDOW
#ifdef SHOW_DEBUG_WINDOW
    tbd.calc_pre_draw(str::Adjustment::Left);
    tbd.draw(sh, ui::VerticalAlignment::TOP, ui::HorizontalAlignment::CENTER, { Color::Blue, Color::Yellow }, true, true, 0, 0, std::nullopt, drawing::OutlineType::Line, true);
#endif
      
    if (!show_confirm_overwrite && show_menu)
      draw_box_outline(sh, 0, nc - menu_width, nr - 1, menu_width - 1, drawing::OutlineType::Line, ui_style);
  
    if (is_modified)
      sh.write_buffer("*", 0, 0, Color::Red, Color::White);
    draw_frame(sh, Color::White);
    
    if (show_confirm_overwrite)
    {
      bg_color = Color::DarkCyan;
      draw_confirm(sh, { "Are you sure you want to overwrite the file \"" + file_path_curr_texture + "\"?" },
                   overwrite_confirm_button,
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
      else if (show_goto_pos)
      {
        if ('0' <= kpd.curr_key && kpd.curr_key <= '9')
        {
          tb_goto[1][(goto_tab == 0 ? 0 : 10) + goto_caret_idx[goto_tab]] = kpd.curr_key;
          goto_caret_idx[goto_tab]++;
          if (goto_caret_idx[goto_tab] > 7)
            goto_caret_idx[goto_tab] = 7;
        }
        else if (kpd.curr_special_key == keyboard::SpecialKey::Backspace)
        {
          goto_caret_idx[goto_tab]--;
          if (goto_caret_idx[goto_tab] < 0)
            goto_caret_idx[goto_tab] = 0;
          tb_goto[1][(goto_tab == 0 ? 0 : 10) + goto_caret_idx[goto_tab]] = ' ';
        }
        else if (kpd.curr_special_key == keyboard::SpecialKey::Tab)
          goto_tab = 1 - goto_tab;
        else if (kpd.curr_special_key == keyboard::SpecialKey::Enter)
        {
          std::istringstream iss(tb_goto[1].substr(0, 8));
          RC pos;
          iss >> pos.r;
          iss.str(tb_goto[1].substr(10, 8));
          iss.clear();
          iss >> pos.c;
          if (math::in_range<int>(pos.r, 0, curr_texture.size.r, Range::ClosedOpen)
              && math::in_range<int>(pos.c, 0, curr_texture.size.c, Range::ClosedOpen))
          {
            cursor_pos = pos;
            screen_pos = { nr/2 - cursor_pos.r, nc/2 - cursor_pos.c };
          }
          reset_goto_input();
          show_goto_pos = false;
        }
        
        tb_goto.calc_pre_draw(str::Adjustment::Left);
        tb_goto.draw(sh, ui::VerticalAlignment::CENTER, ui::HorizontalAlignment::CENTER, { Color::White, Color::DarkBlue }, true, true, 0, 1, std::nullopt, drawing::OutlineType::Line, true);
      }
      
      message_handler->update(sh, static_cast<float>(sim_time_s));
      
      // Caret
      if (anim_ctr % 2 == 0 && (!show_menu || screen_pos.c + cursor_pos.c + 1 < nc - menu_width))
        sh.write_buffer("#", screen_pos.r + cursor_pos.r + 1, screen_pos.c + cursor_pos.c + 1, ui_style);
      
      draw_coord_sys(draw_vert_coords, draw_horiz_coords, nc, menu_width);
      
      int box_width_curr = curr_texture.size.c + 1;
      int box_width_tracing = tracing_texture.size.c + 1;
      if (show_menu)
      {
        if (curr_texture.size.c > nc - menu_width)
          box_width_curr = nc - menu_width - screen_pos.c + 1;
        if (tracing_texture.size.c > nc - menu_width)
          box_width_tracing = nc - menu_width - screen_pos.c + 1;
      }
      if (show_materials)
      {
        draw_box_texture_materials(sh,
                                   screen_pos.r, screen_pos.c,
                                   curr_texture.size.r+1, box_width_curr,
                                   curr_texture);
      }
      else
      {
        draw_box_textured(sh,
                          screen_pos.r, screen_pos.c,
                          curr_texture.size.r+1, box_width_curr,
                          drawing::Direction::None,
                          curr_texture);
      }
      if (show_tracing && (tracing_texture.size.r > 0 || tracing_texture.size.c > 0))
      {
        draw_box_textured(sh,
                          screen_pos.r, screen_pos.c,
                          tracing_texture.size.r+1, box_width_tracing,
                          drawing::Direction::None,
                          tracing_texture);
      }
    }
                      
    // Keypresses:
    if (kpd.curr_key == '-')
      math::toggle(show_menu);
      
    bool is_up = kpd.curr_special_key == keyboard::SpecialKey::Up || kpd.curr_key == 'w';
    bool is_down = kpd.curr_special_key == keyboard::SpecialKey::Down || kpd.curr_key == 's';
    bool is_left = kpd.curr_special_key == keyboard::SpecialKey::Left || kpd.curr_key == 'a';
    bool is_right = kpd.curr_special_key == keyboard::SpecialKey::Right || kpd.curr_key == 'd';
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
      else if (is_left)
      {
        int curr_mat = textel_presets[selected_textel_preset_idx].textel_normal.mat;
        int prev_mat_idx = stlutils::find_if_idx(textel_presets, [curr_mat](const auto& tp) { return tp.textel_normal.mat == curr_mat - 1; });
        if (0 <= prev_mat_idx)
          selected_textel_preset_idx = prev_mat_idx;
      }
      else if (is_right)
      {
        int curr_mat = textel_presets[selected_textel_preset_idx].textel_normal.mat;
        int prev_mat_idx = stlutils::find_if_idx(textel_presets, [curr_mat](const auto& tp) { return tp.textel_normal.mat == curr_mat + 1; });
        if (0 <= prev_mat_idx)
          selected_textel_preset_idx = prev_mat_idx;
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
      else if (kpd.curr_key == 'W')
      {
        cursor_pos.r -= nri;
        if (cursor_pos.r < 0)
          cursor_pos.r = 0;
        while (cursor_pos.r + screen_pos.r < 0)
          screen_pos.r++;
      }
      else if (kpd.curr_key == 'S')
      {
        cursor_pos.r += nri;
        if (cursor_pos.r >= static_cast<int>(curr_texture.size.r))
          cursor_pos.r = curr_texture.size.r - 1;
        while (cursor_pos.r + screen_pos.r >= nri)
          screen_pos.r--;
      }
      else if (kpd.curr_key == 'A')
      {
        cursor_pos.c -= nci;
        if (cursor_pos.c < 0)
          cursor_pos.c = 0;
        while (cursor_pos.c + screen_pos.c < 0)
          screen_pos.c++;
      }
      else if (kpd.curr_key == 'D')
      {
        cursor_pos.c += nci;
        if (cursor_pos.c >= static_cast<int>(curr_texture.size.c))
          cursor_pos.c = curr_texture.size.c - 1;
        while (cursor_pos.c + screen_pos.c >= nci)
          screen_pos.c--;
      }
      else if (kpd.curr_key == ' ')
      {
        undo_buffer.push({ cursor_pos, curr_texture(cursor_pos) });
        curr_texture.set_textel(cursor_pos, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
        redo_buffer = {};
        is_modified = true;
      }
      else if (kpd.curr_key == 'z')
      {
        if (!undo_buffer.empty())
        {
          auto& up = undo_buffer.top();
          redo_buffer.push({ up.first, curr_texture(up.first) });
          curr_texture.set_textel(up.first, up.second);
          undo_buffer.pop();
          is_modified = true;
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
          is_modified = true;
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
        is_modified = true;
      }
      else if (kpd.curr_key == 'b')
      {
        for (int i = -2; i <= 2; ++i)
        {
          int j_offs = std::abs(i) == 2 ? 2 : 4;
          for (int j = -j_offs; j <= j_offs; ++j)
          {
            RC offs { i, j };
            undo_buffer.push({ cursor_pos + offs, curr_texture(cursor_pos + offs) });
            curr_texture.set_textel(cursor_pos + offs, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
          }
        }
        redo_buffer = {};
        is_modified = true;
      }
      else if (kpd.curr_key == 'B')
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
            RC offs { i, j };
            undo_buffer.push({ cursor_pos + offs, curr_texture(cursor_pos + offs) });
            curr_texture.set_textel(cursor_pos + offs, textel_presets[selected_textel_preset_idx].get_textel(use_shadow_textels));
          }
        }
        redo_buffer = {};
        is_modified = true;
      }
      else if (str::to_lower(kpd.curr_key) == 'l')
        message_handler->add_message(static_cast<float>(sim_time_s),
                                     "Cursor @ " + cursor_pos.str(),
                                     MessageHandler::Level::Guide);
      else if (str::to_lower(kpd.curr_key) == 'g')
      {
        if (!math::toggle(show_goto_pos))
          reset_goto_input();
      }
      else if (str::to_lower(kpd.curr_key) == 't')
        math::toggle(show_tracing);
      else if (str::to_lower(kpd.curr_key) == 'm')
        math::toggle(show_materials);
    }
    
    if (str::to_lower(kpd.curr_key) == 'i')
        math::toggle(use_shadow_textels);
    else if (str::to_lower(kpd.curr_key) == 'x' || safe_to_save)
    {
      if (file_mode == EditorFileMode::NEW_OR_OVERWRITE_FILE)
      {
        if (folder::exists(file_path_curr_texture))
        {
          show_confirm_overwrite = true;
          overwrite_confirm_button = YesNoButtons::No;
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
          message_handler->add_message(static_cast<float>(sim_time_s),
                                       "Your work was successfully saved.",
                                       MessageHandler::Level::Guide);
                                       
          is_modified = false;
        }
        else
          message_handler->add_message(static_cast<float>(sim_time_s),
                                       "An error occurred while saving your work!",
                                       MessageHandler::Level::Fatal);
                                       
        safe_to_save = false;
        show_confirm_overwrite = false;
      }
    }
    
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
    
  std::vector<ASCII_Fonts::ColorScheme> color_schemes;
  
  std::string font_data_path;
  
  ASCII_Fonts::FontDataColl font_data;
  
  drawing::Texture curr_texture;
  drawing::Texture tracing_texture;
  drawing::Texture bright_texture;
  std::string file_path_curr_texture;
  std::string file_path_tracing_texture;
  std::string file_path_bright_texture;
  bool convert = false;
  EditorFileMode file_mode = EditorFileMode::OPEN_EXISTING_FILE;
  
  RC screen_pos { 0, 0 };
  RC cursor_pos { 0, 0 };
  int menu_r_offs = 0;
  
  bool show_menu = false;
  bool show_confirm_overwrite = false;
  bool show_tracing = true;
  bool show_goto_pos = false;
  bool show_materials = false;
  
  YesNoButtons overwrite_confirm_button = YesNoButtons::No;
  bool safe_to_save = false;
  
  std::vector<TextelItem> textel_presets;
  int selected_textel_preset_idx = 0;
  
  std::unique_ptr<MessageHandler> message_handler;
  std::stack<std::pair<RC, drawing::Textel>> undo_buffer;
  std::stack<std::pair<RC, drawing::Textel>> redo_buffer;
  bool is_modified = false;
  
  bool draw_vert_coords = true;
  bool draw_horiz_coords = true;
  
  bool use_shadow_textels = false;
  
  ui::TextBoxDebug tbd;
  
  ui::TextBox tb_goto;
  std::array<int, 2> goto_caret_idx { 0, 0 };
  int goto_tab = 0;
};

int main(int argc, char** argv)
{
  GameEngineParams params;
  params.enable_title_screen = false;
  params.enable_instructions_screen = false;
  params.enable_quit_confirm_screen = false;
  params.quit_confirm_unsaved_changes = true;
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
