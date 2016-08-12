#pragma once
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

class MicroBitFile
{

	char * file_name;
	uint8_t * file_directory;
	uint8_t * read_pointer;
	FileDescriptor * fd;
	MicroBitFileSystem *fileSystem;
	int offset;
	MicroBitFlash mf;
	uint16_t * last_block;

	private:

	public:

	uint8_t * read_pointer;


	/*
	A delagation to MicroBitFile(char *file_name, char * directory)
	*/
	MicroBitFile(char *file_name) : MicroBitFile(file_name, NULL) {};


	/*
	Creates an instance of MicroBitFile with the given name and directory,
	if no directory is specified it is set to root

	@param name the name of the file

	@param directory the path from root the parent directory seperated by slashes
	*/
	MicroBitFile(char *file_name, char * directory);
	

	/*
	Returns the updated FileDecriptor to be used in FileSystem

	@return this MicroBitFile's updated FileDescriptor (e.: increase length after append())
	*/
	FileDescriptor * close();


	/*
	Sets the readpointer in the MicroBitFile to the given offset

	@param offset the desired offset of the readpointer

	@return MICROBIT_OK on succes, returns MICROBIT_INVALID_PARAMETER otherwise
	*/
	int setPosition(int offset);
	

	/*
	Returns the current offset of the readpointer

	@return the current offset of the readpointer
	*/
	int getPosition();


	/*
	Reads a single character from the MicroBitFile starting from the current offset

	@return the read character
	*/
	int read();


	/*
	Writes a stream of characters from the MicroBitFile to the given array starting
	at the current positon of the readpointer

	@param buffer the destination array

	@param length the number of read characters

	@return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER otherwise
	*/
	int read(char * buffer, int length);


	/*
	Writes 'len' number of bytes from an array to the MicrBitFile starting from the
	curret position of the readpointer. Existing bytes are overwritten, lenght is increased
	if we pass the end of the file and we allocate new blocks if necassary and we have enough space

	@bytes the source array

	@len the number of written bytes

	@return MICROBIT_OK on succes, MICROBIT_INVALID_PARAMETER if len is too short and
	MICROBIT_NO_RESOURCES if len is too big and we can't allocate new blocks
	*/
	int write(uint8_t *bytes, int len);

	
	/*
	Appends bytes from an array to the end of the MicroBitFile increasing its size

	@bytes the source array

	@len the number of written bytes

	@return MICROBIT_OK on succes, MICROBIT_INVALID_PARAMETER if len is too short and
	MICROBIT_NO_RESOURCES if len is too big and we can't allocate new blocks
	*/
	int append(uint8_t * bytes, int len);


	/*
	Returns the length of the MicroBitFile

	@return the length of the file
	*/
	int length();


	/*
	Removes the MicroBitFile from the FileSystem then deletes this instance

	@return MICROBIT_OK
	*/
	int remove();


	/*
	Returns the name of the MicroBitFile

	@return the name of the file
	*/
	char * getName();


	/*
	Returns the directory of the MicroBitFile

	@return a pointer to the file's parent directory
	*/
	uint8_t * getDirectory();

};

#endif