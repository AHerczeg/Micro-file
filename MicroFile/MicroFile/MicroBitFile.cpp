int MicroBitFileSystem::open(char *fileName, uint8_t flags) {
	//Find file, if not found return -1
	//Check file flags, if we can't modify, return 2
	//Set file flags
	//TODO: define fd_table
	//Return FileDescriptor fd on success, return ERRROR otherwise

	return 0; // DELETE
}

int MicroBitFileSystem::close(int fd) {
	// Check if fd valid, if not return INVALID_FD
	//Find fd, release flags
	//Return 1 on success, return ERROR

	return 0; // DELETE
}

int MicroBitFileSystem::seek(int fd, int offset, uint8_t flags) {
	//Check if fd valid, if not return INVALID_FD
	//

	return 0; // DELETE
}

int MicroBitFileSystem::read() {
	//

	return 0; // DELETE
}

int MicroBitFileSystem::write() {
	//

	return 0; // DELETE
}