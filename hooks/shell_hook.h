// shell_hook.h
#ifndef SHELL_HOOK_H
#define SHELL_HOOK_H

#include <windows.h>

// Function prototypes
bool HookShellExecute();
void UnhookShellExecute();
BOOL WINAPI HookedShellExecuteExW(SHELLEXECUTEINFOW* pExecInfo);

#endif