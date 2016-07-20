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


int main()
{
	MicroBitFileSystem fs;
	uint8_t number = 64;
	fs.add("file.txt", &number, 1);
	fs.print();

	while(1){}
    return 0;
}

