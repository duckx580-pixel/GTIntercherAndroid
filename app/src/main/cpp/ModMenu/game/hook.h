#pragma once

#include <jni.h>

namespace game {
namespace hook {
// Installs both hooks synchronously on the calling thread using the given
// JNIEnv. Must be called from a proper Java-originated thread (see the
// comment at the call site in main.cpp for why) -- it does not attach or
// detach any thread itself.
void init(JNIEnv* env);
} // hook
} // game
