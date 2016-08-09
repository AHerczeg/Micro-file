#ifndef MICROBIT_FILE_SYSTEM_H
#define MICROBIT_FILE_SYSTEM_H
#include "MicroBitConfig.h"
#include "MicroBitFlash.h"
//#include "MicrobitFile.h"
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 1024  // Size of page in flash.
#define BLOCK_SIZE 256

#define META_DATA_LENGTH 3
#define DOT_DOT 2 // Length of block number of parent directory
#define MAX_FILESYSTEM_PAGES 60 // Max. number of pages available.

#define PAGE_OFFSET(a) ((int) a % PAGE_SIZE)
#define PAGE_START(a) (a - PAGE_OFFSET(a))

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
	uint16_t free_blocks;
	MicroBitFlash mf;

	

	private:
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

		bool fileDescriptorExists(char *file_name, uint8_t *directory);

		bool validBlockNumber(uint16_t block_number);

		uint16_t getBlockNumber(uint8_t *address);

		uint8_t *getRandomScratch();

		uint8_t* getBlockAddress(uint16_t block_number);
		uint8_t *getNextBlock(uint8_t *block_address);
		uint16_t* getTableAddress(uint8_t *block_address);
		uint16_t* getTableAddress(uint16_t block_number);

		uint16_t* getFATEntry(uint8_t *address);

		uint8_t *get(uint16_t index, bool get_block);

		FileDescriptor *getFirstFileDescriptor(uint8_t *directory);
		FileDescriptor *getNextFileDescriptor(FileDescriptor *fd);
		int remove(char * file_name, bool is_directory, uint8_t * directory);
		void deleteTableEntries(uint16_t block_number);

	public:

		static MicroBitFileSystem *defaultFileSystem;

		MicroBitFileSystem();

		int createDirectory(char * file_path);
	
		int createFile(char * file_name, char * directory);
		int createFile(char *file_name, uint8_t *byte_array, int length, char *directory);
		int createDirectory(char *dir_name, uint8_t *target_dir);
		int createDirectory(char * name, char * target_directory);
	
		void print();

};


#endif
