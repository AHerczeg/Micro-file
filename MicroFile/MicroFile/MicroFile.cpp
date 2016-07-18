// MicroFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"

//uint8_t* byteArray[4];

uint8_t flash[61440];


int main()
{

	int i, j;
	MicroBitFlash mf;
	uint8_t testArray[10];
	for (i = 0; i < 10; i++)
		testArray[i] = 0x00 + i;
	for (i = 0; i < 61439; i++)
		flash[i] = 0xFF;
	mf.flash_write(&flash[2048], testArray, 5, &flash[0]);
	mf.flash_write(&flash[2048], &testArray[2], 1, &flash[0]);
	//mf.flash_erase_mem((uint8_t*)(byteArray[3]), 2, NULL);
	for (i = 0; i < 10; i++)
		printf("%d\n", flash[2048+i]);
	while(1){}
    return 0;
}

