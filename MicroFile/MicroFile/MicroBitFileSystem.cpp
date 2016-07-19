#include "stdafx.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFlash.h"
#include "ErrorNo.h"

#define FAT 1
#define BLOCK_SIZE 256

#define TEMP_FAT_PAGE 0
#define TEMP_ROOT_PAGE 1
#define TEMP_SCRATCH_PAGE 2

#define FLASH_SPACE 61440
uint8_t flash[FLASH_SPACE];

uint8_t *fat_page;
uint8_t *root_page;
uint8_t free_blocks;

MicroBitFileSystem::MicroBitFileSystem() 
{
	int i;
	// TEST: initialize flash memory
	for (i = 0; i < 61439; i++)
		flash[i] = 0xFF;

	//Own init function?
	//If no pre-existing file system exists initialize
	//get random fat page here, and write to KVS
	//Get KVS values for fat- and root page
	fat_page = &(flash[TEMP_FAT_PAGE * PAGE_SIZE]);
	root_page = &(flash[TEMP_ROOT_PAGE * PAGE_SIZE]);
	free_blocks = (FLASH_SPACE - (3 * PAGE_SIZE)) / BLOCK_SIZE;


}

int MicroBitFileSystem::add(char *file_name, uint8_t *byteArray, int length)
{
	//File f = { fileName, 3072, 0, length };
	uint8_t file_entry[24];
	for (int i = 0; i < 24; i++)
		file_entry[i] = 0;

	_memccpy(&file_entry[0], file_name, 0, strlen(file_name));
	file_entry[16] = 0; //block number
	file_entry[18] = 0xFF; //flags
	file_entry[23] = 1;
	MicroBitFlash mf;
	mf.flash_write(&flash[1024], file_entry, 24, &flash[2048]);

	mf.flash_write(&flash[PAGE_SIZE * 3], byteArray, 1, &flash[2048]);


	//Check if filename is unique, return EXISTING_FILE if not
	//Check filename length, return FILENAME_TOO_LONG if too long
	//Check space left, return OUT_OF_SPACE if there's not enough space
	
	//Write to memory/fat
		//try to find free page if not
		//get random blocks, as described by algorithm
	//Write to root
		//Flip free bit
	//Return MICROBIT_OK on success, return ERROR otherwise
	


	return 0; // DELETE
}

void MicroBitFileSystem::print() {
	for (int i = 0; i < 8; i++)
		printf("%d", flash[3072 + i]);
}

int MicroBitFileSystem::add(char *fileName)
{
	return this->add(fileName, NULL, 0);

	return 0; // DELETE
}

int MicroBitFileSystem::remove(char *fileName) {
	//Find file, if not found return FILE_NOT_FOUND
	//Flip valid bit
	//Mark fat entries; set to 0x0000
	//Add number of deleted blocks to free_blocks
	//Return MICROBIT_OK on success, return ERROR otherwise
	return 0; // DELETE
}

/*
int MicroBitFileSystem::close(MicroBitFile file) {
	file.close();
	
	return 0; // DELETE
}



MicroBitFile MicroBitFileSystem::open(char *fileName) {
	
	//If file not found throw exception

	MicroBitFile f = MicroBitFile(fileName, 0, 0x00);

	return f;
}
*/
int MicroBitFileSystem::format() {
	
	
	return 0; // DELETE
}

