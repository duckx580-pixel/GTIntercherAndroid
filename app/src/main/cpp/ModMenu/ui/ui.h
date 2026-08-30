#pragma once
#include "imgui_wrapper.h"

namespace ui {
class Ui : public ImGuiWrapper {
public:
    Ui(ImVec2 display_size);
    ~Ui() override = default;

    void render() override;
    void draw() override;

    // `action` is an Android MotionEvent action code, `finger` the pointer id
    // assigned by SharedMultiTouchInput (0 on the single-touch path).
    void on_touch(int action, int finger, float x, float y);

private:
    bool m_clear_pos;
};
} // ui
