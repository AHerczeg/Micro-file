#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{

	char * file_name;
	uint8_t * file_directory;
	
	FileDescriptor * fd;
	MicroBitFileSystem *fileSystem;
	int offset;
	MicroBitFlash mf;
	uint16_t * last_block;

	private:

	public:

	uint8_t * read_pointer;

	MicroBitFile(char *file_name) : MicroBitFile(file_name, NULL) {};

	MicroBitFile(char *file_name, uint8_t * directory);
	
	FileDescriptor * close();

	int setPosition(int offset);

	int getPosition();

	int read();

	int read(char * buffer, int length);

	int write(uint8_t *bytes, int len); // TODO

	int append(uint8_t * bytes, int len);

	int length();

	int move(uint8_t * directory); // TODO

	int remove();

};

#endif