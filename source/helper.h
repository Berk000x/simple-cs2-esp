#ifndef HELPER_H
#define HELPER_H

#define MAX_PLAYERS  1000

typedef struct _RGB {
	int r;
	int g;
	int b;
} RGB;

typedef struct _COORDINATES {
	float x, y, z;
} COORDINATES;

typedef struct _VIEW_MATRIX_T {
	float matrix[4][4];
} VIEW_MATRIX_T;

bool get_config(/*OFFSETS**/ void* offsets);
COORDINATES subtract_vectors(const COORDINATES* v1, const COORDINATES* v2);
float vector_length(const COORDINATES* v);
void LoopThroughPlayers(HDC hdcBuffer);

#endif
