// MicroFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"

/*

	FAT TABLE: 0-1023
	ROOT DIRECTORY: 1024-2047
	DEFAULT SCRATCHPAGE: 2048-3071

*/

uint8_t flash[61440];




int main()
{

	MicroBitFileSystem fs;

	int i, j;
	MicroBitFlash mf;
	uint8_t testArray[10];
	for (i = 0; i < 10; i++)
		testArray[i] = 0x00 + i;
	for (i = 0; i < 61439; i++)
		flash[i] = 0xFF;
	mf.flash_write(&flash[3072], &testArray[1], 1, &flash[2048]);
	mf.flash_write(&flash[3072], &testArray[2], 1, &flash[2048]);
	//mf.flash_erase_mem(&flash[2048], 2, NULL);
	for (i = 0; i < 10; i++)
		printf("%d\n", flash[3072+i]);
	while(1){}
    return 0;
}

