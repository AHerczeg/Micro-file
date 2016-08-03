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



#define FLASH_SPACE 61440
uint8_t flash_memory[FLASH_SPACE];

MicroBitFileSystem* MicroBitFileSystem::defaultFileSystem = NULL;


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
	root_dir = &(flash_memory[2 * PAGE_SIZE]);

	//todo count free blocks from fat table
	free_blocks = (FLASH_SPACE - (3 * PAGE_SIZE)) / BLOCK_SIZE;
	// occupy fat_entry with actual fat & root directory blocks

	uint16_t *fat_entry = fat_page;
	for (int i = 0; i < 9; i++) {
		*fat_entry = 0xFFFE;
		fat_entry++;
	}

	if (MicroBitFileSystem::defaultFileSystem == NULL)
		MicroBitFileSystem::defaultFileSystem = this;
}


int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length) {
	return create(file_name, byte_array, length, root_dir);
}

int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length, uint8_t *directory)
{
	int name_length = strlen(file_name);
	if (name_length == 0 || name_length > 16)
		return 1; // TODO filename length error

	if (fileDescriptorExists(file_name, directory)) //will check that both file and directory exists
		return 0; //todo return error code

	//check that directory exists
	//if (!fileDescriptorExists(directory, getParentDirectory(directory)))
		//return 0; //todo return error code

	//Check space left, return OUT_OF_SPACE if there's not enough space

	if (length > free_blocks * BLOCK_SIZE) {

		// Count deleted blocks
		int deleted_blocks = 0;
		uint16_t *table_entry;
		uint16_t i = 0;

		while (table_entry = getTableAddress(i)) {
			if (*table_entry == DELETED)
				deleted_blocks;
			i++;
		}

		if (length > (free_blocks + deleted_blocks) * BLOCK_SIZE)
			return 1;  // todo return error
		else
			clearFat();
	}
	else
		return 1; //todo error here

	//Find free file descriptor
	FileDescriptor *fd = getFirstFileDescriptor(directory);
	bool deleted_fds = false;
	while (fd) {
		if (fd->flags & DELETED)
			deleted_fds = true;
		else if (fd->flags & IS_FREE)
			break;
		fd = getNextFileDescriptor(fd);
	}

	//Write file to memory
	FileDescriptor fd = { "", this->write(byte_array, length), ~IS_FREE, length };
	memcpy((char *) &fd, file_name, 16);
	mf.flash_write((uint8_t *) fd, (uint8_t *) &fd, 24, getRandomScratch());

	return MICROBIT_OK;
}

uint16_t MicroBitFileSystem::write(uint8_t *byte_array, int length)
{
	int blocks_needed = length / BLOCK_SIZE + 1;

	if (length % BLOCK_SIZE == 0)
		blocks_needed--;

	uint16_t table_entry = EOF;

	for (int i = blocks_needed - 1; i >= 0; i--) {
		uint16_t block_number = getRandomFreeBlock();

		// Edit FAT Entry
		mf.flash_write((uint8_t *) getTableAddress(block_number), (uint8_t *) &table_entry, 2, NULL);
		table_entry = block_number;

		// Write data
		uint8_t *byte = byte_array + (i * BLOCK_SIZE);
		int byte_length = length - (i * BLOCK_SIZE);
		length -= byte_length;
		mf.flash_write(getBlockAddress(block_number), byte, byte_length, getRandomScratch());
		free_blocks --;
	}

	return table_entry;
}

bool MicroBitFileSystem::freeSpace(int length)
{
	if (free_blocks < length) {
		int deleted_blocks = 0;
		uint16_t *table_entry;
		uint16_t i = 0;

		while (table_entry = getTableAddress(i)) {
			if (*table_entry == DELETED)
				deleted_blocks;
			i++;
		}

		if (length > (free_blocks + deleted_blocks) * BLOCK_SIZE)
			return 1;  // todo return error
		else
			clearFat();
	}
	else
		return false;
}

uint16_t MicroBitFileSystem::getRandomFreeBlock()
{
	if (free_blocks) {
		uint16_t r = (free_blocks == 1) ? 1 : ((std::rand() % (free_blocks - 1)) + 1);
		int block_number = 0;

		while (r) {
			block_number++;
			if (*getTableAddress(block_number) == UNUSED)
				r--;
		}
		return (uint16_t) block_number;
	}
	return NULL;
}



FileDescriptor* MicroBitFileSystem::defragment(uint8_t *directory)
{
	FileDescriptor *fd = getFirstFileDescriptor(directory);
	uint8_t max_fds_to_block = BLOCK_SIZE / sizeof(FileDescriptor);
	uint8_t fds_copied = 0;

	uint8_t *current_block = (uint8_t *)fd;
	uint8_t *scratch_page = getRandomScratch();
	uint8_t *scratch_write_head = scratch_page + PAGE_OFFSET(current_block);

	while (fd) {
		// Create new scratch page when previous is full
		if (fds_copied == max_fds_to_block) {
			completePartialCopy(scratch_page, current_block);
			fds_copied = 0;
			scratch_page = getRandomScratch();
			scratch_write_head = scratch_page + PAGE_OFFSET(current_block);
		}

		// Copy valid file descriptor
		if (fd->flags & IS_VALID && ~fd->flags & IS_FREE) {
			mf.flash_write(scratch_write_head, (uint8_t *)fd, sizeof(FileDescriptor), NULL);
			scratch_write_head += sizeof(FileDescriptor);
			fds_copied++;
		}

		fd = getNextFileDescriptor(fd);
	}

	if (fds_copied)
		completePartialCopy(scratch_page, current_block);

	// If last block was full, leave one free block
	if (fds_copied == max_fds_to_block) {
		current_block = getNextBlock(current_block);
		completePartialCopy(getRandomScratch(), current_block);
	}

	// Cut off and mark rest of unused directory blocks as deleted
	uint16_t eof = EOF;
	mf.flash_write((uint8_t *) getTableAddress(current_block), (uint8_t *)&eof, 2, getRandomScratch());
	current_block = getNextBlock(current_block);

	while (current_block != NULL) {
		mf.flash_write((uint8_t *) getTableAddress(current_block), DELETED, 2, NULL);
		current_block = getNextBlock(current_block);
	}
}

inline void MicroBitFileSystem::completePartialCopy(uint8_t * scratch_page, uint8_t *block_to_skip)
{
	uint8_t *write_head = scratch_page;
	while (write_head < scratch_page + PAGE_SIZE) {
		if (PAGE_OFFSET(write_head) != PAGE_OFFSET(block_to_skip)) //Skip copying of original_block
			mf.flash_write(write_head, PAGE_START(block_to_skip) + PAGE_OFFSET(write_head), BLOCK_SIZE, NULL);
		write_head += BLOCK_SIZE;
	}

	//Copy scratch page back to original page
	mf.erase_page((uint32_t *) PAGE_START(block_to_skip));
	mf.flash_write(PAGE_START(block_to_skip), scratch_page, PAGE_SIZE, NULL);
	mf.erase_page((uint32_t *) scratch_page);
}



int MicroBitFileSystem::remove(char *file_name, uint8_t *directory) {
	//Find file, if not found return FILE_NOT_FOUND
	if (!fileDescriptorExists(file_name, directory))
		return 0; //todo microbit error code

	// Flip valid bit
	FileDescriptor *fd = findFileDescriptor(file_name, directory);

	mf.flash_memset((uint8_t *)&fd->flags, fd->flags & ~IS_VALID, 2, NULL);

	//Mark fat entries, set to 0x0000
	uint16_t block_number = fd->first_block;
	while (1) {
		uint16_t *fat_entry = fat_page + block_number;
		block_number = *fat_entry;

		mf.flash_memset((uint8_t *)fat_entry, 0x00, 2, NULL);

		if (block_number == EOF) // Last fat entry deleted is end of file
			break;
	}

	//Return MICROBIT_OK on success, return ERROR otherwise

	return 1; // DELETE
}

uint8_t * MicroBitFileSystem::getRandomScratch()
{
	return &flash_address[0];
}

int MicroBitFileSystem::clearFat()
{
	uint16_t *fat_entry = fat_page;
	uint8_t *scratch_page = getRandomScratch();
	int deleted_blocks = 0;

	for (int i = 0; i < PAGE_SIZE * fat_length / sizeof(uint16_t); i++) {
		if (*fat_entry & UNUSED)
			mf.flash_write(scratch_page + PAGE_OFFSET(fat_entry), (uint8_t *)fat_entry, 2, NULL);
		else
			deleted_blocks++;

		if (PAGE_OFFSET(fat_entry + 1) == 0) {
			mf.erase_page((uint32_t *)PAGE_START(fat_entry));
			mf.flash_write((uint8_t *)PAGE_START(fat_entry), scratch_page, PAGE_SIZE, NULL);
			mf.erase_page((uint32_t *)scratch_page);
			scratch_page = getRandomScratch();
		}

		fat_entry++;
	}

	free_blocks += deleted_blocks;
	return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////// Helper Functions


FileDescriptor* MicroBitFileSystem::getFirstFileDescriptor(uint8_t *directory)
{
	return (FileDescriptor *)directory + DOT_DOT;
}

FileDescriptor* MicroBitFileSystem::getNextFileDescriptor(FileDescriptor * fd)
{
	if (((int)fd % BLOCK_SIZE) + sizeof(FileDescriptor) < BLOCK_SIZE)
		return ++fd;

	uint16_t* fat_entry = getTableAddress((uint8_t *) fd);
	if (fat_entry != NULL && validBlockNumber(*fat_entry))
		return getFirstFileDescriptor(getBlockAddress(*fat_entry));

	return NULL;
}

bool MicroBitFileSystem::fileDescriptorExists(char *file_name, uint8_t *directory)
{
	if (findFileDescriptor(file_name, directory) != NULL)
		return true;
	else
		return false;
}

FileDescriptor* MicroBitFileSystem::findFileDescriptor(char *file_name, uint8_t *directory)
{
	FileDescriptor *fd = getFirstFileDescriptor(directory); //todo check if directory is valid
	int name_length = strlen(file_name);

	if (name_length > 16)
		name_length = 16;

	int i = 0;
	while (fd != NULL) {
		i++;
		if (~fd->flags & IS_FREE && fd->flags & IS_VALID) { // check if directory?
			if (!strncmp(file_name, (char *)fd, name_length))
				return fd;
		}
		fd = getNextFileDescriptor(fd);
	}

	return NULL;
}


inline uint8_t *MicroBitFileSystem::getNextBlock(uint8_t *block_address)
{
	uint16_t *fat_entry = getTableAddress(block_address);
	if (fat_entry != NULL && validBlockNumber(*fat_entry))
		return getBlockAddress(*fat_entry);

	return NULL;
}

inline uint16_t* MicroBitFileSystem::getTableAddress(uint8_t *block_address)
{
	return getTableAddress((block_address - flash_address) / BLOCK_SIZE);
}

inline uint16_t* MicroBitFileSystem::getTableAddress(uint16_t block_number)
{
	if (validBlockNumber(block_number))
		return fat_page + block_number * sizeof(uint16_t);

	return NULL;
}

inline uint8_t* MicroBitFileSystem::getBlockAddress(uint16_t block_number)
{
	if (validBlockNumber(block_number))
		return flash_address + (block_number * BLOCK_SIZE);

	return NULL;
}

inline bool MicroBitFileSystem::validBlockNumber(uint16_t block_number)
{
	if (block_number <= (fat_length * PAGE_SIZE / sizeof(uint16_t)) - META_DATA_LENGTH)
		return true;

	return false;
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
	/*
	for (int i = 0; i < 42; i++) {
		FileDescriptor *fd = getRootEntry(i); // Change
		if (fd->file_name[0] == -1)
			break;
		printf("%s ", fd->file_name);
		printf("%#04x ", fd->flags);
		printf("\n");
	}
	*/

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
