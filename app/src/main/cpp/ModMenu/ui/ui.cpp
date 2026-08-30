#include <cstdio>
#include <cstring>

#include "ui.h"
#include "../game/game_api.h"

namespace {
// android.view.MotionEvent action codes. SharedMultiTouchInput normalizes
// ACTION_POINTER_DOWN/UP down to these before calling native.
constexpr int kActionDown = 0;
constexpr int kActionUp = 1;
constexpr int kActionMove = 2;
constexpr int kActionCancel = 3;

constexpr std::size_t kMaxLogLines = 64;

// ImGui::SeparatorText landed in 1.89.3; the vendored copy is 1.89.2 WIP.
void section(const char* label)
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("%s", label);
    ImGui::Spacing();
}

void status_label(bool available, const char* name)
{
    if (available) {
        ImGui::TextColored(ImVec4(0.31f, 0.78f, 0.47f, 1.0f), "available");
    } else {
        ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.35f, 1.0f), "unavailable");
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(name);
}
} // namespace

namespace ui {

bool Repeater::tick(float delta)
{
    if (!enabled) {
        elapsed = 0.0f;
        return false;
    }

    elapsed += delta;
    if (elapsed < interval) {
        return false;
    }

    elapsed = 0.0f;
    return true;
}

Ui::Ui(ImVec2 display_size)
    : ImGuiWrapper(display_size)
{
    std::snprintf(m_quick[0], sizeof(m_quick[0]), "/help");
    std::snprintf(m_quick[1], sizeof(m_quick[1]), "/who");
    std::snprintf(m_quick[2], sizeof(m_quick[2]), "/time");
    std::snprintf(m_quick[3], sizeof(m_quick[3]), "/friends");
}

bool Ui::wants_input() const
{
    if (ImGui::GetCurrentContext() == nullptr) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

void Ui::log(const std::string& line)
{
    m_log.push_back(line);
    while (m_log.size() > kMaxLogLines) {
        m_log.pop_front();
    }
}

void Ui::render()
{
    ImGuiWrapper::render();

    // Park the cursor offscreen after a release so nothing stays hovered until
    // the next touch lands.
    if (m_clear_pos) {
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        m_clear_pos = false;
    }
}

void Ui::apply_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.94f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.07f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.16f, 0.68f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.16f, 0.32f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.29f, 0.16f, 0.68f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.39f, 0.23f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.30f, 0.93f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.29f, 0.16f, 0.68f, 0.70f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.39f, 0.23f, 0.85f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.11f, 0.28f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.39f, 0.23f, 0.85f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.29f, 0.16f, 0.68f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.55f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.30f, 0.93f, 1.00f);
}

void Ui::draw()
{
    static bool themed = false;
    if (!themed) {
        apply_theme();
        themed = true;
    }

    run_automation(ImGui::GetIO().DeltaTime);

    draw_toggle_button();

    if (m_open) {
        draw_menu();
    }
}

void Ui::draw_toggle_button()
{
    const ImVec2 size(ImGui::GetFontSize() * 4.5f, ImGui::GetFontSize() * 2.0f);

    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("##gtl_toggle", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button(m_open ? "Hide" : "GTL", size)) {
        m_open = !m_open;
    }

    ImGui::End();
}

void Ui::draw_menu()
{
    ImGui::SetNextWindowSize(ImVec2(m_display_size.x * 0.55f, m_display_size.y * 0.62f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(m_display_size.x * 0.06f, m_display_size.y * 0.14f),
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("GTIntercher  |  Growtopia 5.55", &m_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("##gtl_tabs")) {
        if (ImGui::BeginTabItem("Commands")) {
            draw_tab_commands();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Automation")) {
            draw_tab_automation();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Info")) {
            draw_tab_info();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Status")) {
            draw_tab_status();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void Ui::draw_tab_commands()
{
    const bool can_send = game::api::capabilities().send_text;

    if (!can_send) {
        ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.35f, 1.0f),
            "nativeOnInputText/nativeOnKey unavailable on this build.");
        ImGui::Separator();
    }

    ImGui::TextUnformatted("Send chat message or command");
    ImGui::SetNextItemWidth(-1.0f);
    const bool entered = ImGui::InputText("##gtl_cmd", m_command, sizeof(m_command),
        ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::BeginDisabled(!can_send);
    const bool clicked = ImGui::Button("Send");
    ImGui::EndDisabled();

    if ((entered || clicked) && can_send && m_command[0] != '\0') {
        if (game::api::send_text(m_command)) {
            log(std::string("sent: ") + m_command);
        } else {
            log("send failed (no JNI env this frame)");
        }
        m_command[0] = '\0';
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear log")) {
        m_log.clear();
    }

    section("Quick commands");
    for (int i = 0; i < 4; ++i) {
        if (i != 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(i);
        ImGui::BeginDisabled(!can_send);
        if (ImGui::Button(m_quick[i])) {
            if (game::api::send_text(m_quick[i])) {
                log(std::string("sent: ") + m_quick[i]);
            }
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    section("Edit quick slots");
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(100 + i);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##slot", m_quick[i], sizeof(m_quick[i]));
        ImGui::PopID();
    }

    section("Activity");
    ImGui::BeginChild("##gtl_log", ImVec2(0, ImGui::GetFontSize() * 6.0f), true);
    for (const std::string& line : m_log) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

void Ui::draw_tab_automation()
{
    const game::api::Capabilities& caps = game::api::capabilities();

    section("Auto command");
    ImGui::BeginDisabled(!caps.send_text);
    ImGui::Checkbox("Repeat command", &m_auto_command.enabled);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##gtl_autocmd", m_auto_command_text, sizeof(m_auto_command_text));
    ImGui::SliderFloat("Interval (s)##cmd", &m_auto_command.interval, 0.5f, 30.0f, "%.1f");
    ImGui::EndDisabled();
    if (!caps.send_text) {
        ImGui::TextDisabled("Requires nativeOnInputText.");
    }

    section("Auto tap");
    ImGui::BeginDisabled(!caps.touch);
    ImGui::Checkbox("Repeat tap", &m_auto_tap.enabled);
    ImGui::SliderFloat("Interval (s)##tap", &m_auto_tap.interval, 0.05f, 5.0f, "%.2f");
    ImGui::SliderFloat2("Position", m_auto_tap_pos, 0.0f,
        m_display_size.x > m_display_size.y ? m_display_size.x : m_display_size.y, "%.0f");

    if (ImGui::Button(m_picking_tap_point ? "Tap the screen..." : "Pick point")) {
        m_picking_tap_point = true;
        log("pick mode: next touch sets the auto-tap point");
    }
    ImGui::SameLine();
    if (ImGui::Button("Tap once")) {
        if (game::api::inject_touch(kActionDown, m_auto_tap_pos[0], m_auto_tap_pos[1], 0)
            && game::api::inject_touch(kActionUp, m_auto_tap_pos[0], m_auto_tap_pos[1], 0)) {
            log("injected tap");
        } else {
            log("tap injection failed");
        }
    }
    ImGui::EndDisabled();
    if (!caps.touch) {
        ImGui::TextDisabled("Requires the nativeOnTouch hook to be installed.");
    }

    ImGui::Separator();
    ImGui::TextWrapped(
        "Injected taps bypass the menu's own touch hook, so they reach the game "
        "directly and will not re-trigger the menu.");
}

void Ui::draw_tab_info()
{
    const ImGuiIO& io = ImGui::GetIO();

    section("Frame");
    ImGui::Text("FPS        : %.1f", io.Framerate);
    ImGui::Text("Frame time : %.2f ms", io.DeltaTime * 1000.0f);

    section("Display");
    ImGui::Text("GL viewport : %.0f x %.0f", m_display_size.x, m_display_size.y);

    float w = 0.0f;
    float h = 0.0f;
    if (game::api::screen_size(w, h)) {
        ImGui::Text("Game screen : %.0f x %.0f", w, h);
    } else {
        ImGui::TextDisabled("Game screen : unavailable");
    }

    section("Input");
    if (m_last_action >= 0) {
        const char* name = "?";
        switch (m_last_action) {
        case kActionDown: name = "DOWN"; break;
        case kActionUp: name = "UP"; break;
        case kActionMove: name = "MOVE"; break;
        case kActionCancel: name = "CANCEL"; break;
        default: break;
        }
        ImGui::Text("Last touch : %s at %.0f, %.0f", name, m_last_touch[0], m_last_touch[1]);
    } else {
        ImGui::TextDisabled("Last touch : none yet");
    }
    ImGui::Text("ImGui wants mouse : %s", io.WantCaptureMouse ? "yes" : "no");
}

void Ui::draw_tab_status()
{
    ImGui::TextWrapped(
        "Growtopia 5.55 ships a stripped libgrowtopia.so, so only its exported "
        "JNI entry points can be reached. Anything listed unavailable is absent "
        "from this build of the game.");
    ImGui::Separator();

    const game::api::Capabilities& caps = game::api::capabilities();
    status_label(caps.send_text, "nativeOnInputText + nativeOnKey  (send chat/commands)");
    status_label(caps.key, "nativeOnKey  (key events)");
    status_label(caps.touch, "nativeOnTouch  (tap injection)");
    status_label(caps.screen_size, "nativeGetScreenWidth/Height");
    status_label(caps.gui_event, "nativeSendGUIEx");
    status_label(caps.cancel_button, "nativeCancelBtnPressed");

    ImGui::Separator();
    ImGui::Text("JNI env this frame : %s", game::api::has_frame_env() ? "yes" : "no");

    ImGui::Separator();
    ImGui::TextWrapped(
        "Packet interception is not available: SendPacket is an internal engine "
        "symbol and was stripped, so it cannot be resolved by name.");
}

void Ui::run_automation(float delta)
{
    if (m_auto_command.tick(delta) && m_auto_command_text[0] != '\0') {
        if (game::api::send_text(m_auto_command_text)) {
            log(std::string("auto: ") + m_auto_command_text);
        }
    }

    if (m_auto_tap.tick(delta)) {
        game::api::inject_touch(kActionDown, m_auto_tap_pos[0], m_auto_tap_pos[1], 0);
        game::api::inject_touch(kActionUp, m_auto_tap_pos[0], m_auto_tap_pos[1], 0);
    }
}

void Ui::on_touch(int action, int finger, float x, float y) {
    // Only the primary pointer drives the ImGui cursor.
    if (finger != 0) {
        return;
    }

    m_last_action = action;
    m_last_touch[0] = x;
    m_last_touch[1] = y;

    if (m_picking_tap_point && action == kActionDown) {
        m_auto_tap_pos[0] = x;
        m_auto_tap_pos[1] = y;
        m_picking_tap_point = false;
        log("auto-tap point set");
    }

    ImGuiIO &io = ImGui::GetIO();
    switch (action) {
    case kActionDown:
        io.AddMousePosEvent(x, y);
        io.AddMouseButtonEvent(0, true);
        break;
    case kActionUp:
    case kActionCancel:
        io.AddMouseButtonEvent(0, false);
        m_clear_pos = true;
        break;
    case kActionMove:
        io.AddMousePosEvent(x, y);
        break;
    default:
        break;
    }
}
} // ui
