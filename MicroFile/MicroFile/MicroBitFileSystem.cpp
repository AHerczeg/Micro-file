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
uint8_t *default_scratch_page;
uint8_t *scratch_page = &flash[TEMP_SCRATCH_PAGE * PAGE_SIZE];
int free_blocks;

MicroBitFlash mf; //TODO place this appropriately


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

	//todo count free blocks from fat table
	free_blocks = (FLASH_SPACE - (3 * PAGE_SIZE)) / BLOCK_SIZE;

	// occupy fat_entry with actual fat & root directory blocks
	for (int i = 0; i < 24; i++)
		flash[i] = 0x00;
}



int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length)
{
	//Check filename length, return FILENAME_TOO_LONG if too long
	int name_length = strlen(file_name);
	if (name_length > 16)
		return 1; // TODO filename too long error

	//Check if filename is unique, return EXISTING_FILE if not
	if (this->fileExists(file_name))
		return 0; //todo return error message

	//Check if space in root directory
	uint8_t *root_entry = this->getFreeRootEntry();
	if (root_entry == NULL)
		return 0; //todo insert error message here


	//Check space left, return OUT_OF_SPACE if there's not enough space
	if (length >= free_blocks * BLOCK_SIZE)
		return 3;  // todo  return error 



	//Write to memory/fat
	uint16_t first_block = this->write(byte_array, length);

	// Add to root directory
	mf.flash_write(root_entry, (uint8_t *) file_name, name_length, getRandomScratch());
	mf.flash_memset(root_entry + name_length, 0x00, (16 - name_length), NULL); //file name padding

	// Multiple flash_writes are not going to take a toll on flash int terms of wear, as it is 
	// initialized to 0xFF and we only change the each bit maximum once in this operation

	uint16_t file_entry[4];
	file_entry[0] = first_block; //first block
	file_entry[1] = 0x7FFF; //flags, 7F because 'free' should be set to 0
	*((uint32_t *) &file_entry[2]) = length;
	mf.flash_write(root_entry + 16, (uint8_t *) &file_entry, 8, getRandomScratch());


	//Return MICROBIT_OK on success, return ERROR otherwise
	return 1; // DELETE
}

uint16_t MicroBitFileSystem::write(uint8_t *byte_array, int length)
{
	int blocks_needed = length / BLOCK_SIZE + 1;

	if (length % BLOCK_SIZE == 0)
		blocks_needed--;

	uint16_t fat_entry = 0xFFFE; //EoF

	for (int i = blocks_needed - 1; i >= 0; i--) {

		
		int r = std::rand() % free_blocks;
		uint16_t block_number = 0;

		while (r) {
			uint16_t checked_block = *(((uint16_t *)fat_page) + block_number);
			if (checked_block == 0xFFFF) // consider 2 bytes at a time
				r--;
			block_number++;
		}

		mf.flash_write((fat_page + block_number * 2), (uint8_t *) &fat_entry, 2, NULL);
		fat_entry = block_number;

		//write actual data
		uint8_t *byte = byte_array + (i * BLOCK_SIZE);
		int byte_length = length - (i * BLOCK_SIZE);
		length -= byte_length;
		//printf("i: %d, length: %d, byte_length: %d, block_number: %d\n", i, length, byte_length, block_number);
		mf.flash_write(&flash[block_number * BLOCK_SIZE], byte, byte_length, getRandomScratch());
		free_blocks --;
	}

	return fat_entry;
}



uint8_t * MicroBitFileSystem::getRootEntry(int i)
{
	if (i >= 42) //todo make dynamic
		return NULL;
	i = i * 24;
	return (root_page + i);
}


bool MicroBitFileSystem::fileExists(char *file_name)
{
	if (this->getFileIndex(file_name) >= 0)
		return true;
	else
		return false;
}

uint8_t * MicroBitFileSystem::getFreeRootEntry()
{
	uint8_t *entry_deleted = NULL;
	uint8_t *root_entry = getRootEntry(0);
	
	int i = 0;
	while (root_entry != NULL) {
		if (*root_entry == 0xFF) //todo check flag instead
			return root_entry;

		if (~root_entry[18] & 0x40 && entry_deleted == NULL) // bitwise magic
			entry_deleted = root_entry;
		
		i++;
		root_entry = getRootEntry(i);
	}

	if (entry_deleted != NULL) {
		root_entry = root_page;

		for (int root_page_number = 0; root_page_number < 1; root_page_number++) { //todo number of root pages
		//repeat per root page
			//copy to scratch page
			uint8_t *scratch = getRandomScratch();
			
			for (int i = 0; i < 42; i++) {
				root_entry = getRootEntry(i);

				if (! (root_entry[18] & 0x40)) { //not deleted
					mf.flash_write(scratch + (i * 24), root_entry, 24, NULL);
				}
			}
			mf.erase_page((uint32_t *) root_page);
			mf.flash_write(scratch, root_page, PAGE_SIZE, NULL);
			mf.erase_page((uint32_t *) scratch);
		}
		return entry_deleted;
		//return first deleted entry
	}

	printf("Full\n");
	return NULL; //root directory full
}

uint8_t * MicroBitFileSystem::getRandomScratch()
{
	return scratch_page;
}


int MicroBitFileSystem::getFileIndex(char *file_name)
{
	uint8_t *root_entry = getRootEntry(0);

	int i = 0;
	while (root_entry != NULL) {
		if (*root_entry == 0xFF)
			return -1; //TODO write error code, no more root entries
	
		if (~(root_entry[18]) & 0x7F && (root_entry[18]) & 0x40) {
			for (int c = 0; c < 16; c++) {
				if (root_entry[c] != file_name[c])
					break;

				else if (root_entry[c] == 0xFF || c == 15)
					return i;
			}
		}

		i++;
		root_entry = getRootEntry(i);
	}

	return -1;
}
 


// Test function
#include <inttypes.h>
void MicroBitFileSystem::print()
{
	for (int entry = 0; entry < 50; entry++) {
		for (int i = 0; i < 16; i++) {
			printf("%c", flash[TEMP_ROOT_PAGE * PAGE_SIZE + entry*24 + i]);
		}
		
		//printf(" % " PRIu16, (uint16_t) flash[TEMP_ROOT_PAGE * PAGE_SIZE + entry * 24 + 16]);
		printf(" % " PRIu16, *((uint16_t *) &flash[TEMP_ROOT_PAGE * PAGE_SIZE + entry * 24 + 18]));
		//printf(" %d ", (uint16_t *)flash[TEMP_ROOT_PAGE * PAGE_SIZE + entry * 24 + 16]);

		printf("\n");
	}
	/*
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
	*/
}




int MicroBitFileSystem::remove(char *file_name) {
	//Find file, if not found return FILE_NOT_FOUND
	if (!fileExists(file_name))
		return 0; //todo microbit error code

	// Flip valid bit
	uint8_t * root_entry = getRootEntry(getFileIndex(file_name));
	uint8_t flags = root_entry[18] & 0x3F;
	mf.flash_memset(&root_entry[18], flags, 1, NULL);

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

