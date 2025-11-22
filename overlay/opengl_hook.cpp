/*
    opengl_hook.cpp
    Public C-interface wrappers for the OpenGL + Dear ImGui overlay hook.

    These wrappers expose simple functions that can be used elsewhere in the DLL
    (or exported) without depending on C++ name mangling or the OverlayGL namespace.

    Build notes:
    - Ensure opengl_hook.h is on the include path.
    - This translation unit can be safely compiled into the existing DLL project.
*/

#include "opengl_hook.h"

extern "C" {

// Installs the OpenGL SwapBuffers hook and initializes the overlay systems on first frame.
__declspec(dllexport) BOOL Overlay_InstallOpenGLHooks() {
    return OverlayGL::InstallOpenGLHooks() ? TRUE : FALSE;
}

// Removes the OpenGL hook and shuts down the overlay systems.
__declspec(dllexport) void Overlay_RemoveOpenGLHooks() {
    OverlayGL::RemoveOpenGLHooks();
}

// Sets overlay visibility explicitly (TRUE to show, FALSE to hide).
__declspec(dllexport) void Overlay_SetVisible(BOOL visible) {
    OverlayGL::SetOverlayVisible(visible ? true : false);
}

// Returns current overlay visibility (TRUE if visible).
__declspec(dllexport) BOOL Overlay_IsVisible() {
    return OverlayGL::IsOverlayVisible() ? TRUE : FALSE;
}

} // extern "C"