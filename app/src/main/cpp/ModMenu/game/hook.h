#pragma once

#include <jni.h>

namespace game {
namespace hook {
// Installs both hooks synchronously on the calling thread using the given
// JNIEnv. The caller is expected to have already attached that thread to the
// JVM (see the comment at the call site in main.cpp) -- this does not attach
// or detach anything itself.
void init(JNIEnv* env);
} // hook
} // game
