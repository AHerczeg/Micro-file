#include "stdafx.h"
#include <stdlib.h>
#include "MicroBitFlash.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFile.h"
#include "ErrorNo.h"


/*
Creates an instance of MicroBitFile with the given name and directory,
if no directory is specified it is set to root

@param name the name of the file

@param directory the path from root the parent directory seperated by slashes
*/

MicroBitFile::MicroBitFile(char * name, char * directory)
 {
  	this->fileSystem = MicroBitFileSystem::defaultFileSystem;
 	this->file_name  = name;
 	this->file_directory = (*fileSystem).getDirectory(directory); // TODO What if invalid directory
  	this->fd = (*fileSystem).findFileDescriptor(name, false, file_directory); // TODO What if file not found
  	this->offset = 0;
  	this->read_pointer = (*fileSystem).getBlockAddress(fd->first_block);
 	this->last_block = (*fileSystem).getTableAddress(fd->first_block);
 	if (*last_block != 0xFFFE) {
 		do
 		{
 			last_block = (*fileSystem).getTableAddress(*last_block);
 		} while (*last_block != 0xFFFE);
 	}
  }



/*
Returns the updated FileDecriptor to be used in FileSystem

@return this MicroBitFile's updated FileDescriptor (e.: increase length after append()) 
*/

FileDescriptor * MicroBitFile::close() {

	return this->fd;
}


/*

to write in FileSystem: 

MicroBitFileSystem int::close(MicroBitFile file) {
	FileDescriptor * fd = findFileDescriptor(file.getName(), false, file.getDirectory())
	*fd = file.close() 
}

*/


/*
Sets the readpointer in the MicroBitFile to the given offset

@param offset the desired offset of the readpointer

@return MICROBIT_OK on succes, returns MICROBIT_INVALID_PARAMETER otherwise
*/
int MicroBitFile::setPosition(int offset) {

	
	if (offset < 0)
		return MICROBIT_INVALID_PARAMETER;

	if (offset > this->fd->length)
		return MICROBIT_INVALID_PARAMETER;

	this->read_pointer = (*fileSystem).getBlockAddress(fd->first_block);

	for (int i = 0; i < offset / BLOCK_SIZE; i++) {
		if((*fileSystem).getNextBlock(read_pointer))
			this->read_pointer = (*fileSystem).getNextBlock(read_pointer);
	}

	this->offset = offset;
	
	if (offset % BLOCK_SIZE)
		this->read_pointer += offset % BLOCK_SIZE;

	return MICROBIT_OK;
}

/*
Returns the current offset of the readpointer

@return the current offset of the readpointer
*/
int MicroBitFile::getPosition() {
	return this->offset;
}


/*
Reads a single character from the MicroBitFile starting from the current offset

@return the read character
*/
int MicroBitFile::read() {
	char c[1];
	read(c, 1);
	return c[0];	
}


/*
Writes a stream of characters from the MicroBitFile to the given array starting
at the current positon of the readpointer

@param buffer the destination array

@param length the number of read characters

@return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER otherwise
*/
int MicroBitFile::read(char * buffer, int length) {


	if(length > this->length())
		return MICROBIT_INVALID_PARAMETER; // TODO add error message

	for (int i = 0; i < length; i++) {
		buffer[i] = *(this->read_pointer);
		if (setPosition(this->offset + 1) < 0)  // Change this when setPosition is fixed
			break;
	}

	return MICROBIT_OK;
}


/*
Writes 'len' number of bytes from an array to the MicrBitFile starting from the
curret position of the readpointer. Existing bytes are overwritten, lenght is increased
if we pass the end of the file and we allocate new blocks if necassary and we have enough space

@bytes the source array

@len the number of written bytes

@return MICROBIT_OK on succes, MICROBIT_INVALID_PARAMETER if len is too short and
		MICROBIT_NO_RESOURCES if len is too big and we can't allocate new blocks
*/
int MicroBitFile::write(uint8_t * bytes, int len)
{

	if (len < 0)
		return MICROBIT_INVALID_PARAMETER;

	// Check if we need to allocate new block
	if (this->offset + len > this->fd->length) {
		if ( ( ( (this->offset + len) - this->fd->length) / BLOCK_SIZE) > (*fileSystem).free_blocks) {
			return MICROBIT_NO_RESOURCES; // TODO add error message
		}
		else {

			if ((this->offset % BLOCK_SIZE) + len > BLOCK_SIZE) {
				uint16_t table_entry = EOF;

				for (int i = 0; i < (((this->offset + len) - this->fd->length) / BLOCK_SIZE) + 1; i++) {
					uint16_t block_number = (*fileSystem).getRandomFreeBlock();

					mf.flash_write((uint8_t *)(*fileSystem).getTableAddress(block_number), (uint8_t *)&table_entry, 2, NULL);
					table_entry = block_number;

					(*fileSystem).free_blocks--;
				}

				mf.flash_write((uint8_t *)last_block, (uint8_t *)&table_entry, 2, (*fileSystem).getRandomScratch());

				last_block = (*fileSystem).getTableAddress(table_entry);

			}

			this->fd->length += ((this->offset + len) - this->fd->length);
		}
	}

	(*fileSystem).print();
	int written_bytes = 0;
	for (int i = 0; i <= ( ( (this->offset % BLOCK_SIZE) + len) / BLOCK_SIZE) + 1; i++) {
		int bytes_to_write = ((BLOCK_SIZE - (this->offset % BLOCK_SIZE)) <= len) ? (BLOCK_SIZE - (this->offset % BLOCK_SIZE)) : len;
		mf.flash_write(this->read_pointer, (uint8_t *)&bytes[written_bytes], (bytes_to_write), (*fileSystem).getRandomScratch());
		written_bytes += bytes_to_write;
		setPosition(this->offset + bytes_to_write);
	}

	return MICROBIT_OK;
}

/*
Appends bytes from an array to the end of the MicroBitFile increasing its size

@bytes the source array

@len the number of written bytes

@return MICROBIT_OK on succes, MICROBIT_INVALID_PARAMETER if len is too short and
		MICROBIT_NO_RESOURCES if len is too big and we can't allocate new blocks
*/
int MicroBitFile::append(uint8_t *bytes, int len) {

	if (len <= 0)
		return MICROBIT_OK;

	setPosition(this->fd->length);

	return write(bytes, len);
}

/*
Returns the length of the MicroBitFile

@return the length of the file
*/
int MicroBitFile::length() {
	return this->fd->length;
}

/*
Removes the MicroBitFile from the FileSystem then deletes this instance

@return MICROBIT_OK
*/
int MicroBitFile::remove()
{

	(*fileSystem).remove(this->file_name);
	delete this;
	return MICROBIT_OK; //Should this be an error instead?
}


/*
Returns the name of the MicroBitFile

@return the name of the file
*/
char * MicroBitFile::getName()
{
	return this->file_name;
}

/*
Returns the directory of the MicroBitFile

@return a pointer to the file's parent directory
*/
uint8_t * MicroBitFile::getDirectory()
{
	return this->file_directory;
}
