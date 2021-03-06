#ifndef MICROBIT_FILE_SYSTEM_H
#define MICROBIT_FILE_SYSTEM_H
#include "MicroBitConfig.h"
#include "MicroBitFlash.h"
//#include "MicrobitFile.h"
#include <stdint.h>
#include <stdio.h>
#include "MicroBitFile.h"

#define PAGE_SIZE 1024  // Size of page in flash.
#define BLOCK_SIZE 256

#define META_DATA_LENGTH 3
#define DOT_DOT 2 // Length of block number of parent directory
#define MAX_FILESYSTEM_PAGES 60 // Max. number of pages available.

#define MAX_DEPTH 3

#define PAGE_OFFSET(a) ((a - flash_address) % PAGE_SIZE)
#define PAGE_OFFSET(a) (((uint8_t *)a - flash_address) % PAGE_SIZE)
#define PAGE_START(a) (a - PAGE_OFFSET(a))

#define BLOCK_OFFSET(a) ((a - flash_address) % BLOCK_SIZE)
#define BLOCK_START(a) (a - BLOCK_OFFSET(a))

#define BLOCK_NUMBER(a) (((a - flash_address) / BLOCK_SIZE) + 1)
#define SAME_BLOCK(a, b) (BLOCK_NUMBER(a) == BLOCK_NUMBER(b))

// FAT & File Descriptor flags
#define UNUSED 0xFFFF
#define EOF 0xFFFE
#define DELETED 0x0000

#define IS_FREE 0x8000
#define IS_VALID 0x4000
#define IS_DIRECTORY 0x2000

/*
// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_APPEND 0x08

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04
*/

struct FileDescriptor
{
	char file_name[16];
	uint16_t first_block;
	uint16_t flags;
	uint32_t length;
};

class MicroBitFileSystem {

	uint8_t *flash_address;
	uint16_t *fat_page;
	uint8_t fat_length;
	uint8_t *root_dir;
	
	MicroBitFlash mf;

	

	private:


	public:
		
		uint16_t free_blocks;
		
		
		uint16_t getRandomFreeBlock();
		uint8_t* getDirectory(char * directory);
		int freeSpace(int length);
		uint16_t write(uint8_t *byte_array, int length);
		FileDescriptor *newFileDescriptor(uint8_t *directory, bool allow_expand);
		uint8_t* getParentDirAddress(uint8_t *directory);
		uint8_t* getParentDirAddress(FileDescriptor *fd);


		FileDescriptor *clearDirectory(uint8_t *directory);
		inline void copyNeighboringBlocks(uint8_t * scratch_address, uint8_t *block_to_skip);

		FileDescriptor *findFileDescriptor(char * file_name, bool is_directory, uint8_t * directory);

		bool validBlockNumber(uint16_t block_number);

		uint8_t *getRandomScratch();

		uint8_t* getBlockAddress(uint16_t block_number);
		uint8_t *getNextBlock(uint8_t *block_address);
		uint16_t* getTableAddress(uint8_t *block_address);
		uint16_t* getTableAddress(uint16_t block_number);


		FileDescriptor *getFirstFileDescriptor(uint8_t *directory);
		FileDescriptor *getNextFileDescriptor(FileDescriptor *fd);

		void deleteTableEntries(uint16_t block_number);







		int remove(char * file_name);
		int remove(char * file_name, char * directory);
		int remove(FileDescriptor * fd); //can be private
		int removeDirectory(char * folder_name);
		int removeDirectory(char * folder_name, uint8_t * directory);
		int removeDirectory(char * folder_name, char * directory);
		int repeat(FileDescriptor *fd);
		static MicroBitFileSystem *defaultFileSystem;

		MicroBitFileSystem();
	
		int createFile(char *file_name, uint8_t *byte_array, int length, char *directory);
		int createDirectory(char * name, char * target_directory);
	
		void print();
		int close(MicroBitFile file);
		void printDir(uint16_t block_number, int level);
};


#endif
