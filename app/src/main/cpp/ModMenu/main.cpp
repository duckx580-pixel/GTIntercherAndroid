#include <chrono>
#include <thread>
#include <dlfcn.h>
#include <KittyMemory.h>

#include "game/hook.h"
#include "utils/log.h"
#include "ui/ui.h"
#include "mod_menu.h"

ModMenu* g_mod_menu{ nullptr };

__unused __attribute__((constructor))
void constructor_main()
{
    // Create a new thread because we don't want do while loop make main thread
    // stuck.
    auto thread = std::thread([]() {
        // Wait until Growtopia native library loaded. Bounded, so a failed or
        // aborted launch leaves a thread that exits rather than one spinning
        // for the life of the process.
        constexpr int kPollIntervalMs = 32;
        constexpr int kTimeoutMs = 60000;

        int waited = 0;
        while (dlopen("libgrowtopia.so", RTLD_NOLOAD) == nullptr) {
            if (waited >= kTimeoutMs) {
                LOGE("libgrowtopia.so did not load within %d ms; giving up", kTimeoutMs);
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{ kPollIntervalMs });
            waited += kPollIntervalMs;
        }

        KittyMemory::ProcMap map{};
        map = KittyMemory::getLibraryBaseMap("libgrowtopia.so");
        if (!map.isValid()) {
            LOGE("Failed to load libgrowtopia.so");
            return;
        }

        LOGD("libgrowtopia.so start address: 0x%llX", map.startAddress);

        // Create a global pointer to hold the class that will be
        // called inside the hook function.
        g_mod_menu = new ModMenu{};

        // Starting to hook Growtopia function.
        game::hook::init();
    });

    // Don't forget to detach the thread from the main thread.
    thread.detach();
}
