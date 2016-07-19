#ifndef MICROBIT_FILE_SYSTEM_H
#define MICROBIT_FILE_SYSTEM_H
#include "MicroBitConfig.h"
#include "MicroBitFlash.h"
//#include "MicrobitFile.h"
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 1024  // Size of page in flash.

#define MAX_FD 4  // Max number of open FD's.

#define FILENAME_LEN 16 // Filename length, including \0

#define MAX_FILESYSTEM_PAGES 60 // Max. number of pages available.



// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_APPEND 0x08

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04

class MicroBitFileSystem {
	

	private:


	public:

	MicroBitFileSystem();

	//MicroBitFile open(char *fileName);

	//int close(MicroBitFile file);

	int add(char *filename, uint8_t *byteArray, int length);

	int add(char *filename);

	int remove(char *fileName);
	
	int format();

	void print();

};

struct File
{
	char * name;
	uint8_t firstBlock;
	uint8_t flags;
	int length;
};

#endif
