#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"


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
	this->last_block = (*fileSystem).getTableAddress(fd->first_block);
	if (*last_block != 0xFFFE) {
		do
		{
			last_block = (*fileSystem).getTableAddress(*last_block);
		} while (*last_block != 0xFFFE);
	}
}



FileDescriptor * MicroBitFile::close() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return nullptr; // TODO add error message

	return this->fd; // DELETE
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
	
	if(length > this->length())
		return -1; // TODO add error message

	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		if (setPosition(this->offset + 1) < 0)  // Change this when setPosition is fixed
			break;
	}

	return 0; // DELETE
}

int MicroBitFile::write(const char *bytes, int len) {
	
	// If len + offset > length, we have to change the fd too!
	
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message


	if (this->offset + len > this->fd->length) {
		if ( ( ( (this->offset + len) - this->fd->length) / BLOCK_SIZE) > (*fileSystem).free_blocks) {
			return -1; // TODO add error message
		}
		else {
			
			if ((this->offset % BLOCK_SIZE) + len > BLOCK_SIZE) {

				//Almost exactly fs.write()
				uint16_t table_entry = EOF;

				for (int i = 0; i < (((this->offset + len) - this->fd->length) / BLOCK_SIZE) + 1; i++) {
					uint16_t block_number = (*fileSystem).getRandomFreeBlock();

					mf.flash_write((uint8_t *)(*fileSystem).getTableAddress(block_number), (uint8_t *)&table_entry, 2, NULL);
					table_entry = block_number;

					(*fileSystem).free_blocks--;
				}

				//Change the original 'last block' from EoF to next block
				mf.flash_write((uint8_t *)last_block, (uint8_t *)&table_entry, 2, (*fileSystem).getRandomScratch());

				last_block = (*fileSystem).getTableAddress(table_entry);

			}

			this->fd->length += ((this->offset + len) - this->fd->length);

		}
	}

	int written_bytes = 0;
	for (int i = 0; i <= ( ( (this->offset % BLOCK_SIZE) + len) / BLOCK_SIZE) + 1; i++) {
		int bytes_to_write = ((BLOCK_SIZE - (this->offset % BLOCK_SIZE)) <= len) ? (BLOCK_SIZE - (this->offset % BLOCK_SIZE)) : len;
		mf.flash_write(this->read_pointer, (uint8_t *)&bytes[written_bytes], (bytes_to_write), (*fileSystem).getRandomScratch());
		written_bytes += bytes_to_write;
		setPosition(this->offset + bytes_to_write);
	}

	return 0; // DELETE
}

int MicroBitFile::append(const char *bytes, int len) {

	// If len > 0, we have to change the fd too!

	if (len == 0)
		return 0;

	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	if (len < 0)
		return -1;

	setPosition(this->fd->length);

	write(bytes, len);

	return 0; // DELETE
}

int MicroBitFile::length() {
	//Check if File valid, if not return ERROR
	if (!(*fileSystem).fileDescriptorExists(this->file_name, this->file_directory))
		return -1; // TODO add error message

	return this->fd->length; // DELETE
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
