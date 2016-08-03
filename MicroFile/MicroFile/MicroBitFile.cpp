#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"

MicroBitFileSystem *fileSystem; 

MicroBitFile::MicroBitFile(char * name)
{
	//MicroBitFile(name, fs.getRootDir());
	fileSystem = MicroBitFileSystem::defaultFileSystem;
	this->file_name = name;
	this->file_directory = (*fileSystem).getRootDir();
	this->read_pointer = (*fileSystem).getBlock(80); //Check FAT table to find the start of the data
}

MicroBitFile::MicroBitFile(char * name, uint8_t* directory)
{
	file_name = name;
	file_directory = directory;
	read_pointer = (*fileSystem).getBlock(80); //Check FAT table to find the start of the data
}



int MicroBitFile::open(uint8_t flags) {
	//Find file, if not found return -1
	//Check file flags, if we can't modify, return 2
	//Set file flags
	this->flags = flags;

	//return flags so we can change it in the fd?

	return 0; // DELETE
}

int MicroBitFile::close() {
	//Check if File valid, if not return ERROR
	//Find fd, release flags
	//Return 1 on success, return ERROR

	return 0; // DELETE
}


int MicroBitFile::seek(int offset) {
	//Check if File valid, if not return ERROR
	//If offset > sizeOf(File)
		//return ERROR
	//
	
	return 0; // DELETE
}

int MicroBitFile::read(char * buffer, int length) {
	//Check if File valid, if not return ERROR
	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		this->read_pointer++;
	}
	return 0; // DELETE
}

int MicroBitFile::write() {
	//Check if File valid, if not return ERROR

	return 0; // DELETE
}

int MicroBitFile::append() {
	//Check if File valid, if not return ERROR

return 0; // DELETE
}

int MicroBitFile::length() {
	//Check if File valid, if not return ERROR

return 0; // DELETE
}
