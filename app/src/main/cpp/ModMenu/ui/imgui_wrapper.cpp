#include <chrono>

#include "imgui_wrapper.h"

namespace {
double now_seconds()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
} // namespace

namespace ui {
ImGuiWrapper::ImGuiWrapper(ImVec2 display_size)
    : m_display_size(display_size)
    , m_last_frame_time(0.0)
{
    // Scale relative to a 1920x1080 reference.
    m_display_scale = ImVec2(m_display_size.x * 0.00052083333f, m_display_size.y * 0.00092592592f);
}

ImGuiWrapper::~ImGuiWrapper()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::set_display_size(ImVec2 display_size)
{
    m_display_size = display_size;
}

bool ImGuiWrapper::init()
{
    // Setup Dear Gui context
    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext()) {
        return false;
    }

    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = m_display_size;

    // There is no filesystem-backed location to persist layout to inside the
    // host app, and letting ImGui write imgui.ini into the CWD fails silently.
    io.IniFilename = nullptr;

    // Setup Renderer backends
    if (!ImGui_ImplOpenGL3_Init()) {
        ImGui::DestroyContext();
        return false;
    }

    // Setup Font
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 14.0f * (m_display_scale.x + m_display_scale.y);
    io.Fonts->AddFontDefault(&font_cfg);

    // Scale All Widgets Size
    ImGui::GetStyle().ScaleAllSizes(m_display_scale.x + m_display_scale.y);
    return true;
}

void ImGuiWrapper::render() {
    ImGuiIO &io = ImGui::GetIO();

    // Track the live viewport so rotation and resizes are handled.
    io.DisplaySize = m_display_size;

    double now = now_seconds();
    if (m_last_frame_time > 0.0) {
        io.DeltaTime = static_cast<float>(now - m_last_frame_time);
    }
    // ImGui requires a strictly positive delta.
    if (io.DeltaTime <= 0.0f) {
        io.DeltaTime = 1.0f / 60.0f;
    }
    m_last_frame_time = now;

    // Start the Dear Gui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    draw();

    // Rendering
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // ui
