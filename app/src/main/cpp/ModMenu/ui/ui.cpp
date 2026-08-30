#include "ui.h"

namespace {
// android.view.MotionEvent action codes. SharedMultiTouchInput normalizes
// ACTION_POINTER_DOWN/ACTION_POINTER_UP down to these before calling native.
constexpr int kActionDown = 0;
constexpr int kActionUp = 1;
constexpr int kActionMove = 2;
constexpr int kActionCancel = 3;
} // namespace

namespace ui {
Ui::Ui(ImVec2 display_size)
    : ImGuiWrapper(display_size)
    , m_clear_pos(true) {}

void Ui::render()
{
    ImGuiWrapper::render();

    // Park the cursor offscreen after a release so nothing stays hovered until
    // the next touch lands.
    if (m_clear_pos) {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        m_clear_pos = false;
    }
}

void Ui::draw()
{
    static bool open = true;
    ImGui::ShowDemoWindow(&open);
}

void Ui::on_touch(int action, int finger, float x, float y) {
    // Only the primary pointer drives the ImGui cursor.
    if (finger != 0) {
        return;
    }

    ImGuiIO &io = ImGui::GetIO();
    switch (action) {
    case kActionDown:
        io.MousePos = ImVec2(x, y);
        io.MouseDown[0] = true;
        break;
    case kActionUp:
    case kActionCancel:
        io.MouseDown[0] = false;
        m_clear_pos = true;
        break;
    case kActionMove:
        io.MousePos = ImVec2(x, y);
        break;
    default:
        break;
    }
}
} // ui
