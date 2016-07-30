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
#define ROOT 1
#define BLOCK_SIZE 256


#define FLASH_SPACE 61440
uint8_t flash_memory[FLASH_SPACE];


MicroBitFileSystem::MicroBitFileSystem() 
{

	int i;
	// EMULATE: initialize flash memory
	for (i = 0; i < FLASH_SPACE; i++)
		flash_memory[i] = 0xFF;

	//Own init function?
	//If no pre-existing file system exists initialize
	//get random fat page here, and write to KVS
	//Get KVS values for fat- and root page
	this->flash_address = &flash_memory[0];
	fat_page = (uint16_t *) &(flash_memory[1 * PAGE_SIZE]);
	root_dir = &(flash_memory[1 * PAGE_SIZE]);

	//todo count free blocks from fat table
	free_blocks = (FLASH_SPACE - (3 * PAGE_SIZE)) / BLOCK_SIZE;
	// occupy fat_entry with actual fat & root directory blocks
	
	uint16_t *fat_entry = fat_page;
	for (int i = 0; i < 9; i++) {
		*fat_entry = 0xFFFE;
		fat_entry++;
	}
}



int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length)
{
	int name_length = strlen(file_name);
	if (name_length > 16)
		return 1; // TODO filename too long error

	if (this->fileExists(file_name))
		return 0; //todo return error message

	//Check if space in root directory
	FileDescriptor *dir_entry = this->getFreeRootEntry();
	if (dir_entry == NULL)
		return 0; //todo insert error message here

	//Check space left, return OUT_OF_SPACE if there's not enough space
	if (length > free_blocks * BLOCK_SIZE) {
		int deleted_blocks = 0;
		uint16_t *fat_entry = fat_page;

		// Check that fat entry pointer stays within our fat page/s && pointing to actual existing blocks
		for (int i = 0; i < PAGE_SIZE * FAT / 2 && i < FLASH_SPACE / BLOCK_SIZE; i++) {
			if (*fat_entry == DELETED)
				deleted_blocks++;
			fat_entry++;
		}
		if (length > (free_blocks + deleted_blocks) * BLOCK_SIZE)
			return 3;  // todo  return error
		else
			clearFat();
	}
	
	FileDescriptor fd = { "", this->write(byte_array, length), ~IS_FREE, length };
	memcpy((char *) &fd, file_name, 16);
	mf.flash_write((uint8_t *) dir_entry, (uint8_t *) &fd, 24, getRandomScratch());
	
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

		int r = (free_blocks == 1) ? 1 : ((std::rand() % (free_blocks - 1)) + 1);
		int block_number = -1;

		while (r) { //todo make for loop out of this
			block_number++;
			uint16_t checked_block = *(fat_page + block_number);
			if (checked_block == 0xFFFF) // consider 2 bytes at a time
				r--;
		}
		
		mf.flash_write((uint8_t *) (fat_page + block_number), (uint8_t *) &fat_entry, 2, NULL);
		fat_entry = block_number;

		//write actual data
		uint8_t *byte = byte_array + (i * BLOCK_SIZE);
		int byte_length = length - (i * BLOCK_SIZE);
		length -= byte_length;
		if (block_number > 300)
			block_number = block_number;
		mf.flash_write(&flash_address[0] + block_number * BLOCK_SIZE, byte, byte_length, getRandomScratch());
		//printf("END:FREE %d\n", free_blocks);
		free_blocks --;
	}

	return fat_entry;
}









FileDescriptor * MicroBitFileSystem::getFreeRootEntry(uint8_t *directory)
{
	bool fd_deleted = false;
	FileDescriptor *fd = getFirstFileDescriptor(directory);

	while (fd != NULL) {
		if (fd->flags & IS_FREE)
			return fd;

		if (~fd->flags & IS_VALID)
			fd_deleted = true;

		fd = getNextFileDescriptor(fd);
	}

	if (fd_deleted) { //defragment directory
		fd = getFirstFileDescriptor(directory);
		uint8_t *dir_head = ((uint8_t *) getFirstFileDescriptor(directory)) - DOT_DOT;
		
		uint8_t *scratch_page = getRandomScratch();
		
		uint8_t *read_head = 
		uint8_t *write_head;
		for (int i = 0; i < PAGE_SIZE / BLOCK_SIZE; i++) {
			if (dir_block != block_number)
				mf.write(scratch_page + (i * BLOCK_SIZE), getBlock(block_number), BLOCK_SIZE, NULL);
			else
				write_head = scratch_page + (i * BLOCK_SIZE);
		}
		
		while (fd != NULL) {
			if (fd->flags & IS_VALID) {
				mf.flash_write(write, fd, sizeof(FileDescriptor), NULL);
				write += 24;
				fd = nextFileDescriptor(fd);
			}
		}
	}
	else {
		uint16_t new_block = freeBlock();
	}

	//if free return fd
	
	//if not, check for deleted
		//if deleted, clear all blocks and rearrange entries
		//if no deleted allocate new block



	/*
	if (deleted_entry != NULL) {
		fd = root_page;

		for (int root_page_number = 0; root_page_number < 1; root_page_number++) { //todo number of root pages
			uint8_t *scratch_page = getRandomScratch();
			
			for (int i = 0; i < 42; i++) {
				fd = getRootEntry(i);

				if (fd[18] & 0x40) { //not deleted
					mf.flash_write(scratch_page + (i * 24), fd, 24, NULL);
				}
			}
			mf.erase_page((uint32_t *) root_page);
			mf.flash_write(root_page, scratch_page, PAGE_SIZE, NULL);
			mf.erase_page((uint32_t *) scratch_page);
		}
		return deleted_entry;
		//return first deleted entry
	}
	*/

	return NULL; //root directory full
}

int MicroBitFileSystem::clearFat()
{
	uint16_t *fat_entry = fat_page;
	uint8_t *scratch_page = getRandomScratch();
	int deleted_blocks = 0;

	for (int i = 0; i < PAGE_SIZE * FAT / 2 && i < FLASH_SPACE / BLOCK_SIZE; i++) {
		if (*fat_entry & UNUSED)
			mf.flash_write(scratch_page + i * 2, (uint8_t *)fat_entry, 2, getRandomScratch());
		else
			deleted_blocks++;
		fat_entry++;
	}

	mf.erase_page((uint32_t *)fat_page);
	mf.flash_write((uint8_t *)fat_page, scratch_page, PAGE_SIZE, NULL);
	mf.erase_page((uint32_t *)scratch_page);
	free_blocks += deleted_blocks;
	return 0;
}

uint8_t * MicroBitFileSystem::getRandomScratch()
{
	return &flash_address[0];
}


// Get file descriptor by name
FileDescriptor* MicroBitFileSystem::getFileDescriptor(char *file_name, uint8_t *directory)
{
	FileDescriptor *fd = (FileDescriptor *) directory;
	int name_length = strlen(file_name);
	
	if (name_length > 16)
		name_length = 16;
	
	int i = 0;
	while (fd != NULL) {
		if (~fd->flags & IS_FREE && fd->flags & IS_VALID) { // check if directory?
			if (!strncmp(file_name, (char *) fd, name_length))
				return i;
		}

		i++;
		fd = getNextFileDescriptor(fd);
	}

	return NULL;
}

/*
uint16_t MicroBitFileSystem::firstBlockOnPage(uint8_t *p)
{

}

uint16_t MicroBitFileSystem::getBlockByNumber(uint8_t *p)
{
	return (p - flash_address) / PAGE_SIZE;
}
*/

uint8_t* MicroBitFileSystem::getBlock(uint16_t block_number)
{
	return get(block_number, true);
}

uint16_t* MicroBitFileSystem::getFATEntry(uint16_t entry_index)
{
	return (uint16_t *)get(entry_index, false);
}

uint8_t* MicroBitFileSystem::get(uint16_t index, bool get_fat_entry)
{
	if (index > (fat_length * PAGE_SIZE / sizeof(uint16_t)) - META_DATA_LENGTH)
		return NULL;

	if (get_fat_entry)
		return (uint8_t *) (fat_page + index);
	else
		return flash_address + (index * BLOCK_SIZE);

}

FileDescriptor* MicroBitFileSystem::getFirstFileDescriptor(uint8_t *directory)
{
	return (FileDescriptor *) directory + DOT_DOT;
}

FileDescriptor* MicroBitFileSystem::getNextFileDescriptor(FileDescriptor * fd)
{
	int relative_address = ((uint8_t *) fd - flash_address);
	
	if (relative_address % BLOCK_SIZE < BLOCK_SIZE - sizeof(FileDescriptor))
		return fd++;

	uint16_t fat_entry_value = *getFATEntryByNumber(relative_address / BLOCK_SIZE);
	if (fat_entry_value == EOF || fat_entry_value == DELETED)
		return NULL;
	else
		return (FileDescriptor *) (getBlock(fat_entry_value) + DOT_DOT);
}




// Test function
#include <inttypes.h>
void MicroBitFileSystem::print()
{

	//print fat table
	uint16_t *fat_entry = fat_page;
	for (int i = 0; i < 240; i++) {
		if (*fat_entry == 0xFFFE)
			printf("EoF ");
		else if (*fat_entry == 0xFFFF)
			printf("    ");
		else if (*fat_entry == 0x0000)
			printf("--- ");
		else
			printf("x^x ");

		fat_entry++;
	}
	printf("free_blocks: %d\n\n\n", free_blocks);

	//print root page
	for (int i = 0; i < 42; i++) {
		FileDescriptor *fd = getRootEntry(i);
		if (fd->file_name[0] == -1)
			break;
		printf("%s ", fd->file_name);
		printf("%#04x ", fd->flags);
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
	FileDescriptor *fd = getRootEntry(getFileIndex(file_name));
	
	mf.flash_memset((uint8_t *) &fd->flags, fd->flags & ~IS_VALID, 2, NULL);

	//Mark fat entries, set to 0x0000
	uint16_t block_number = fd->first_block;
	while (1) {
		uint16_t *fat_entry = fat_page + block_number;
		block_number = *fat_entry;

		mf.flash_memset((uint8_t *) fat_entry, 0x00, 2, NULL);
		
		if (block_number == EOF) // Last fat entry deleted is end of file
			break;
	}
	
	//Return MICROBIT_OK on success, return ERROR otherwise
	
	return 1; // DELETE
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

