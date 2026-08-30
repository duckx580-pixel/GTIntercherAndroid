#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <android/log.h>
#include <dobby.h>

#include "../helper/gnu_string.h"

#define LOG_TAG "GTL.Fix"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define INSTALL_HOOK(fn_name_t, fn_ret_t, fn_args_t...)                                                                    \
    fn_ret_t (*orig_##fn_name_t)(fn_args_t);                                                                               \
    fn_ret_t fake_##fn_name_t(fn_args_t);                                                                                  \
    static void install_hook_##fn_name_t(void* sym_addr)                                                                   \
    {                                                                                                                      \
        DobbyHook(sym_addr, (dobby_dummy_func_t)fake_##fn_name_t, (dobby_dummy_func_t*)&orig_##fn_name_t);                 \
    }                                                                                                                      \
    static bool install_hook_##fn_name_t(const char* lib, const char* name)                                                \
    {                                                                                                                      \
        void *sym_addr = DobbySymbolResolver(lib, name);                                                                   \
        /* Growtopia 5.55 ships a stripped libgrowtopia.so; hooking a null    */                                           \
        /* address would take the game down with us, so skip instead.         */                                           \
        if (sym_addr == nullptr) {                                                                                         \
            LOGW("Symbol '%s' not found; " #fn_name_t " hook disabled", name);                                             \
            return false;                                                                                                  \
        }                                                                                                                  \
        LOGI("Resolved '%s' -> %p", name, sym_addr);                                                                       \
        install_hook_##fn_name_t(sym_addr);                                                                                \
        return true;                                                                                                       \
    }                                                                                                                      \
    static bool install_hook_##fn_name_t(const char* name)                                                                 \
    {                                                                                                                      \
        return install_hook_##fn_name_t(nullptr, name);                                                                    \
    }                                                                                                                      \
    fn_ret_t fake_##fn_name_t(fn_args_t)

// Fix for printing blank message in the console.
INSTALL_HOOK(LogMsg, void, const char* msg, ...)
{
    if (msg[0] == '\0') {
        return;
    }

    char buffer[0x1000u];
    va_list va{};
    va_start(va, msg);
    std::memset(buffer, 0, sizeof(buffer));
    std::vsnprintf(buffer, 0x1000u, msg, va);
    va_end(va);

    // Double check.
    if (buffer[0] == '\0') {
        return;
    }

    auto get_app_name =
        reinterpret_cast<const char* (__cdecl*)()>(DobbySymbolResolver(nullptr, "_Z10GetAppNamev"));

    __android_log_print(
        ANDROID_LOG_INFO,
        get_app_name != nullptr ? get_app_name() : LOG_TAG,
        "%s", buffer
    );
}

INSTALL_HOOK(SendPacket, void, int message_type, gnu::string& message, void* enet_peer)
{
    __android_log_print(
        ANDROID_LOG_INFO,
        "GTL.Native",
        "type: %d, length: %lu, data: %s",
        message_type,
        message.length(),
        message.data()
    );
    orig_SendPacket(message_type, message, enet_peer);
}

namespace game {
namespace hook {
    void init()
    {
        // set Dobby logging level.
        log_set_level(0);

        // Growtopia 5.55 exports only its JNI entry points -- every internal
        // engine symbol below was stripped, so these hooks no-op on 5.55 and
        // stay available for older builds that still export them.
        //
        // The BaseApp::Draw hook that forced the FPS counter on is gone
        // entirely: it wrote through hardcoded 3.77-era struct offsets
        // (fpsVisible at +441), and blind-writing those into 5.55's relaid-out
        // BaseApp would corrupt unrelated memory.

        // LogMsg(char const*,...)
        install_hook_LogMsg("_Z6LogMsgPKcz");

        // SendPacket(eNetMessageType,std::string const&,_ENetPeer *)
        install_hook_SendPacket("_Z10SendPacket15eNetMessageTypeRKSsP9_ENetPeer");
    }
} // hook
} // game
