#include <stdio.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <sys/stat.h> 
int main() { 
	int fd, fd_dup, fd_dup2; 
	char buffer[100]; 
	struct stat fileInfo; 
	int n;
	
	/* 1. open() - Create and open a file */ 
	fd = open("sample.txt", O_CREAT | O_RDWR | O_TRUNC, 0644); 
	if (fd == -1) { 
		perror("opening file failure"); 
		return 1; 
	} 
	printf("1. File opened successfully.\n");
	printf(" File Descriptor = %d\n\n", fd);
	
	/* 2. write() - Write data into the file */ 
	write(fd, "Hello Linux File System Calls\n", 31); 
	printf("2. Data written successfully.\n"); 
	
	/* 3. lseek() - Move file pointer to beginning */ 
	lseek(fd, 0, SEEK_SET);
	printf("3. File offset moved to beginning.\n\n"); 
	
	/* 4. read() - Read data from the file */ 
	n = read(fd, buffer, sizeof(buffer) - 1); 
	if (n > 0) { 
		buffer[n] = '\0'; 
		printf("\n4. Data read from file:\n"); 
		printf(" %s\n", buffer);
	} 
	
	/* --- 5. dup() - Duplicate file descriptor --------- */ 
	fd_dup = dup(fd); 
	if (fd_dup == -1) { 
		perror("dup"); 
		close(fd); 
		return 1; 
	} 
	printf("5. dup() successful.\n"); 
	printf(" Original FD = %d\n", fd);
	
	/* ------ 6. dup2() - Duplicate fd to a specified descriptor ----- */ 
	fd_dup2 = dup2(fd, 10); 
	if (fd_dup2 == -1) { 
		perror("dup2"); 
		close(fd_dup); 
		close(fd_dup); 
		return 1; 
	} 
	printf("6. dup2() successful.\n"); 
	printf(" Original FD = %d\n", fd); 
	printf(" Specified Duplicate = %d\n\n", fd_dup2);
	
	/* ------ 7. write() using duplicated descriptor ----- */ 
	write(fd_dup, "Data written using dup()\n", 25); 
	printf("7. Data written using duplicated descriptor.\n\n");
	
	/* ---- 8. fsync() - Synchronize file data with storage ---- */ 
	if (fsync(fd) == 0) { 
		printf("8. fsync() successful.\n"); 
		printf(" File data synchronized with storage.\n\n"); 
	} 
	else { 
		perror("fsync"); 
	}
	
	/* 5. close() - Close the file */ 
	close(fd); 
	close(fd_dup); 
	close(fd_dup2);
	printf("\nFile closed successfully.\n"); 
	
	/* 6. stat() - Get file information */
	if (stat("sample.txt", &fileInfo) == 0) { 
		printf("\nFile Information:\n"); 
		printf("File size: %ld bytes\n", fileInfo.st_size); 
		printf("File permissions: %o\n", fileInfo.st_mode & 0777); 
	}
	
	/* ------ 10. chmod() - Change file permissions ---- */ 
	if (chmod("sample.txt", 0600) == 0) { 
		printf("10. chmod() successful.\n"); 
		printf(" File permissions changed to 600.\n\n"); 
	} 
	else { 
		perror("chmod"); 
	}
	
	/* --- 11. access() - Check file accessibility ------ */ 
	if (access("sample.txt", F_OK) == 0) { 
		printf("11. access() successful.\n"); 
		printf(" File exists.\n"); 
	} 
	if (access("sample.txt", R_OK) == 0) { 
		printf(" Read permission is available.\n");
	} 
	if (access("sample.txt", W_OK) == 0) { 
		printf(" Write permission is available.\n"); 
	} 
	printf("\n"); 
	
	/* ---- 12. truncate() - Change file size ------ */ 
	if (truncate("sample.txt", 10) == 0) { 
		printf("12. truncate() successful.\n"); 
		printf(" File size changed to 10 bytes.\n\n"); 
	} 
	else { 
		perror("truncate"); 
	}
	
	/* 7. unlink() - Delete the file */ 
	if (unlink("sample.txt") == 0) { 
		printf("File deleted successfully.\n"); 
	} 
	return 0;
}
