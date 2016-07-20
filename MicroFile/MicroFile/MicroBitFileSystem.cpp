#include "stdafx.h"
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include "MicroBitConfig.h"
//#include "ManagedString.h"
#include "MicroBitFileSystem.h"
#include "MicroBitFlash.h"
#include "ErrorNo.h"


#define FAT 1
#define BLOCK_SIZE 256

#define TEMP_FAT_PAGE 0
#define TEMP_ROOT_PAGE 1
#define TEMP_SCRATCH_PAGE 2

#define UPPER_BYTE(a) ((a | 0x00FF) - 0xFF)
#define LOWER_BYTE(a) (a & 0xFF)

#define FLASH_SPACE 61440
uint8_t flash[FLASH_SPACE];

uint8_t *fat_page;
uint8_t *root_page;
uint8_t *scratch_page = &flash[TEMP_SCRATCH_PAGE * PAGE_SIZE];
int free_blocks;

MicroBitFileSystem::MicroBitFileSystem() 
{
	int i;
	// EMULATE: initialize flash memory
	for (i = 0; i < 61439; i++)
		flash[i] = 0xFF;

	//Own init function?
	//If no pre-existing file system exists initialize
	//get random fat page here, and write to KVS
	//Get KVS values for fat- and root page
	fat_page = &(flash[TEMP_FAT_PAGE * PAGE_SIZE]);
	root_page = &(flash[TEMP_ROOT_PAGE * PAGE_SIZE]);
	free_blocks = (FLASH_SPACE - (3 * PAGE_SIZE)) / BLOCK_SIZE;

	// occupy fat_entry with actual fat & root directory blocks
	for (int i = 0; i < 24; i++)
		flash[i] = 0x00;
}

MicroBitFlash mf; //TODO place this appropriately
int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length)
{
	//Check filename length, return FILENAME_TOO_LONG if too long
	int file_name_length = strlen(file_name);
	if (file_name_length > 16)
		return 1;

	file_name_length = 16;

	//Check if filename is unique, return EXISTING_FILE if not
	if (this->fileExists(file_name))
		return 2;

	// Space left in root directory?

	//Check space left, return OUT_OF_SPACE if there's not enough space
	if (length >= free_blocks * BLOCK_SIZE)
		return 3;

	//Write to memory/fat
	//get random blocks, as described by algorithm
	uint8_t first_block = this->write(byte_array, length);

	// Add to root directory
	mf.flash_write(&flash[1024], (uint8_t *)file_name, file_name_length, &flash[2048]);

	// Two writes are not going to take a toll on flash int terms of wear, as it is 
	// initialized to 0xFF and we only change the each bit maximum once in this operation

	uint8_t file_entry[8];
	file_entry[0] = 0; //first block
	file_entry[2] = 0x7F; //flags, 7F because 'free' should be 0
	file_entry[3] = 0xFF; //more flags
	file_entry[6] = UPPER_BYTE(length); // size part I
	file_entry[7] = LOWER_BYTE(length); // size part II
	mf.flash_write(root_page+16, file_entry, 8, scratch_page);


	//Return MICROBIT_OK on success, return ERROR otherwise
	return 1; // DELETE
}

/*
int MicroBitFileSystem::add(ManagedString file_name, uint8_t *byte_array, int length)
{
	return this->add(file_name.toCharArray(), byte_array, length);
}


int MicroBitFileSystem::add(char *file_name)
{
	return this->add(file_name, NULL, 0);

	return 0; // DELETE
}
*/

int MicroBitFileSystem::fileExists(char *file_name)
{
	// Check all 24 byte blocks in root directory
	for (int i = (TEMP_ROOT_PAGE * PAGE_SIZE); i < ((TEMP_ROOT_PAGE+1) * PAGE_SIZE); i += 24) {
		
		// If !free and valid (not deleted) flags are set
		if (~flash[i] & 0x7F  && flash[i] & 0x40) {
			for (int c = 0; c < 16; c++) {
				if (flash[i + c] != file_name[c])
					break;

				else if (flash[i + c] == 0xFF || c == 15)
					return 1;
			}
		}
	}

	return 0;
}
 

void MicroBitFileSystem::print()
{
	bool zero = 0;
	for (int i = 0; i < FLASH_SPACE; i++) {
		if (flash[i] != 0xFF) {
			printf("%c", flash[i]);
			zero = false;
		}
		else if (!zero) {
			printf("\n\n");
			zero = true;
		}
	}
}

uint8_t MicroBitFileSystem::write(uint8_t *byte_array, int length)
{
	int blocks_needed = length / BLOCK_SIZE + 1;
	
	if (length % BLOCK_SIZE == 0)
		blocks_needed--;

	uint8_t fat_entry = 0x00;

	for (int i = blocks_needed - 1; i >= 0; i--) {
		
		int r = std::rand() % free_blocks;
		printf("Random: %d\n", r);
		uint8_t block_number = 0;

		while (r) {
			if (flash[(TEMP_FAT_PAGE * PAGE_SIZE) + block_number * 2] == 0xFF)
				r--;
			block_number++;
		}

		flash[(TEMP_FAT_PAGE * PAGE_SIZE) + block_number * 2] = UPPER_BYTE(fat_entry); // TODO flash_memset
		flash[((TEMP_FAT_PAGE * PAGE_SIZE) + block_number * 2) + 1] = LOWER_BYTE(fat_entry); // TODO flash_memset
		fat_entry = block_number;

		//write actual data
		uint8_t *byte = byte_array + (i * BLOCK_SIZE);
		int byte_length = length - (i * BLOCK_SIZE);
		length -= byte_length;
		printf("i: %d, length: %d, byte_length: %d, block_number: %d\n", i, length, byte_length, block_number);
		mf.flash_write(&flash[block_number * BLOCK_SIZE], byte, byte_length, scratch_page);
	}

	free_blocks -= blocks_needed;
	return fat_entry;
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

