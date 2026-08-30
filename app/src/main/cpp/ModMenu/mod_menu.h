#pragma once

namespace ui {
class Ui;
}

struct ModMenu {
    // Created lazily on the first frame, once the GL viewport is known.
    ui::Ui* m_ui{ nullptr };
};
