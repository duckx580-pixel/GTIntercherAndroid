#include <cstdio>
#include <cstring>
#include <string>
#include <android/log.h>
#include <dobby.h>

#define INSTALL_HOOK(fn_name_t, fn_ret_t, fn_args_t...)                                          \
    fn_ret_t (*orig_##fn_name_t)(fn_args_t);                                                     \
    fn_ret_t fake_##fn_name_t(fn_args_t);                                                        \
    static void install_hook_##fn_name_t(void* sym_addr)                                         \
    {                                                                                             \
        DobbyHook(sym_addr, (dobby_dummy_func_t)fake_##fn_name_t,                               \
                  (dobby_dummy_func_t*)&orig_##fn_name_t);                                       \
    }                                                                                             \
    static void install_hook_##fn_name_t(const char* lib, const char* name)                      \
    {                                                                                             \
        void* sym_addr = DobbySymbolResolver(lib, name);                                         \
        install_hook_##fn_name_t(sym_addr);                                                      \
    }                                                                                             \
    static void install_hook_##fn_name_t(const char* name)                                       \
    {                                                                                             \
        install_hook_##fn_name_t(nullptr, name);                                                 \
    }                                                                                             \
    fn_ret_t fake_##fn_name_t(fn_args_t)

// Fix for blank-message spam in the developer console.
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

    if (buffer[0] == '\0') {
        return;
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        reinterpret_cast<const char* (__cdecl*)()>(DobbySymbolResolver(nullptr, "_Z10GetAppNamev"))(),
        "%s", buffer
    );
}

// v5.54: SendPacket switched from GCC libstdc++ COW std::string to NDK libc++
// SSO std::string.  Since we build with ANDROID_STL=c++_shared our std::string
// IS std::__ndk1::basic_string, so the parameter ABI matches directly.
INSTALL_HOOK(SendPacket, void, int message_type, std::string const& message, void* enet_peer)
{
    __android_log_print(
        ANDROID_LOG_INFO,
        "GTL.Native",
        "type: %d, length: %zu, data: %s",
        message_type,
        message.length(),
        message.data()
    );
    orig_SendPacket(message_type, message, enet_peer);
}

// v5.54 ARM64 BaseApp layout.
// fpsVisible sits at byte 0x1B9 (after 18 x 24-byte BoostSignal slots +
// 8-byte void* pad + bool consoleVisible).  If this ever breaks, verify with:
//   aarch64-linux-android-nm -D libgrowtopia.so | grep -i SetFpsVisible
struct BoostSignal {
    void* pad;   // +0
    void* pad2;  // +8
    void* pad3;  // +16
    // 24 bytes total on ARM64
};

struct BaseApp {
    BoostSignal signals[18]; // 0x000–0x1AF  (432 bytes)
    void*       _pad;        // 0x1B0         (8 bytes)
    bool        consoleVisible; // 0x1B8
    bool        fpsVisible;    // 0x1B9
    // v5.54 appends additional members after this point.
};

INSTALL_HOOK(BaseApp__Draw, void, BaseApp* thiz)
{
    thiz->fpsVisible = true;
    orig_BaseApp__Draw(thiz);
}

namespace game {
namespace hook {
    void init()
    {
        log_set_level(0);

        // LogMsg(char const*, ...)
        install_hook_LogMsg("_Z6LogMsgPKcz");

        // v5.54: NDK libc++ (SSO) std::string replaces GCC COW std::string.
        // Old symbol (v3.77): _Z10SendPacket15eNetMessageTypeRKSsP9_ENetPeer
        install_hook_SendPacket(
            "_Z10SendPacket15eNetMessageTypeRKNSt6__ndk112basic_stringIcNS_"
            "11char_traitsIcEENS_9allocatorIcEEEEP9_ENetPeer"
        );

        // BaseApp::Draw(void) – mangled name is stable across GT versions.
        install_hook_BaseApp__Draw("_ZN7BaseApp4DrawEv");
    }
} // hook
} // game
