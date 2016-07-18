// MicroFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"

uint8_t* byteArray[4];
int main()
{
	int i;
	MicroBitFlash mf;
	uint8_t testArray[10];
	for (i = 0; i < 10; i++)
		testArray[i] = 0x00 + i;
	for(i = 0; i < 4; i++)
		byteArray[i] = (uint8_t*)malloc(1024);
	mf.flash_write((uint8_t*)(byteArray[3]), (uint8_t*)(&testArray), 4, NULL);
	mf.flash_erase_mem((uint8_t*)(byteArray[3]), 2, NULL);
	for (i = 0; i < 5; i++)
		printf("%d\n", byteArray[3][i]);
	for (i = 0; i < 4; i++)
		free(byteArray[i]);
	while(1){}
    return 0;
}

