#include <wtypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <math.h>
#include <TlHelp32.h>
#include <Psapi.h> 

#include "helper.h"
#include "memory.h"

GAME_INFO players[MAX_PLAYERS];
COORDINATES localOrigin;
int localTeam;
int player_count = 0;
VIEW_MATRIX_T view_matrix;
RECT game_bounds;

bool get_config(OFFSETS* offsets) {
	offsets->m_vOldOrigin = get_int("m_vOldOrigin");
	offsets->m_iHealth = get_int("m_iHealth");
	offsets->m_hPlayerPawn = get_int("m_hPlayerPawn");
	offsets->m_iTeamNum = get_int("m_iTeamNum");

	if (offsets->m_vOldOrigin == -1  ||
		offsets->m_iHealth == -1     ||
		offsets->m_hPlayerPawn == -1 ||
		offsets->m_iTeamNum == 0)
	{
		return false;
	}

	return true;
}

char* get_str(char* key_name) {
	FILE* file;
	char line[256];

	char* key;
	char* value;

	file = fopen("offsets.txt", "r");

	if (file == 0) {
		printf("Error: The file path does not exist or could not be opened.\n");
		return 0;
	}

	// Read the file line by line
	while (fgets(line, sizeof(line), file)) {
		key = strtok(line, "=");
		value = strtok(0, "=");

		if (value != 0) {
			// Remove newline character from value if present
			size_t len = strlen(value);
			if (len > 0 && value[len - 1] == '\n') {
				value[len - 1] = '\0';
			}
		}

		if (strcmp(key_name, key) == 0) {
			fclose(file);
			char* ret = (char*)malloc(strlen(value) + 1);
			strcpy(ret, value);
			return ret;
		}
	}

	fclose(file);
	printf("Error: The variable name could not be found in the txt file.");
	return 0;
}

int get_int(char* key_name) {
	char* str = get_str(key_name);
	if (str == 0) {
		return -1;
	}
	int ret = atoi(str);
	free(str);
	return ret;
}

COLORREF RGB_to_COLORREF(RGB color) {
	return ((DWORD)0 << 24) | ((DWORD)color.r << 16) | ((DWORD)color.g << 8) | ((DWORD)color.b);
}

COORDINATES subtract_vectors(const COORDINATES* v1, const COORDINATES* v2) {
	COORDINATES result = {
		v1->x - v2->x,
		v1->y - v2->y,
		v1->z - v2->z
	};
	return result;
}

float vector_length(const COORDINATES* v) {
	return sqrtf(v->x * v->x + v->y * v->y);
}

float calculate_distance(const COORDINATES* v1, const COORDINATES* v2) {
	float dx = v2->x - v1->x;
	float dy = v2->y - v1->y;
	float dz = v2->z - v1->z;

	return sqrtf(dx * dx + dy * dy + dz * dz);
}

void DrawBorderBox(HDC hdc, int x, int y, int w, int h, COLORREF borderColor) {
	HBRUSH hBorderBrush = CreateSolidBrush(borderColor);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBorderBrush);

	RECT rect = { x, y, x + w, y + h };
	FrameRect(hdc, &rect, hBorderBrush);

	SelectObject(hdc, hOldBrush); // Restore the original brush
	DeleteObject(hBorderBrush);	  // Delete the temporary brush
}

float calculate_component(VIEW_MATRIX_T* view_matrix, COORDINATES* v, int row) {
	return view_matrix->matrix[row][0] * v->x +
		view_matrix->matrix[row][1] * v->y +
		view_matrix->matrix[row][2] * v->z +
		view_matrix->matrix[row][3];
}

COORDINATES world_to_screen(COORDINATES* v) {
	COORDINATES vector;
	float _x = calculate_component(&view_matrix, v, 0); // _x using row 0

	float _y = calculate_component(&view_matrix, v, 1); // _y using row 1

	float w = calculate_component(&view_matrix, v, 3); // _w using row 3

	float inv_w = 1.0 / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = game_bounds.right * 0.5;
	float y = game_bounds.bottom * 0.5;

	x += 0.5 * _x * game_bounds.right + 0.5;
	y -= 0.5 * _y * game_bounds.bottom + 0.5;

	vector.x = x;
	vector.y = y;
	vector.z = w;
	return vector;
}

void LoopThroughPlayers(HDC hdcBuffer) {
	RGB esp_box_color_team = { 0, 255, 64 };
	RGB esp_box_color_enemy = { 0, 255, 64 };

	for (int i = 0; i < player_count; i++)
	{
		COORDINATES screenPos = world_to_screen(&players[i].origin);
		COORDINATES screenHead = world_to_screen(&players[i].head);

		if (screenPos.z >= 0.01) {
			float height = screenPos.y - screenHead.y;
			float width = height / 2.4;

			float distance = calculate_distance(&localOrigin, &players[i].origin);
			int roundedDistance = (int)round(distance / 10.0);

			COLORREF BorderColor;
			if (localTeam == players[i].team)
				BorderColor = RGB_to_COLORREF(esp_box_color_team);
			else
				BorderColor = RGB_to_COLORREF(esp_box_color_enemy);


			DrawBorderBox(
				hdcBuffer,
				screenHead.x - width / 2,
				screenHead.y,
				width,
				height,
				BorderColor
			);
		}
	}
}