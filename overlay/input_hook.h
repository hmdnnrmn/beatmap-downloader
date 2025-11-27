#pragma once
#include <windows.h>
#include <vector>
#include <iostream>

namespace InputHook {

    // Store the original raw input devices to restore them later
    static std::vector<RAWINPUTDEVICE> g_OriginalDevices;
    static bool g_RawInputDisabled = false;

    static void DisableRawInputDevices() {
        if (g_RawInputDisabled) return;

        UINT nDevices = 0;
        GetRegisteredRawInputDevices(NULL, &nDevices, sizeof(RAWINPUTDEVICE));

        if (nDevices == 0) return;

        std::vector<RAWINPUTDEVICE> devices(nDevices);
        if (GetRegisteredRawInputDevices(devices.data(), &nDevices, sizeof(RAWINPUTDEVICE)) == (UINT)-1) {
            return;
        }

        g_OriginalDevices = devices; // Save original state

        // Create a list of devices to remove (specifically mouse)
        std::vector<RAWINPUTDEVICE> removeDevices;
        for (const auto& dev : devices) {
            if (dev.usUsagePage == 1 && (dev.usUsage == 2 || dev.usUsage == 6)) { // Generic Desktop Controls: Mouse (2) or Keyboard (6)
                RAWINPUTDEVICE removeDev = dev;
                removeDev.dwFlags = RIDEV_REMOVE;
                removeDev.hwndTarget = NULL; // Must be NULL for RIDEV_REMOVE
                removeDevices.push_back(removeDev);
            }
        }

        if (!removeDevices.empty()) {
            if (RegisterRawInputDevices(removeDevices.data(), (UINT)removeDevices.size(), sizeof(RAWINPUTDEVICE))) {
                g_RawInputDisabled = true;
                // std::cout << "Raw input (mouse) disabled." << std::endl;
            } else {
                // std::cerr << "Failed to disable raw input. Error: " << GetLastError() << std::endl;
            }
        }
    }

    static void RestoreRawInputDevices() {
        if (!g_RawInputDisabled || g_OriginalDevices.empty()) return;

        if (RegisterRawInputDevices(g_OriginalDevices.data(), (UINT)g_OriginalDevices.size(), sizeof(RAWINPUTDEVICE))) {
            g_RawInputDisabled = false;
            g_OriginalDevices.clear();
            // std::cout << "Raw input restored." << std::endl;
        } else {
            // std::cerr << "Failed to restore raw input. Error: " << GetLastError() << std::endl;
        }
    }
}
