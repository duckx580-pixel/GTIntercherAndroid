#include <chrono>

#include "imgui_wrapper.h"
#include "../utils/log.h"

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
    // This whole function runs on the GL thread, on its very first frame.
    // A freeze observed on real test runs shows that thread permanently
    // stuck deep inside libhoudini.so (MEmu's ARM-to-x86 translator) with no
    // Java frames and no resolvable symbols at all -- i.e. Houdini itself
    // hung translating/running something in here, or in render() below, and
    // never returned. There is no way to see past that boundary from an ANR
    // trace or a debugger, so these LOGI calls exist purely to bracket every
    // step: whichever "... done" line is missing on the next freeze is the
    // one Houdini choked on.
    LOGI("ImGuiWrapper::init: ImGui::CreateContext");
    IMGUI_CHECKVERSION();
    if (!ImGui::CreateContext()) {
        return false;
    }
    LOGI("ImGuiWrapper::init: ImGui::CreateContext done");

    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = m_display_size;

    // There is no filesystem-backed location to persist layout to inside the
    // host app, and letting ImGui write imgui.ini into the CWD fails silently.
    io.IniFilename = nullptr;

    // Setup Renderer backends
    LOGI("ImGuiWrapper::init: ImGui_ImplOpenGL3_Init");
    if (!ImGui_ImplOpenGL3_Init()) {
        ImGui::DestroyContext();
        return false;
    }
    LOGI("ImGuiWrapper::init: ImGui_ImplOpenGL3_Init done");

    // Setup Font
    LOGI("ImGuiWrapper::init: AddFontDefault");
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 14.0f * (m_display_scale.x + m_display_scale.y);
    io.Fonts->AddFontDefault(&font_cfg);
    LOGI("ImGuiWrapper::init: AddFontDefault done");

    // Scale All Widgets Size
    ImGui::GetStyle().ScaleAllSizes(m_display_scale.x + m_display_scale.y);
    LOGI("ImGuiWrapper::init: complete");
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

    // Start the Dear Gui frame.
    //
    // ImGui_ImplOpenGL3_NewFrame() lazily creates the backend's GL device
    // objects (shader program, font texture) on its first call, which was
    // one of the two most likely places for the render-thread freeze
    // described above -- see the comment in ImGuiWrapper::init(). Logging
    // is kept here (not just in init()) because that first call happens
    // from inside this function, not from init() itself.
    static bool first_frame = true;
    if (first_frame) LOGI("ImGuiWrapper::render: first ImGui_ImplOpenGL3_NewFrame (creates GL device objects)");
    ImGui_ImplOpenGL3_NewFrame();
    if (first_frame) LOGI("ImGuiWrapper::render: first ImGui_ImplOpenGL3_NewFrame done");

    ImGui::NewFrame();

    draw();

    // Rendering
    ImGui::EndFrame();
    ImGui::Render();
    if (first_frame) LOGI("ImGuiWrapper::render: first ImGui_ImplOpenGL3_RenderDrawData");
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (first_frame) {
        LOGI("ImGuiWrapper::render: first ImGui_ImplOpenGL3_RenderDrawData done");
        first_frame = false;
    }
}
} // ui
