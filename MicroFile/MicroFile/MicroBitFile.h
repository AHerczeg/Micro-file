#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{
	//int fileHandle;
	char * file_name;
	int file_size;
	uint8_t *file_data;

	private:


	public:

	MicroBitFile(char *file_name, int size, uint8_t *data);

	int close();

	int 

}

#endif