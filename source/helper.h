#ifndef HELPER_h
#define HELPER_h

#include <wtypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <math.h>
#include <TlHelp32.h>
#include <Psapi.h> 

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


COORDINATES subtract_vectors(const COORDINATES* v1, const COORDINATES* v2);
float vector_length(const COORDINATES* v);
void LoopThroughPlayers(HDC hdcBuffer);


#endif
