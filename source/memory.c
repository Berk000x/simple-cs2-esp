#include <wtypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <math.h>
#include <TlHelp32.h>
#include <Psapi.h> 

#include "memory.h"

bool team_esp = true;

MODULE_INFO ModuleInfo;
OFFSETS offset;
HANDLE hProcess = 0;
extern VIEW_MATRIX_T view_matrix;
extern GAME_INFO players[MAX_PLAYERS];
extern COORDINATES localOrigin;
extern int player_count;
extern int localTeam;

int GetPidFromProcessName(const char* name) {
	HANDLE hSnapshot;
	PROCESSENTRY32 pe;

	// Take a snapshot of all processes in the system
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		printf("Failed to create process snapshot.\n");
		return 1;
	}

	pe.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process
	if (Process32First(hSnapshot, &pe)) {
		// Now enter the loop and print information for all processes
		while (Process32Next(hSnapshot, &pe)) {
			if (strcmp(pe.szExeFile, name) == 0) {
				return pe.th32ProcessID;
			}
		}
	}
	else {
		printf("Failed to retrieve first process.\n");
		CloseHandle(hSnapshot);
		return 1;
	}

	// Clean up the snapshot object
	CloseHandle(hSnapshot);

	return 0;
}

HWND GetWindowHandleFromPid(unsigned long ProcessId) {
	HWND hwnd = NULL;
	do {
		hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);
		unsigned long pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid == ProcessId) {
			TCHAR windowTitle[MAX_PATH];
			GetWindowText(hwnd, windowTitle, MAX_PATH);
			if (IsWindowVisible(hwnd) && windowTitle[0] != '\0') {
				return hwnd;
			}
		}
	} while (hwnd != NULL);

	return NULL; // No main window found for the given process ID
}

bool UpdateHWND(HWND* hWindow, int pid) {
	*hWindow = GetWindowHandleFromPid(pid);
	return *hWindow == 0;
}

bool GetModuleInfo(DWORD pid, const char* ModuleName, MODULE_INFO* ModuleInfo) {
	MODULEENTRY32 ModuleEntry = { 0 };

	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (SnapShot == INVALID_HANDLE_VALUE)
		return false;

	ModuleEntry.dwSize = sizeof(ModuleEntry);

	while (Module32Next(SnapShot, &ModuleEntry)) {
		if (strcmp(ModuleEntry.szModule, ModuleName) == 0)
		{
			CloseHandle(SnapShot);
			ModuleInfo->base = (unsigned long long)ModuleEntry.modBaseAddr;
			ModuleInfo->size = ModuleEntry.modBaseSize;
			return true;
		}
	}

	CloseHandle(SnapShot);
	return false;
}

unsigned long long FindOffset(
	HANDLE process, 
	unsigned long long base, 
	unsigned char pattern[], 
	int size, 
	int distance, 
	int BaseSize) {

	#define PAGE_SIZE 4096 // Typically, 4 KB
	unsigned char buffer[PAGE_SIZE];
	unsigned int SearchedPointer = 0;

	for (unsigned long long address = base; address < base + BaseSize; address += PAGE_SIZE - size + 1)
	{
		SIZE_T bytes_read;

		if (!ReadProcessMemory(process, (LPCVOID)address, buffer, PAGE_SIZE, &bytes_read))
		{
			continue;
		}

		for (int i = 0; i <= bytes_read - size; i++)
		{
			if (memcmp(buffer + i, pattern, size) != 0)
			{
				continue;
			}

			if (ReadProcessMemory(process, (LPCVOID)(address + i + size + distance), &SearchedPointer, sizeof(SearchedPointer), 0))
			{
				return address + size + distance + SearchedPointer + sizeof(int) + i;
			}
		}
	}

	return 0;
}

bool FindOffsets(OFFSETS* offsets) {
	MODULE_INFO module_info;
	int pid = GetPidFromProcessName("cs2.exe");
	unsigned long long base = GetModuleInfo(pid, "client.dll", &module_info);

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	byte entity_list[] = { 0x41, 0x5D, 0x5B, 0x5D, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
						   0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48 };

	byte player_controller[] = { 0x48, 0x83, 0xC4, 0x20, 0x5F, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
								 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x83, 0xEC, 0x28, 0x83, 0xF9 };

	byte view_matrix[] = { 0x32, 0xC0, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
						   0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x63, 0xC2 };

	offsets->dwEntityList = FindOffset(process, module_info.base, entity_list, sizeof(entity_list), 82, module_info.size);
	offsets->dwLocalPlayerController = FindOffset(process, module_info.base, player_controller, sizeof(player_controller), 32, module_info.size);
	offsets->dwViewMatrix = FindOffset(process, module_info.base, view_matrix, sizeof(view_matrix), 3, module_info.size);

	CloseHandle(process);

	if (offsets->dwEntityList == 0 ||
		offsets->dwLocalPlayerController == 0 ||
		offsets->dwViewMatrix == 0)
	{
		return false;
	}

	offsets->dwEntityList -= module_info.base;
	offsets->dwLocalPlayerController -= module_info.base;
	offsets->dwViewMatrix -= module_info.base;

	return true;
}

void add_to_list(GAME_INFO player) {
	if (player_count < MAX_PLAYERS) {
		players[player_count] = player;
		player_count++;

	}
	else {
		printf("Error: Player list is full.\n");
	}
}

void FindGameInfo() {

	unsigned long long entity_list;
	unsigned long long localPlayer;
	unsigned long long localpCSPlayerPawn;
	unsigned int localPlayerPawn;
	unsigned long long localList_entry2;
	float render_distance = -1.0;
	int playerIndex = 0;
	GAME_INFO player;
	uintptr_t list_entry, list_entry2, playerPawn;

	bool Result = ReadProcessMemory(hProcess, (void*)(ModuleInfo.base + offset.dwLocalPlayerController), &localPlayer, sizeof(long long), 0);

	if (!Result || !localPlayer)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(localPlayer + offset.m_hPlayerPawn), &localPlayerPawn, sizeof(int), 0);

	if (!Result || !localPlayerPawn)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(ModuleInfo.base + offset.dwEntityList), &entity_list, sizeof(long long), 0);

	if (!Result)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(entity_list + 0x8 * ((localPlayerPawn & 0x7FFF) >> 9) + 16), &localList_entry2, sizeof(long long), 0);

	if (!Result)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(localList_entry2 + 120 * (localPlayerPawn & 0x1FF)), &localpCSPlayerPawn, sizeof(long long), 0);

	if (!Result || !localpCSPlayerPawn)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(ModuleInfo.base + offset.dwViewMatrix), &view_matrix, sizeof(VIEW_MATRIX_T), 0);

	if (!Result)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(localPlayer + offset.m_iTeamNum), &localTeam, sizeof(int), 0);

	if (!Result)
		return;

	Result = ReadProcessMemory(hProcess, (void*)(localpCSPlayerPawn + offset.m_vOldOrigin), &localOrigin, sizeof(COORDINATES), 0);

	if (!Result)
		return;

	while (true) {
		playerIndex++;

		Result = ReadProcessMemory(hProcess, (void*)(entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16), &list_entry, sizeof(long long), 0);

		if (!Result || !list_entry)
			break;

		Result = ReadProcessMemory(hProcess, (void*)(list_entry + 120 * (playerIndex & 0x1FF)), &player.entity, sizeof(long long), 0);

		if (!Result || !player.entity)
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(player.entity + offset.m_iTeamNum), &player.team, sizeof(int), 0);

		if (!Result)
			continue;

		if (!team_esp && (player.team == localTeam))
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(player.entity + offset.m_hPlayerPawn), &playerPawn, sizeof(long long), 0);

		if (!Result)
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16), &list_entry2, sizeof(long long), 0);

		if (!Result || !list_entry2)
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(list_entry2 + 120 * (playerPawn & 0x1FF)), &player.pCSPlayerPawn, sizeof(long long), 0);

		if (!Result || !player.pCSPlayerPawn)
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(player.pCSPlayerPawn + offset.m_iHealth), &player.health, sizeof(int), 0);

		if (!Result)
			continue;

		if (player.health <= 0 || player.health > 100)
			continue;

		if (!team_esp && (player.pCSPlayerPawn == localPlayer))
			continue;

		Result = ReadProcessMemory(hProcess, (void*)(player.pCSPlayerPawn + offset.m_vOldOrigin), &player.origin, sizeof(COORDINATES), 0);

		if (!Result)
			continue;

		player.head.x = player.origin.x;
		player.head.y = player.origin.y;
		player.head.z = player.origin.z + 75.0;

		if (player.origin.x == localOrigin.x && player.origin.y == localOrigin.y && player.origin.z == localOrigin.z)
			continue;

		COORDINATES difference = subtract_vectors(&localOrigin, &player.origin);
		if (render_distance != -1 && vector_length(&difference) > render_distance) {
			continue;
		}

		if (player.origin.x == 0 && player.origin.y == 0) continue;

		add_to_list(player);
	}

	player_count = 0;
}