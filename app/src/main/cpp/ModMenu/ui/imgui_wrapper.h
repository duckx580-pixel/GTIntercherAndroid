#pragma once
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>

namespace ui {
class ImGuiWrapper {
public:
    ImGuiWrapper(ImVec2 display_size);
    ~ImGuiWrapper();

    virtual bool init();
    virtual void render();

protected:
    virtual void draw() = 0;

    // Normalised scale factors (~1.0 on a 1920×1080 display).
    // x ≈ width/1920, y ≈ height/1080.  Available to derived classes for
    // sizing ImGui windows, cards, and font metrics consistently across DPIs.
    ImVec2 m_display_scale;

private:
    ImVec2 m_display_size;
};
} // ui
