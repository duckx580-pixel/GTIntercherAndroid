#pragma once
#include <string>
#include "imgui_wrapper.h"

namespace ui {

struct CheatState {
    bool  fps_visible  = false;
    bool  console_vis  = false;
    bool  no_clip      = false;
    bool  auto_respawn = false;
    float speed_mult   = 1.0f;
    float punch_range  = 1.0f;
};

struct BotState {
    bool  auto_farm    = false;
    bool  auto_collect = false;
    bool  auto_break   = false;
    bool  auto_plant   = false;
    float walk_speed   = 1.0f;
    char  target_world[32] = {};
};

struct LuaState {
    char        script[4096]     = {};
    std::string output;
    bool        scroll_to_bottom = false;
};

class Ui : public ImGuiWrapper {
public:
    Ui(ImVec2 display_size);
    ~Ui() = default;

    void render() override;
    void draw() override;

    void on_touch(int type, bool multi, float x, float y);

    CheatState cheats{};
    BotState   bot{};
    LuaState   lua{};

private:
    bool m_clear_pos;

    void push_theme();
    void pop_theme();
    void draw_cheats_tab();
    void draw_bot_tab();
    void draw_lua_tab();
};

} // ui
