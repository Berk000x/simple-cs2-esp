#include <wtypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <math.h>
#include <TlHelp32.h>
#include <Psapi.h> 

#include "memory.h"
#include "helper.h"


int esp_delay = 1;

RECT gameBounds;
OFFSETS offset;
HWND hWindow = 0;
HDC hdcBuffer = NULL;
extern HANDLE hProcess;
RECT game_bounds;
MODULE_INFO ModuleInfo;
int pid = 0;
int NewPid = 0;

void PrintBanner() {
	system("cls");
	printf(" /$$$$$$$$  /$$$$$$  /$$$$$$$ \n");
	printf("| $$_____/ /$$__  $$| $$__  $$\n");
	printf("| $$      | $$  \\__/| $$  \\ $$\n");
	printf("| $$$$$   |  $$$$$$ | $$$$$$$/\n");
	printf("| $$__/    \\____  $$| $$____/ \n");
	printf("| $$       /$$  \\ $$| $$      \n");
	printf("| $$$$$$$$|  $$$$$$/| $$      \n");
	printf("|________/ \\______/ |__/      \n");
	printf("\n");
	printf("==================================================\n");
	printf("Version 1.1                     \n");
	printf("The CS2 ESP is running successfully!    \n");
	printf("==================================================\n");
	printf("\n");
	Beep(500, 100);
}

bool HighwayInit() {

	pid = GetPidFromProcessName("cs2.exe");

	if (!pid) {
		Sleep(1000);
		return false;
	}

	hWindow = GetWindowHandleFromPid(pid);

	if (!hWindow) {
		Sleep(1000);
		return false;
	}

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	GetModuleInfo(pid, "client.dll", &ModuleInfo);
	if (ModuleInfo.base == 0)
	{
		Sleep(1000);
		return false;
	}

	FindOffsets(&offset);

	get_config(&offset);

	GetClientRect(hWindow, &game_bounds);

	return true;
}

LRESULT WndProc(HWND hWnd, unsigned int message, unsigned long long wParam, long long lParam) {
	HBITMAP hbmBuffer = NULL;

	switch (message)
	{
	case WM_CREATE:
	{
		hdcBuffer = CreateCompatibleDC(NULL);

		if (hdcBuffer == NULL) {
			printf("Error: hdcBuffer failed.");
			return 0;
		}

		hbmBuffer = CreateCompatibleBitmap(GetDC(hWnd), gameBounds.right, gameBounds.bottom);

		if (hbmBuffer == NULL) {
			printf("Error: hbmBuffer failed.");
			return 0;
		}

		SelectObject(hdcBuffer, hbmBuffer);

		if (SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED) == 0) {
			printf("Error: SetWindowLong failed.");
			return 0;
		}

		if (SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), 0, LWA_COLORKEY) == 0) {
			printf("Error: SetLayeredWindowAttributes failed.");
			return 0;
		}

		PrintBanner();
		break;
	}
	case WM_ERASEBKGND: // We handle this message to avoid flickering
		return TRUE;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		if (hdc == NULL) {
			printf("Error: hdc failed.");
			return 0;
		}

		//DOUBLE BUFFERING
		if (FillRect(hdcBuffer, &ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH)) == 0) {
			printf("Error: FillRect failed.");
			return 0;
		}


		if (GetForegroundWindow() == hWindow) {
			LoopThroughPlayers(hdcBuffer);
		}

		if (BitBlt(hdc, 0, 0, gameBounds.right, gameBounds.bottom, hdcBuffer, 0, 0, SRCCOPY) == 0) {
			printf("Error: BitBlt failed.");
			return 0;
		}

		EndPaint(hWnd, &ps);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
	case WM_DESTROY:
		DeleteDC(hdcBuffer);
		DeleteObject(hbmBuffer);

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI read_thread(LPVOID param) {
	while (true) {
		FindGameInfo();
		Sleep(1);
	}
	return 0;
}

int main() {
	//SetConsoleTitle("CS2 ESP");
	printf("Waiting for cs2.exe...\n");

	while(!HighwayInit(&hWindow, &pid));

	printf("cs2.exe was loaded successfully. \n");

	printf("Make sure your game is in Full Screen Windowed\n");

	while (GetForegroundWindow() != hWindow) {
		Sleep(1000);
		UpdateHWND(&hWindow, pid);
		ShowWindow(hWindow, TRUE);
	}

	WNDCLASSEXA wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.lpfnWndProc = WndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = WHITE_BRUSH;
	wc.hInstance = (HINSTANCE)GetWindowLongA(hWindow, -6); // GWL_HINSTANCE));
	wc.lpszMenuName = " ";
	wc.lpszClassName = " ";

	if (RegisterClassExA(&wc) == 0) {
		printf("Error: RegisterClassExA failed.");
		return 0;
	}

	if (GetClientRect(hWindow, &gameBounds) == 0) {
		printf("Error: GetClientRect failed.");
		return 0;
	}

	HWND hWnd = CreateWindowExA(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, " ", "Highway", WS_POPUP,
		gameBounds.left, gameBounds.top, gameBounds.right - gameBounds.left, gameBounds.bottom + gameBounds.left, NULL, NULL, NULL, NULL); 

	if (hWnd == NULL)
		return 0;

	if (SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) == 0) {
		printf("Error: SetWindowPos failed.");
		return 0;
	}

	ShowWindow(hWnd, TRUE);

	HANDLE readThread = CreateThread(NULL, 0, read_thread, NULL, 0, NULL);

	if (readThread == NULL) {
		printf("Error: readThread failed.");
		return 0;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		Sleep(esp_delay);
	}
	CloseHandle(readThread);

	return 1;
}
