#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"
#include <string.h>


MicroBitFile::MicroBitFile(char * name)
{
	fileSystem = MicroBitFileSystem::defaultFileSystem;
	MicroBitFile(name, (*fileSystem).getRoot());
}

MicroBitFile::MicroBitFile(char * name, uint8_t* directory)
{
	fileSystem = MicroBitFileSystem::defaultFileSystem;
	this->file_name = name;
	this->file_directory = directory;
	this->fd = (*fileSystem).findFileDescriptor(name, file_directory);
	this->offset = 0;
	read_pointer = (*fileSystem).getBlockAddress(fd->first_block); //Check FAT table to find the start of the data
}



int MicroBitFile::close() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message
	//Find fd, release flags
	//Return 1 on success, return ERROR

	return 0; // DELETE
}


int MicroBitFile::setPosition(int offset) {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message
	
	if (offset < 0)
		return -1;

	if (offset > this->fd->length)
		return -1;

	for (int i = 0; i < offset / BLOCK_SIZE; i++) {
		this->read_pointer = (*fileSystem).getNextBlock(read_pointer);
	}

	this->read_pointer += offset % BLOCK_SIZE;


	this->offset = offset;
	
	return 0; // DELETE
}

int MicroBitFile::getPosition() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message
	
	return this->offset;

	return 0; // DELETE
}

int MicroBitFile::read() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	char c[1];
	read(c, 1);
	return c[0];
	
}

int MicroBitFile::read(char * buffer, int length) {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	if(sizeof(buffer) > length)
		return -1; // TODO add error message

	if(length > this->length())
		return -1; // TODO add error message

	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		if (i > 0 && i % BLOCK_SIZE != 0) {
			if ((*fileSystem).getNextBlock(read_pointer) != NULL)
				this->read_pointer = (*fileSystem).getNextBlock(read_pointer);
			else
				return 0; // TODO add error message maybe?
		}
		else
			this->read_pointer++;
	}
	return 0; // DELETE
}

int MicroBitFile::write(const char *bytes, int len) {
	
	// If len > 0, we have to change the fd too!
	
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	return 0; // DELETE
}

int MicroBitFile::append(const char *bytes, int len) {

	// If len > 0, we have to change the fd too!

	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	if (len < 0)
		return -1;

	setPosition(this->fd->length);

	write(bytes, len);

	return 0; // DELETE
}

void MicroBitFile::operator+=(const char c)
{
	append(&c, 1);
}

void MicroBitFile::operator+=(const char * s)
{
	append(s, strlen(s));
}

int MicroBitFile::length() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	return this->fd->length; // DELETE
}

int MicroBitFile::move(uint8_t * directory)
{
	return 0;
}

int MicroBitFile::remove()
{
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message
	(*fileSystem).remove(this->file_name, this->file_directory);
	return 0;
}
