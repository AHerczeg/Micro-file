#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{
	// Do we need all this?
	char * file_name;
	uint8_t * file_directory;
	uint8_t * read_pointer;
	uint16_t * last_block;
	FileDescriptor * fd;
	MicroBitFileSystem *fileSystem;
	int offset; // There has to be a better way
 
	MicroBitFlash mf;

	private:

	public:

	MicroBitFile(char *file_name) : MicroBitFile(file_name, NULL) {};

	MicroBitFile(char *file_name, uint8_t * directory);

	FileDescriptor * close(); // TODO
	
	int setPosition(int offset);

	int getPosition();

	int read();

	int read(char * buffer, int length);

	int write(const char *bytes, int len); // TODO

	int append(const char *bytes, int len);

	int length();

	int remove();

};

#endif