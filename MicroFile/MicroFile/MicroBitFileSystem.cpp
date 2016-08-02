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
}


int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length) {
	return create(file_name, byte_array, length, root_dir);
}

int MicroBitFileSystem::create(char *file_name, uint8_t *byte_array, int length, uint8_t *directory)
{
	int name_length = strlen(file_name);
	if (name_length > 16)
		return 1; // TODO filename too long error

	//todo check if directory exists

	if (this->fileDescriptorExists(file_name, directory))
		return 0; //todo return error message

	//Get space for file descriptor or error if memory is full
	FileDescriptor *free_fd = this->getFreeFileDescriptor(directory);
	if (free_fd == NULL)
		return 0; //todo insert error message here

	//Check space left, return OUT_OF_SPACE if there's not enough space
	if (length > free_blocks * BLOCK_SIZE) {
		int deleted_blocks = 0;
		uint16_t *fat_entry = fat_page;

		// Check that fat entry pointer stays within our fat page/s
		while (fat_entry < (fat_page - META_DATA_LENGTH) + fat_length * PAGE_SIZE) {
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
	mf.flash_write((uint8_t *) free_fd, (uint8_t *) &fd, 24, getRandomScratch());

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
			uint16_t checked_block = *getFATEntry(getBlock(block_number));
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
		mf.flash_write(getBlock(block_number), byte, byte_length, getRandomScratch());
		free_blocks --;
	}

	return fat_entry;
}






/*	bool fd_deleted = false;
	FileDescriptor *fd = getFirstFileDescriptor(directory);

	while (fd != NULL) {
		if (fd->flags & IS_FREE)
			return fd;

		if (~fd->flags & IS_VALID)
			fd_deleted = true;

		fd = getNextFileDescriptor(fd);
	}
*/


FileDescriptor * MicroBitFileSystem::defragmentDirectory(uint8_t *directory)
{
	// copy surrounding blocks
	// copy all non-deleted entries to scratch page
	// when full or no more entries exists copy page back from scratch and clear scratch
	// if was full, continue process for the next directory block
	// if no more entries, get next (and the rest) directory block and mark as deleted in fat table

	FileDescriptor *fd = getFirstFileDescriptor(directory); //pointer to fd
	uint8_t *current_dir_block = ((uint8_t *) fd) - DOT_DOT;

	uint8_t *scratch_page = getRandomScratch(); //might not need this, if we can use write = PAGE_START(write)
	uint8_t *scratch_write_head = scratch_page + PAGE_OFFSET(current_dir_block);
	uint8_t max_fds_in_block = BLOCK_SIZE / sizeof(FileDescriptor);
	uint8_t fds_written_in_block = 0;

	//Copy dot..dot block address
	mf.flash_write(scratch_write_head, current_dir_block, DOT_DOT, NULL);
	scratch_write_head += DOT_DOT;

	while (fd != NULL) {
		//Copy valid file descriptor
		if (fd->flags & IS_VALID && ~fd->flags & IS_FREE) {
			mf.flash_write(scratch_write_head, (uint8_t *) fd, sizeof(FileDescriptor), NULL);
			scratch_write_head += sizeof(FileDescriptor);
			fds_written_in_block++;
		}

		fd = getNextFileDescriptor(fd);

		//Block on scratch page is filled up or no more fds in directory;
		if (fds_written_in_block >= max_fds_in_block || fd == NULL) {
			scratch_write_head = scratch_page;

			//Copy surrounding blocks on original page to scratch
			while (scratch_write_head < scratch_page + PAGE_SIZE) {
				if (PAGE_OFFSET(scratch_write_head) != PAGE_OFFSET(current_dir_block)) //skip copying of current_dir_block
					mf.flash_write(scratch_write_head, PAGE_START(current_dir_block) + PAGE_OFFSET(scratch_write_head), BLOCK_SIZE, NULL);
				scratch_write_head += BLOCK_SIZE;
			}

			//Copy scratch page back to original page
			mf.erase_page((uint32_t *) PAGE_START(current_dir_block));
			mf.flash_write(PAGE_START(current_dir_block), scratch_page, PAGE_SIZE, NULL);
			mf.erase_page((uint32_t *) scratch_page);

			//If more file descriptors to copy, get next directory block to fill up
			//and prepare new scratch page
			if (fd != NULL) {
				current_dir_block = getBlock(*getFATEntry(current_dir_block));
				scratch_page = getRandomScratch();
				scratch_write_head = scratch_page + PAGE_OFFSET(current_dir_block);
				fds_written_in_block = 0;
			}
		}
	}

	

	//Mark FAT entries as deleted
	uint16_t *fat_entry = getFATEntry(current_dir_block);
	uint16_t fat_value = *fat_entry;
	
	uint16_t eof = EOF;
	mf.flash_write((uint8_t *) fat_entry, (uint8_t *) &eof, 2, getRandomScratch()); //todo; does a 16 bit hex work here?
	
	while (fat_value != EOF) {
		fat_entry = getFATEntry(getBlock(fat_value));
		fat_value = *fat_entry;
		mf.flash_memset((uint8_t *) fat_entry, DELETED, 2, NULL);
	}
	//if (fds_written_in_block == max_fds_in_block)

	return 0;
}


uint8_t * MicroBitFileSystem::getRandomScratch()
{
	return &flash_address[0];
}


bool MicroBitFileSystem::fileDescriptorExists(char *file_name, uint8_t *directory)
{
	if (getFileDescriptor(file_name, directory) != NULL)
		return true;
	else
		return false;
}

// Get file descriptor by name
FileDescriptor* MicroBitFileSystem::getFileDescriptor(char *file_name, uint8_t *directory)
{
	FileDescriptor *fd = getFirstFileDescriptor(directory); //todo check if directory is valid
	int name_length = strlen(file_name);

	if (name_length > 16)
		name_length = 16;

	int i = 0;
	while (fd != NULL) {
		i++;
		if (~fd->flags & IS_FREE && fd->flags & IS_VALID) { // check if directory?
			if (!strncmp(file_name, (char *) fd, name_length))
				return fd;
		}
		fd = getNextFileDescriptor(fd);
	}

	return NULL;
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
			mf.erase_page((uint32_t *) PAGE_START(fat_entry));
			mf.flash_write((uint8_t *) PAGE_START(fat_entry), scratch_page, PAGE_SIZE, NULL);
			mf.erase_page((uint32_t *) scratch_page);
			scratch_page = getRandomScratch();
		}

		fat_entry++;
	}

	free_blocks += deleted_blocks;
	return 0;
}


uint16_t* MicroBitFileSystem::getNextFATEntry(uint16_t *address)
{

}

uint16_t MicroBitFileSystem::getBlockNumber(uint8_t *address) //todo remove function?
{
  return (address - flash_address) / BLOCK_SIZE;
}


uint8_t* MicroBitFileSystem::getBlock(uint16_t block_number) //todo rename to ..getBlockAddress
{
	return get(block_number, false);
}


uint16_t* MicroBitFileSystem::getFATEntry(uint8_t *address) //todo rename to:
{
	return (uint16_t *)get(getBlockNumber(address), true);
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
	if (((int) fd % BLOCK_SIZE) + sizeof(FileDescriptor) < BLOCK_SIZE)
		return ++fd;

	uint16_t fat_entry_value = *getFATEntry((uint8_t *) fd); //not going to return null, as fd's block exists
	if (fat_entry_value == EOF || fat_entry_value == DELETED)
		return NULL;
	else
		return (FileDescriptor *) (getBlock(fat_entry_value) + DOT_DOT);
}




int MicroBitFileSystem::remove(char *file_name, uint8_t *directory) {
	//Find file, if not found return FILE_NOT_FOUND
	if (!fileDescriptorExists(file_name, directory))
		return 0; //todo microbit error code

	// Flip valid bit
	FileDescriptor *fd = getFileDescriptor(file_name, directory);

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
