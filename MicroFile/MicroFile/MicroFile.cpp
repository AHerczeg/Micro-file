// MicroFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"

char* byteArray[64];

int main()
{
	int i;
	for (i = 0; i < 64; i++)
		byteArray[i] = (char *)malloc(1024);
	for (i = 0; i < 64; i++)
		free(byteArray[i]);
    return 0;
}

