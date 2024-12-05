#ifndef MEMORY_H
#define MEMORY_H

#include "helper.h"

typedef struct _OFFSETS {
    unsigned long long dwEntityList;
    unsigned long long dwViewMatrix;
    unsigned long long dwLocalPlayerController;
    unsigned long long m_vOldOrigin;
    unsigned long long m_iHealth;
    unsigned long long m_hPlayerPawn;
    unsigned long long m_iTeamNum;
} OFFSETS;

typedef struct _GAME_INFO {
    unsigned long long entity;
    int team;
    unsigned long long pCSPlayerPawn;
    int health;
    COORDINATES origin;
    COORDINATES head;
} GAME_INFO;

typedef struct _MODULE_INFO {
    unsigned long long base;
    int size;
} MODULE_INFO;

int GetPidFromProcessName(const char* name);
HWND GetWindowHandleFromPid(unsigned long ProcessId);
bool UpdateHWND(HWND* hWindow, int pid);
bool GetModuleInfo(DWORD pid, const char* ModuleName, MODULE_INFO* ModuleInfo);
bool FindOffsets(OFFSETS* offsets);
void FindGameInfo();


#endif