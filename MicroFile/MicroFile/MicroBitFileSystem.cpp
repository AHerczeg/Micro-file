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
uint8_t *free_blocks;

MicroBitFileSystem * MicroBitFileSystem::MicroBitFileSystem() 
{
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

int MicroBitFileSystem::add(char *fileName, uint8_t *byteArray)
{
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

int MicroBitFileSystem::remove(char *fileName) {
	//Find file, if not found return FILE_NOT_FOUND
	//Flip valid bit
	//Mark fat entries; set to 0x0000
	//Add number of deleted blocks to free_blocks
	//Return MICROBIT_OK on success, return ERROR otherwise
	return 0; // DELETE
}


