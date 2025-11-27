#pragma once

#include <windows.h>
#include <GL/gl.h>
#include <string>
#include <vector>
#include "imgui.h"

namespace OverlayGL {
    bool InstallOpenGLHooks();
    void RemoveOpenGLHooks();
    bool IsOverlayVisible();
    void SetOverlayVisible(bool visible);
}