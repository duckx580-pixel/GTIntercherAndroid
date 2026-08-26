#include "ui.h"
#include <cstring>

namespace ui {

// ── Dark-purple "The Rare Owl" palette ─────────────────────────────────────
static constexpr ImVec4 kBg           { 0.05f, 0.04f, 0.10f, 0.97f };
static constexpr ImVec4 kTitleBg      { 0.18f, 0.06f, 0.35f, 1.00f };
static constexpr ImVec4 kTitleBgAct   { 0.22f, 0.08f, 0.42f, 1.00f };
static constexpr ImVec4 kText         { 0.93f, 0.84f, 1.00f, 1.00f };
static constexpr ImVec4 kTextDim      { 0.58f, 0.48f, 0.70f, 1.00f };
static constexpr ImVec4 kFrameBg      { 0.12f, 0.07f, 0.22f, 1.00f };
static constexpr ImVec4 kFrameHov     { 0.18f, 0.10f, 0.32f, 1.00f };
static constexpr ImVec4 kFrameAct     { 0.24f, 0.13f, 0.40f, 1.00f };
static constexpr ImVec4 kAccent       { 0.42f, 0.13f, 0.66f, 1.00f };
static constexpr ImVec4 kAccentHov    { 0.54f, 0.23f, 0.78f, 1.00f };
static constexpr ImVec4 kAccentAct    { 0.31f, 0.09f, 0.49f, 1.00f };
static constexpr ImVec4 kCheckMark    { 0.72f, 0.42f, 1.00f, 1.00f };
static constexpr ImVec4 kSliderGrab   { 0.52f, 0.20f, 0.82f, 1.00f };
static constexpr ImVec4 kSliderGrabH  { 0.64f, 0.30f, 0.94f, 1.00f };
static constexpr ImVec4 kBtn          { 0.25f, 0.10f, 0.45f, 1.00f };
static constexpr ImVec4 kBtnHov       { 0.40f, 0.15f, 0.65f, 1.00f };
static constexpr ImVec4 kBtnAct       { 0.31f, 0.09f, 0.49f, 1.00f };
static constexpr ImVec4 kTabBg        { 0.10f, 0.04f, 0.20f, 1.00f };
static constexpr ImVec4 kTabHov       { 0.38f, 0.14f, 0.57f, 1.00f };
static constexpr ImVec4 kTabAct       { 0.49f, 0.21f, 0.71f, 1.00f };
static constexpr ImVec4 kTabUnfocAct  { 0.28f, 0.10f, 0.46f, 1.00f };
static constexpr ImVec4 kSep          { 0.40f, 0.15f, 0.65f, 0.60f };
static constexpr ImVec4 kSepHov       { 0.54f, 0.23f, 0.78f, 0.80f };
static constexpr ImVec4 kScrollBg     { 0.08f, 0.04f, 0.15f, 1.00f };
static constexpr ImVec4 kScrollGrab   { 0.42f, 0.13f, 0.66f, 0.80f };
static constexpr ImVec4 kScrollGrabH  { 0.54f, 0.23f, 0.78f, 0.90f };
static constexpr ImVec4 kHeader       { 0.30f, 0.10f, 0.50f, 0.80f };
static constexpr ImVec4 kHeaderHov    { 0.40f, 0.14f, 0.62f, 0.90f };
static constexpr ImVec4 kHeaderAct    { 0.49f, 0.21f, 0.71f, 1.00f };
// ───────────────────────────────────────────────────────────────────────────

Ui::Ui(ImVec2 display_size)
    : ImGuiWrapper(display_size)
    , m_clear_pos(true) {}

void Ui::push_theme()
{
    float sx = m_display_scale.x; // ~1.0 on a 1920 px-wide display
    float sy = m_display_scale.y; // ~1.0 on a 1080 px-tall display

    // 11 style vars ──────────────────────────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,    10.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,      7.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,      6.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,      7.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding,  8.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,       6.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding,        6.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,    0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,     ImVec2(8.0f * sx, 6.0f * sy));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(8.0f * sx, 5.0f * sy));

    // 32 colors ──────────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_WindowBg,             kBg);
    ImGui::PushStyleColor(ImGuiCol_TitleBg,              kTitleBg);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,        kTitleBgAct);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed,     kTitleBg);
    ImGui::PushStyleColor(ImGuiCol_Text,                 kText);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled,         kTextDim);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,              kFrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,       kFrameHov);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,        kFrameAct);
    ImGui::PushStyleColor(ImGuiCol_CheckMark,            kCheckMark);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,           kSliderGrab);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,     kSliderGrabH);
    ImGui::PushStyleColor(ImGuiCol_Button,               kBtn);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,        kBtnHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,         kBtnAct);
    ImGui::PushStyleColor(ImGuiCol_Tab,                  kTabBg);
    ImGui::PushStyleColor(ImGuiCol_TabHovered,           kTabHov);
    ImGui::PushStyleColor(ImGuiCol_TabActive,            kTabAct);
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive,   kTabUnfocAct);
    ImGui::PushStyleColor(ImGuiCol_Separator,            kSep);
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered,     kSepHov);
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive,      kAccent);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          kScrollBg);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        kScrollGrab);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, kScrollGrabH);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,  kAccent);
    ImGui::PushStyleColor(ImGuiCol_Header,               kHeader);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        kHeaderHov);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,         kHeaderAct);
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip,           kAccent);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered,    kAccentHov);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive,     kAccentAct);
}

void Ui::pop_theme()
{
    ImGui::PopStyleColor(32);
    ImGui::PopStyleVar(11);
}

void Ui::render()
{
    ImGuiWrapper::render();

    if (m_clear_pos) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        m_clear_pos = false;
    }
}

// ── Full-width rounded toggle card ─────────────────────────────────────────
// Each card manages its own push/pop so callers need no bookkeeping.
static void card_toggle(const char* label, const char* desc, bool* value,
                        float sx, float sy)
{
    ImGui::PushID(label);

    bool  has_desc = (desc && desc[0]);
    float card_h   = (has_desc ? 52.0f : 36.0f) * sy;
    float pad_x    = 10.0f * sx;
    float pad_y    =  7.0f * sy;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.07f, 0.22f, 0.70f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f * sx);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad_x, pad_y));

    if (ImGui::BeginChild("##card", ImVec2(0.0f, card_h), false)) {
        ImGui::TextUnformatted(label);
        if (has_desc) {
            ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
            ImGui::TextUnformatted(desc);
            ImGui::PopStyleColor();
        }

        // Checkbox right-edge, vertically centred in the card
        float cb_sz = ImGui::GetFrameHeight();
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - pad_x - cb_sz,
                                   (card_h - cb_sz) * 0.5f));
        ImGui::Checkbox("##v", value);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::PopID();
}
// ───────────────────────────────────────────────────────────────────────────

void Ui::draw()
{
    ImGuiIO& io = ImGui::GetIO();

    push_theme();

    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.34f,
                                    io.DisplaySize.y * 0.56f), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.04f,
                                   io.DisplaySize.y * 0.08f),  ImGuiCond_Once);

    constexpr ImGuiWindowFlags kWinFlags =
        ImGuiWindowFlags_NoCollapse      |
        ImGuiWindowFlags_NoScrollbar     |
        ImGuiWindowFlags_NoScrollWithMouse;

    // Window title: UTF-8 crescent moon + version badge
    if (ImGui::Begin("\xe2\x98\xbd The Rare Owl  \xe2\x80\xa2  v5.54", nullptr, kWinFlags)) {
        ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
        ImGui::TextUnformatted("Growtopia v5.54  |  arm64-v8a / armeabi-v7a");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("##tabs")) {
            if (ImGui::BeginTabItem("  Cheats  ")) { draw_cheats_tab(); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("  Bot  "))    { draw_bot_tab();    ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("  Lua  "))    { draw_lua_tab();    ImGui::EndTabItem(); }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    pop_theme();
}

void Ui::draw_cheats_tab()
{
    float sx = m_display_scale.x;
    float sy = m_display_scale.y;

    card_toggle("Show FPS",        "Display frame-rate counter", &cheats.fps_visible,  sx, sy);
    card_toggle("Console Visible", "Show developer console",     &cheats.console_vis,  sx, sy);
    card_toggle("No-Clip",         "Walk through solid blocks",  &cheats.no_clip,      sx, sy);
    card_toggle("Auto Respawn",    "Instant respawn on death",   &cheats.auto_respawn, sx, sy);

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Speed Multiplier");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("##speed", &cheats.speed_mult,  1.0f, 10.0f, "%.1f x");

    ImGui::Spacing();
    ImGui::TextUnformatted("Punch Range");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("##punch", &cheats.punch_range, 1.0f,  8.0f, "%.1f tiles");
}

void Ui::draw_bot_tab()
{
    float sx = m_display_scale.x;
    float sy = m_display_scale.y;

    card_toggle("Auto Farm",    "Break & collect in sequence", &bot.auto_farm,    sx, sy);
    card_toggle("Auto Collect", "Pick up nearby item drops",   &bot.auto_collect, sx, sy);
    card_toggle("Auto Break",   "Break blocks within range",   &bot.auto_break,   sx, sy);
    card_toggle("Auto Plant",   "Plant seeds automatically",   &bot.auto_plant,   sx, sy);

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Walk Speed");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderFloat("##wsp", &bot.walk_speed, 1.0f, 5.0f, "%.1f x");

    ImGui::Spacing();
    ImGui::TextUnformatted("Target World");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##world", bot.target_world, sizeof(bot.target_world));
}

void Ui::draw_lua_tab()
{
    float sx      = m_display_scale.x;
    float sy      = m_display_scale.y;
    float avail_h = ImGui::GetContentRegionAvail().y;
    float btn_h   = ImGui::GetFrameHeight() + 4.0f * sy;
    float lbl_h   = ImGui::GetTextLineHeight() + 4.0f * sy;
    float out_h   = avail_h * 0.28f;
    float edit_h  = avail_h - btn_h - lbl_h - out_h - 18.0f * sy;

    // Script editor
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.04f, 0.14f, 1.0f));
    ImGui::InputTextMultiline("##lua", lua.script, sizeof(lua.script),
        ImVec2(-1.0f, edit_h), ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Execute / Clear
    float btn_w = (ImGui::GetContentRegionAvail().x - 6.0f * sx) * 0.5f;
    if (ImGui::Button("Execute", ImVec2(btn_w, 0.0f))) {
        lua.output += "> Executed script\n";
        lua.scroll_to_bottom = true;
    }
    ImGui::SameLine(0.0f, 6.0f * sx);
    if (ImGui::Button("Clear", ImVec2(btn_w, 0.0f))) {
        std::memset(lua.script, 0, sizeof(lua.script));
        lua.output.clear();
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kTextDim);
    ImGui::TextUnformatted("Output");
    ImGui::PopStyleColor();

    // Scrolling output console
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.02f, 0.08f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f * sx);
    if (ImGui::BeginChild("##out", ImVec2(-1.0f, out_h), false,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.95f, 0.65f, 1.0f));
        ImGui::TextUnformatted(lua.output.empty() ? "-- no output --"
                                                  : lua.output.c_str());
        ImGui::PopStyleColor();
        if (lua.scroll_to_bottom) {
            ImGui::SetScrollHereY(1.0f);
            lua.scroll_to_bottom = false;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void Ui::on_touch(int type, bool multi, float x, float y)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (type) {
    case 1:
        io.MouseDown[0] = false;
        m_clear_pos = true;
        break;
    case 2:
        io.MousePos     = ImVec2(x, y);
        io.MouseDown[0] = true;
        break;
    case 3:
        io.MousePos = ImVec2(x, y);
        break;
    default:
        break;
    }
}

} // ui
