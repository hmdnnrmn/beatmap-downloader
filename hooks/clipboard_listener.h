// clipboard_listener.h
#ifndef CLIPBOARD_LISTENER_H
#define CLIPBOARD_LISTENER_H

#include <windows.h>

bool StartClipboardListener();
void StopClipboardListener();
LRESULT CALLBACK ClipboardWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif