#pragma once
#include <deque>
#include <string>

#include "imgui_wrapper.h"

namespace ui {

// A repeating action the menu drives from the render loop.
struct Repeater {
    bool enabled{ false };
    float interval{ 1.0f }; // seconds
    float elapsed{ 0.0f };

    // Returns true once per interval while enabled.
    bool tick(float delta);
};

class Ui : public ImGuiWrapper {
public:
    Ui(ImVec2 display_size);
    ~Ui() override = default;

    void render() override;
    void draw() override;

    // `action` is an Android MotionEvent action, `finger` the pointer id.
    void on_touch(int action, int finger, float x, float y);

    // True while the menu wants the touch for itself, so the hook can keep it
    // from also reaching the game.
    bool wants_input() const;

private:
    void draw_toggle_button();
    void draw_menu();
    void draw_tab_commands();
    void draw_tab_automation();
    void draw_tab_info();
    void draw_tab_status();

    void log(const std::string& line);
    void run_automation(float delta);
    void apply_theme();

    bool m_open{ true };
    bool m_clear_pos{ true };

    // Commands tab
    char m_command[256]{};
    char m_quick[4][64]{};

    // Automation tab
    Repeater m_auto_command{};
    char m_auto_command_text[256]{};
    Repeater m_auto_tap{};
    float m_auto_tap_pos[2]{ 0.0f, 0.0f };
    bool m_picking_tap_point{ false };

    // Info tab
    float m_last_touch[2]{ 0.0f, 0.0f };
    int m_last_action{ -1 };

    std::deque<std::string> m_log;
};
} // ui
