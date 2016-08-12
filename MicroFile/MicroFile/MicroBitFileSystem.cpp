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



#define FLASH_SPACE 300000
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
	flash_address = &flash_memory[0];
	fat_page = (uint16_t *) &(flash_memory[1 * PAGE_SIZE]);
	fat_length = 2;
	
	fat_page[0] = 0xBEEF;
	fat_page[1] = 2;
	fat_page[2] = 256;

	fat_page += 3;
	root_dir = &(flash_memory[(3 * PAGE_SIZE)]);

	//todo count free blocks from fat table
	free_blocks = (FLASH_SPACE / BLOCK_SIZE) - 13;
	// occupy fat_entry with actual fat & root directory blocks

	uint16_t *fat_entry = fat_page;
	for (int i = 0; i < 13; i++) {
		*fat_entry = 0xFFFE;
		fat_entry++;
	}

	if (MicroBitFileSystem::defaultFileSystem == NULL)
		MicroBitFileSystem::defaultFileSystem = this;
}

int MicroBitFileSystem::createDirectory(char *name, char *target_directory)
{
	if (uint8_t *dir = getDirectory(target_directory)) {
		FileDescriptor *fd;
		int free_space = freeSpace(1);

		if (free_space < 2) return 0; //todo error code
		else if (free_space == 2) fd = newFileDescriptor(dir, false);
		else fd = newFileDescriptor(dir, true);
			
		uint16_t dir_block = getRandomFreeBlock();

		FileDescriptor new_fd = { "", dir_block, ~IS_FREE, UNUSED };
		memcpy((char *)&new_fd.file_name, name, 16);
		mf.flash_write((uint8_t *) fd, (uint8_t *) &new_fd, sizeof(FileDescriptor), getRandomScratch());

		uint16_t eof = EOF;
		mf.flash_write((uint8_t *)getTableAddress(dir_block), (uint8_t *)&eof, 2, NULL);
		//write dir block meta data
	}
	return 0; //todo error code here
}

int MicroBitFileSystem::createFile(char *file_name, uint8_t *byte_array, int length, char *directory)
{
	//todo appropriate error codes on return
	int name_length = strlen(file_name);

	if (name_length == 0 || name_length > 16) 
		return 0;

	uint8_t *dir = (directory == NULL) ? root_dir : getDirectory(directory);
	if (!dir) 
		return 0;

	if (findFileDescriptor(file_name, false, dir)) 
		return 0;

	if (!byte_array) 
		length = 1;

	uint16_t free_space = freeSpace(length);
	if (!free_space) 
		return 0;

	FileDescriptor *fd;
	if (free_space - 1)
		fd = newFileDescriptor(dir, true);
	else
		fd = newFileDescriptor(dir, false);

	if (!fd)
		return 0;


	FileDescriptor temp_fd = { "", UNUSED, ~IS_FREE & ~IS_DIRECTORY, UNUSED };
	memcpy((char *)&temp_fd.file_name, file_name, 16);

	if (byte_array) {
		temp_fd.length = length;
		temp_fd.first_block = write(byte_array, length);
	}
	else {
		temp_fd.first_block = getRandomFreeBlock();
		mf.flash_write((uint8_t *)getTableAddress(temp_fd.first_block), (uint8_t *)&temp_fd.first_block, 2, NULL);
	}

	mf.flash_write((uint8_t *) fd, (uint8_t *)&temp_fd, sizeof(FileDescriptor), getRandomScratch());

	return MICROBIT_OK;
}


FileDescriptor *MicroBitFileSystem::newFileDescriptor(uint8_t *directory, bool allow_expand)
{
	FileDescriptor *fd = getFirstFileDescriptor(directory);
	FileDescriptor *last_fd = fd;

	bool deleted_fds = false;
	
	while (fd) {
		if (~fd->flags & IS_VALID)
			deleted_fds = true;
		else if (fd->flags & IS_FREE)
			return fd;

		last_fd = fd;
		fd = getNextFileDescriptor(fd);
	}

	if (deleted_fds)
		return clearDirectory(directory);
	 
	else if (allow_expand) {
		uint16_t new_block = getRandomFreeBlock();
		uint8_t *new_block_addr = getBlockAddress(new_block);
		uint16_t eof = EOF;

		mf.flash_write((uint8_t *) getTableAddress((uint8_t *) last_fd), (uint8_t *) &new_block, 2, getRandomScratch());
		mf.flash_write((uint8_t *) getTableAddress(new_block), (uint8_t *) &eof, 2, NULL);
		//mf.flash_write(new_block_addr, (uint8_t *)getTableAddress(((uint8_t *)last_fd)), 2, NULL); todo do we need dotdot?
		return getFirstFileDescriptor(new_block_addr);
	}

	return NULL;
}

uint16_t MicroBitFileSystem::write(uint8_t *byte_array, int length)
{
	int blocks_needed = length / BLOCK_SIZE + (length % BLOCK_SIZE == 0 ? 0 : 1);

	uint16_t table_entry = EOF;

	for (int i = blocks_needed - 1; i >= 0; i--) {
		uint16_t block_number = getRandomFreeBlock();
		uint16_t table_value = *getTableAddress(block_number);
		// Edit Table Entry
		mf.flash_write((uint8_t *) getTableAddress(block_number), (uint8_t *) &table_entry, 2, NULL);
		table_entry = block_number;

		// Write data
		uint8_t *byte = byte_array + (i * BLOCK_SIZE);
		int byte_length = length - (i * BLOCK_SIZE);
		length -= byte_length;
		mf.flash_write(getBlockAddress(block_number), byte, byte_length, getRandomScratch());
	}

	return table_entry;
}

int MicroBitFileSystem::freeSpace(int size)
{
	int blocks_needed = size / BLOCK_SIZE + (size % BLOCK_SIZE == 0 ? 0 : 1);

	if (blocks_needed < free_blocks)
		return (free_blocks - blocks_needed);

	int deleted_blocks = 0;
	uint16_t *table_entry;
	uint16_t i = 0;

	while (table_entry = getTableAddress(i)) {
		if (*table_entry == DELETED)
			deleted_blocks;
		i++;
	}

	if (free_blocks + deleted_blocks < blocks_needed)
		return 0;

	// Clear table for deleted entries
	else {
		free_blocks += deleted_blocks;
		uint8_t *scratch_page = getRandomScratch();
		
		// Copy meta data
		table_entry = PAGE_START(getTableAddress(i));
		for (i = 0; i < META_DATA_LENGTH; i++) {
			mf.flash_write(scratch_page, (uint8_t *) table_entry, 2, NULL);
			table_entry++;
		}

		i = 0;
		table_entry = getTableAddress(i);		
		
		// Copy valid table entries
		while (table_entry) {
			if (PAGE_OFFSET(table_entry) == 0) {
				mf.erase_page((uint32_t *) PAGE_START(table_entry));
				mf.flash_write((uint8_t *) PAGE_START(table_entry), scratch_page, PAGE_SIZE, NULL);
				mf.erase_page((uint32_t *) scratch_page);
				scratch_page = getRandomScratch();
			}

			if (*table_entry != UNUSED)
				mf.flash_write(scratch_page + PAGE_OFFSET(table_entry), (uint8_t *) table_entry, 2, NULL);

			table_entry = getTableAddress(i++);
		}

		return (free_blocks + deleted_blocks - blocks_needed);
	}
}

uint16_t MicroBitFileSystem::getRandomFreeBlock()
{
	if (free_blocks) {
		uint16_t r = (free_blocks == 1) ? 1 : ((std::rand() % (free_blocks - 1)) + 1);
		int block_number = 0;

		while (1) {
			if (validBlockNumber(block_number) && *getTableAddress(block_number) == UNUSED)
				r--;
			if (!r)
				break;
			block_number++;
		}
		free_blocks--;

		return (uint16_t) block_number;
	}
	return NULL;
}


FileDescriptor* MicroBitFileSystem::clearDirectory(uint8_t *directory)
{
	directory = directory - ((directory - flash_address) % BLOCK_SIZE);
	FileDescriptor *fd = getFirstFileDescriptor(directory);
	uint8_t max_fds_to_block = BLOCK_SIZE / sizeof(FileDescriptor);
	uint8_t fds_copied = 0;

	uint8_t *current_block = directory; //todo replace with directory, this variable is unnecessary
	uint8_t *scratch_page = getRandomScratch();
	int x = PAGE_OFFSET(current_block) + DOT_DOT;
	int y = PAGE_OFFSET(scratch_page);
	uint8_t *scratch_write_head = scratch_page + PAGE_OFFSET(current_block) + DOT_DOT;

	while (fd) {
		// Create new scratch page when previous is full
		if (fds_copied == max_fds_to_block) {
			copyNeighboringBlocks(scratch_page, current_block);
			fds_copied = 0;
			scratch_page = getRandomScratch();
			scratch_write_head = scratch_page + PAGE_OFFSET(current_block) + DOT_DOT;
		}

		// Copy valid file descriptor
		if (fd->flags & IS_VALID && ~fd->flags & IS_FREE) {
			mf.flash_write(scratch_write_head, (uint8_t *) fd, sizeof(FileDescriptor), NULL);
			scratch_write_head += sizeof(FileDescriptor);
			fds_copied++;
		}

		fd = getNextFileDescriptor(fd);
	}

	// Copy remaining fds on scratch page
	copyNeighboringBlocks(scratch_page, current_block);

	// If last block was full, leave one free block
	if (fds_copied == max_fds_to_block) {
		current_block = getNextBlock(current_block);
		copyNeighboringBlocks(getRandomScratch(), current_block);
	}

	//Get free file descriptor
	fd = getFirstFileDescriptor(current_block);
	FileDescriptor *free_fd = fd;
	while (fd) {
		if (fd->flags & IS_FREE) {
			free_fd = fd;
			break;
		}
		fd = getNextFileDescriptor(fd);
	}

	// Cut off and mark rest of unused directory blocks as deleted
	uint16_t eof = EOF;
	mf.flash_write((uint8_t *) getTableAddress(current_block), (uint8_t *)&eof, 2, getRandomScratch());
	current_block = getNextBlock(current_block);


	while (current_block) {
		mf.flash_write((uint8_t *) getTableAddress(current_block), DELETED, 2, NULL);
		current_block = getNextBlock(current_block);
	}

	return free_fd;
}

inline void MicroBitFileSystem::copyNeighboringBlocks(uint8_t * scratch_page, uint8_t *target_block)
{
	uint8_t *write_head = scratch_page;

	while (write_head < scratch_page + PAGE_SIZE) {
		int x = PAGE_OFFSET(write_head);
		int y = PAGE_OFFSET(target_block);

		if (PAGE_OFFSET(write_head) != PAGE_OFFSET(target_block)) //Skip copying of original_block
			mf.flash_write(write_head, PAGE_START(target_block) + PAGE_OFFSET(write_head), BLOCK_SIZE, NULL);
		else // Copy .. (parent block address)
			mf.flash_write(scratch_page + PAGE_OFFSET(target_block), target_block, DOT_DOT, NULL);
		write_head += BLOCK_SIZE;
	}

	//Copy scratch page back to original page
	mf.erase_page((uint32_t *) PAGE_START(target_block));
	mf.flash_write(PAGE_START(target_block), scratch_page, PAGE_SIZE, NULL);
	mf.erase_page((uint32_t *) scratch_page);
}

int MicroBitFileSystem::remove(char *file_name)
{
	FileDescriptor *fd = findFileDescriptor(file_name, false, root_dir);
		if (!fd)
			return 0;//todo error code
	return remove(fd);
}

int MicroBitFileSystem::remove(char *file_name, char *directory)
{
	uint8_t *dir = getDirectory(directory);
	if (!dir)
		return 0; //todo error code goez here

	FileDescriptor *fd = findFileDescriptor(file_name, false, root_dir);
	if (!fd)
		return 0;//todo error code

	return remove(fd);
}

int MicroBitFileSystem::remove(FileDescriptor *fd) {
	uint16_t flags = fd->flags & ~IS_VALID;
	mf.flash_write((uint8_t *)&fd->flags, (uint8_t *)&flags, sizeof(fd->flags), NULL);
	deleteTableEntries(fd->first_block);

	return MICROBIT_OK;
}


int MicroBitFileSystem::removeDirectory(char *folder_name, char *directory)
{
	uint8_t *dir = (directory == NULL) ? root_dir : getDirectory(directory);
	if (!dir) return 0; //todo error code
	return removeDirectory(folder_name, dir);
}

int MicroBitFileSystem::removeDirectory(char *folder_name, uint8_t *directory)
{
	FileDescriptor *fd = findFileDescriptor(folder_name, true, directory);
	if (!fd)
		return 0;//todo error code
	
	remove(fd);

	//Remove all files and subdirectories
	return repeat(getFirstFileDescriptor(getBlockAddress(fd->first_block)));
}

int MicroBitFileSystem::repeat(FileDescriptor *fd)
{
	while (fd) {
		if (~fd->flags & IS_FREE && fd->flags & IS_VALID) { // todo check logic
			if (fd->flags & IS_DIRECTORY)
				repeat(getFirstFileDescriptor(getBlockAddress(fd->first_block)));
			remove(fd);
		}
		fd = getNextFileDescriptor(fd);
	}
	return 0;
}

void MicroBitFileSystem::deleteTableEntries(uint16_t block_number)
{
	do {
		uint16_t *table_entry = getTableAddress(block_number);
		block_number = *table_entry;
		mf.flash_memset((uint8_t *)table_entry, DELETED, sizeof(uint16_t), NULL);
	} while (block_number != EOF);
}

uint8_t* MicroBitFileSystem::getRandomScratch()
{
	//clean scratch page
	uint8_t step = PAGE_SIZE / BLOCK_SIZE;
	uint16_t i = 0; //todo fix rand logic
	int r = (std::rand() % (free_blocks / step)) + 1; //possible race condition when scratch is destination we want to write to?

	while (r) {
		bool free = true;
		for (uint8_t j = 0; j < step; j++) {
			if (fat_page[i + j] != UNUSED && fat_page[i + j] != DELETED) //todo mark reserved scratch page
				free = false;
		}
		if (free)
			r--;

		if (!validBlockNumber(i + step))
			i = 0;
		else
			i += step;
	}
	i++; //becase 0x00 is deleted and not fat entry
	mf.erase_page((uint32_t *)PAGE_START(getBlockAddress(i)));

	int x = PAGE_OFFSET(getBlockAddress(i));
	return getBlockAddress(i);
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////// Helper Functions

/*
inline uint8_t* MicroBitFileSystem::getParentDirAddress(FileDescriptor *fd)
{
	return getParentDirAddress((uint8_t *)fd);
}
inline uint8_t* MicroBitFileSystem::getParentDirAddress(uint8_t *directory)
{
	return getBlockAddress((uint16_t) *PAGE_START(directory));
}
*/

inline FileDescriptor* MicroBitFileSystem::getFirstFileDescriptor(uint8_t *directory)
{
	return (FileDescriptor *) ((directory - ((directory - flash_address)  % BLOCK_SIZE)) + DOT_DOT);
}

FileDescriptor* MicroBitFileSystem::getNextFileDescriptor(FileDescriptor *fd)
{
	if ((((uint8_t *)(fd + 1) - flash_address) % BLOCK_SIZE) < BLOCK_SIZE - sizeof(FileDescriptor))
		return ++fd;

	uint16_t* fat_entry = getTableAddress((uint8_t *) fd);
	if (fat_entry != NULL && validBlockNumber(*fat_entry))
		return getFirstFileDescriptor(getBlockAddress(*fat_entry));

	return NULL;
}


FileDescriptor *MicroBitFileSystem::findFileDescriptor(char *file_name, bool is_directory, uint8_t *directory)
{
	uint16_t flags = IS_VALID;

	if (is_directory)
		flags |= IS_DIRECTORY;

	uint8_t name_length = strlen(file_name);
	
	if (name_length == 0)
		return NULL;

	if (name_length > 16)
		name_length = 16;

	FileDescriptor *fd = getFirstFileDescriptor(directory);
	int i = 0;
	
	while (fd != NULL) {
		i++;
		if (~fd->flags & IS_FREE && fd->flags & flags) {
			if (!strncmp(file_name, fd->file_name, name_length)) // handle strings without null characters
				return fd;
		}
		fd = getNextFileDescriptor(fd);
	}

	return NULL;
}

uint8_t *MicroBitFileSystem::getDirectory(char *path)
{
	if (!path) return root_dir; //todo remove higher level checks doing the same thing
	//todo test with combinations of slashes
	char s[17];

	uint8_t depth = 1;
	uint8_t i = 0;
	uint8_t *current_dir = root_dir;
	FileDescriptor *fd;

	while (*path != '\0') {
		if (*path == '/') {
			s[i] = '\0';
			if (0 < i && (i > 17 || !(fd = findFileDescriptor((char *)&s, true, current_dir)))) return NULL;
			i = 0;
			current_dir = getBlockAddress(fd->first_block);
			
			if (++depth == MAX_DEPTH) return NULL;
		}
		else
			s[i++] = *path;
		path++;
	}

	s[i] = '\0';
	printf("depth: %d\n", depth);
	if (!(fd = findFileDescriptor((char *)&s, true, current_dir))) return NULL;
	return getBlockAddress(fd->first_block);
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
	return getTableAddress(((block_address - flash_address) / BLOCK_SIZE) + 1);
}

inline uint16_t* MicroBitFileSystem::getTableAddress(uint16_t block_number)
{
	if (validBlockNumber(block_number))
		return fat_page + (block_number - 1);

	return NULL;
}

inline uint8_t* MicroBitFileSystem::getBlockAddress(uint16_t block_number)
{
	if (validBlockNumber(block_number))
		return flash_address + ((block_number - 1) * BLOCK_SIZE);

	return NULL;
}

inline bool MicroBitFileSystem::validBlockNumber(uint16_t block_number)
{
	if (block_number > DELETED && block_number < EOF &&
		block_number <= (fat_length * PAGE_SIZE / sizeof(uint16_t)) - META_DATA_LENGTH)
		return true;

	return false;
}









// Test function
#include <inttypes.h>
void MicroBitFileSystem::print()
{

	//print fat table

	uint16_t *fat_entry = fat_page;
	for (int i = 0; i < (fat_length * (PAGE_SIZE / sizeof(uint16_t)) - META_DATA_LENGTH); i++) {
		if (*fat_entry == 0xFFFE)
			printf("%d:EoF ", i+1);
		else if (*fat_entry == 0xFFFF)
			printf("    ");
		else if (*fat_entry == 0x0000)
			printf("%d:--- ", i+1);
		else
			printf("%d:%d ", i+1, *fat_entry);

		fat_entry++;
	}
	printf("\nfree_blocks: %d\n\n\n", free_blocks);
	
	
	//print structure
	printDir(12+1, 0);

	/*
	FileDescriptor *fd = getFirstFileDescriptor(root_dir);
	while (fd) {
		for (int i = 0; i < 16; i++) {
			if (fd->file_name[i] == '\0')
				break;
			printf("%c", fd->file_name[i]);
		}
		//printf("%d\n", (99/100));
		//printf(" %d, block: %d\n", fd->flags, ((uint8_t *)fd - flash_address) / BLOCK_SIZE);
		fd = getNextFileDescriptor(fd);
	}
	*/
}


void MicroBitFileSystem::printDir(uint16_t block_number, int level)
{
	FileDescriptor *fd = getFirstFileDescriptor(getBlockAddress(block_number));
	
	int j = 0;
	while (fd) {
		printf("\n");
		for (int i = 0; i < level; i++) {
			printf("  ");
		}
		printf("%d)|_", j++);
		for (int i = 0; i < 16; i++) {
			if (fd->file_name[i] == '\0')
				break;
			printf("%c", fd->file_name[i]);
		}

		if (fd->flags & IS_FREE)
			printf(" 1");
		else
			printf(" 0");
		if (fd->flags & IS_VALID)
			printf("1");
		else
			printf("0");
		if (fd->flags & IS_DIRECTORY)
			printf("1");
		else
			printf("0");

		if (~fd->flags & IS_FREE && fd->flags & IS_DIRECTORY)
			printDir(fd->first_block, (level + 1));

		fd = getNextFileDescriptor(fd);
	}
}
