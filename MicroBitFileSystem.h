#ifndef MICROBIT_FILE_SYSTEM_H
#define MICROBIT_FILE_SYSTEM_H
#include "MicroBitConfig.h"
#include "MicroBitFlash.h"
//#include "MicrobitFile.h"
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 1024  // Size of page in flash.
#define META_DATA_LENGTH 3
#define DOT_DOT 2 // Length of block number of parent directory
#define MAX_FD 4  // Max number of open FD's.

#define FILENAME_LEN 16 // Filename length, including \0

#define MAX_FILESYSTEM_PAGES 60 // Max. number of pages available.

// FAT & File Descriptor flags
#define UNUSED 0xFFFF
#define EOF 0xFFFE 
#define DELETED 0x0000

#define IS_FREE 0x8000
#define IS_VALID 0x4000
#define IS_DIRECTORY 0x2000

// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_APPEND 0x08

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04

struct FileDescriptor
{
	char file_name[16];
	uint16_t first_block;
	uint16_t flags;
	uint32_t length;
};

class MicroBitFileSystem {
	
	uint8_t *flash_address; //todo is base_address a better name?
	uint16_t *fat_page;
	uint8_t fat_length;
	uint8_t *root_dir;
	uint16_t free_blocks;
	MicroBitFlash mf;

	private:

	uint16_t write(uint8_t *byte_array, int length);

	FileDescriptor *getFileDescriptor(char *file_name, uint8_t *directory);
	
	int clearFat();

	bool fileExists(char *file_name);

	uint8_t *getRandomScratch();
	
	uint8_t *getBlockByNumber(uint16_t block_number);

	uint16_t *getFATEntryByNumber(uint16_t entry_number);

	uint8_t *get(uint16_t index, bool get_block);

	FileDescriptor *getFirstFileDescriptor(uint8_t *directory);

	FileDescriptor *getNextFileDescriptor(FileDescriptor *fd);

	public:

	MicroBitFileSystem();
	
	//MicroBitFile open(char *fileName);

	//int close(MicroBitFile file);

	int create(char *file_name, uint8_t *byte_array, int length);

	int remove(char *file_name);
	
	int format();

	void print();

};


#endif