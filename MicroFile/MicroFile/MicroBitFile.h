#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{
	// Do we need all this?
	//int fileHandle;
	char * file_name;
	uint8_t * file_directory;
	//MicroBitFileSystem fs;
	uint8_t * read_pointer;
	FileDescriptor * fd;
	MicroBitFileSystem *fileSystem; // Or move to .cpp? also is it better as static?
	int offset; // There has to be a better way
 
	MicroBitFlash mf;

	private:

	public:

	MicroBitFile(char *file_name) : MicroBitFile(file_name, NULL) {};

	MicroBitFile(char *file_name, uint8_t * directory);

	int close(); // TODO
	
	int setPosition(int offset);

	int getPosition();

	int read();

	int read(char * buffer, int length);

	int write(const char *bytes, int len); // TODO

	int append(const char *bytes, int len);

	void operator+=(const char c);

	void operator+=(const char* s);

	int length();

	int move(uint8_t * directory); // TODO

	int remove();

};

#endif