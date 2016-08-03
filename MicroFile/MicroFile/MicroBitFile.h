#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{
	//int fileHandle;
	char * file_name;
	uint8_t * file_directory;
	//MicroBitFileSystem fs;
	uint8_t * read_pointer;
	uint8_t flags;

	private:

	public:

	MicroBitFile(char *file_name);

	MicroBitFile(char *file_name, uint8_t * directory);

	int open(uint8_t flags);

	int close(); 
	
	int seek(int offset);

	int read(char * buffer, int length);

	int write();

	int append();

	int length();

};

#endif