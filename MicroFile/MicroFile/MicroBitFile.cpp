#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"
#include <string.h>



//TODO Rework overload
/*
MicroBitFile::MicroBitFile(char * name)
{
	this->fileSystem = MicroBitFileSystem::defaultFileSystem;
	this->file_name = name;
	MicroBitFile(name, (*fileSystem).getRoot());
}
*/

MicroBitFile::MicroBitFile(char * name, uint8_t* directory)
{
	this->fileSystem = MicroBitFileSystem::defaultFileSystem;
	this->file_name  = name;
	if (directory == NULL)
		this->file_directory = (*fileSystem).getRoot();
	else
		this->file_directory = directory;
	this->fd = (*fileSystem).findFileDescriptor(name, file_directory);
	this->offset = 0;
	this->read_pointer = (*fileSystem).getBlockAddress(fd->first_block); //Check FAT table to find the start of the data
}



int MicroBitFile::close() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message
	

	//Update 



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

	this->read_pointer = (*fileSystem).getBlockAddress(fd->first_block);

	for (int i = 0; i < offset / BLOCK_SIZE; i++) {
		this->read_pointer = (*fileSystem).getNextBlock(read_pointer);
	}

	this->offset = offset;
	
	this->read_pointer += offset % BLOCK_SIZE;

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

	if(sizeof(buffer) < length)
		return -1; // TODO add error message

	if(length > this->length())
		return -1; // TODO add error message

	// replace with seek? Probably a good idea

	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		if (setPosition(this->offset + 1) < 0)  // Change this when setPosition is fixed
			break;
	}

	/*
	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		if (i > 0 && i % BLOCK_SIZE == 0) {
			if ((*fileSystem).getNextBlock(read_pointer) != NULL)
				this->read_pointer = (*fileSystem).getNextBlock(read_pointer);
			else
				return 0; // TODO add error message maybe?
		}
		else
			this->read_pointer++;
		offset++;
	}
	*/
	return 0; // DELETE
}

int MicroBitFile::write(const char *bytes, int len) {
	
	// If len > 0, we have to change the fd too!
	
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	// Check if we need to allocate new block
	// Fix (we don't have to allocate a new block if there's enough space left in the current one)
	if (this->offset + len > this->fd->length) {
		if ( ( ( (this->offset + len) - this->fd->length) / BLOCK_SIZE) > (*fileSystem).free_blocks) {
			return -1; // TODO add error message
		} else {
			this->fd->length += ((this->offset + len) - this->fd->length);
			for (int i = 0; i < ( ( (this->offset + len) - this->fd->length) / BLOCK_SIZE); i++) {

			}
			//allocate new block
			//repalce EoF with the new block's number 
		}
	}

	// Will probably break during testing
	int written_bytes = 0;
	for (int i = 0; i <= ( ( (this->offset % BLOCK_SIZE) + len) / BLOCK_SIZE); i++) {
		int bytes_to_write = ((BLOCK_SIZE - (this->offset % BLOCK_SIZE)) <= len) ? (BLOCK_SIZE - (this->offset % BLOCK_SIZE)) : len;
		mf.flash_write(this->read_pointer, (uint8_t *)&bytes[written_bytes], (bytes_to_write), (*fileSystem).getRandomScratch());
		written_bytes += bytes_to_write; //fix this?
		setPosition(this->offset + bytes_to_write);
	}

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

	//this->offset++;

	//this->read_pointer++;

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
	delete this;
	return 0;
}
