/* Name: Muskan Hans 
   Vnumber: V00871181
   Course: Csc 360
   Assignment: 3 , diskinfo.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/*Declaring the constants */

#define SECTOR_SIZE 512
#define ROOT_SIZE 224
#define LOGICAL_CLUSTER_FIELD 26
#define ATTRIBUTE_OFFSET 11
 
/*Declaring the functions used in this program here*/
void fetchDiskVolumeLable(char *diskVolumeLabel, char *memory_map_pointer);
int goThroughFATEntries(char *p, int n);
int totalNumOfFiles(char *memory_map_pointer);

/* Declaring variables */
int i,x,j,numOfFiles,first,second;
int temp = SECTOR_SIZE * 19; // to get into first sector of root directory

//main function for displaying the information about disk
int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Please use -> ./diskinfo <disk image>.IMA\n");
		exit(1);
	}

	// Open disk disk_image and map memory
	int disk_image = open(argv[1], O_RDWR);
	if (disk_image < 0){
		printf(" Unable to read the disk disk_image %s\n", argv[1]);
		exit(1);
	}

	struct stat buffer;
	fstat(disk_image, &buffer);

	char *memory_map_pointer = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, disk_image, 0);// memory_map_pointer is just a pointer that points to the first position of the mapped memory
	if (memory_map_pointer == MAP_FAILED){
		printf("unable to map memory\n");
		close(disk_image);
		exit(1);
	}
	// fetching OS name here
	char *OSName = malloc(sizeof(char));
	for (int j = 0; j < 8; j++){ // we need to read only 8 bytes.
		OSName[j] = memory_map_pointer[j+3]; // because the OS name starts at postion 3 and it's length is 8 bytes
	}

	// second we need to fetch the disk label from disk image. 
	char *diskVolumeLabel = malloc(sizeof(char));
	fetchDiskVolumeLable(diskVolumeLabel, memory_map_pointer); // calling the function here 

	// next we need to figure out the disk size
	// more significant byte(MSB) is 12 and less significant byte (LSB) is 11 for (bytes per sector)
	int bytes_in_each_sector =  (memory_map_pointer[12] << 8)+ memory_map_pointer[11] ; //starting byte for bytes per sector is 11

	// more significant byte(MSB) is 12 and less significant byte (LSB) is 11 for (bytes per sector)
	int total_num_sectors = (memory_map_pointer[20] << 8) + memory_map_pointer[19] ; // total sector count starting byte is 19
	int SizeOfDisk = bytes_in_each_sector * total_num_sectors; // this will be the total disk-size

	int freeSizeOfDisk = 0; //initially set it to 0

	// i starts from 2 because first two are reserved and loop is due to the formula: physical_sector_number = 33 + FAT entry number - 2
	for (int i = 2; i < (SizeOfDisk / SECTOR_SIZE) - 31; i++){
		if (goThroughFATEntries(memory_map_pointer, i) == 0x00){ // if the value of the FAT entry is 0x00 then that's an empty sector
		freeSizeOfDisk++;}
	}
	freeSizeOfDisk = SECTOR_SIZE * freeSizeOfDisk; //sector size is 512

	int numOfFiles = totalNumOfFiles(memory_map_pointer);

	// printing all the disk information
	printf("OS Name: %s\n", OSName);
	printf("label of the disk: %s\n", diskVolumeLabel);
	printf("Total size of the disk: %d bytes\n", SizeOfDisk);
	printf("Free size of the disk: %d bytes\n\n", freeSizeOfDisk);
	printf("==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n\n", numOfFiles);
	printf("==============\n");
	printf("Number of FAT copies: %d\n", memory_map_pointer[16]);
	printf("Sectors per FAT: %d\n", memory_map_pointer[22] + (memory_map_pointer[23] << 8));

	// Free variables
	munmap(memory_map_pointer, buffer.st_size);
	free(OSName);
	free(diskVolumeLabel);
	close(disk_image);
	return 0;
}

// this function is used to find the disk volume label from the disk image. 
void fetchDiskVolumeLable(char *diskVolumeLabel, char *memory_map_pointer){
	memory_map_pointer += temp; // make the pointer to jump to the first sector in the root directory
	while(memory_map_pointer[0]!= 0x00)	{ //iterate over the directory entries
		if(memory_map_pointer[11]==0x08){ // we know in th root directory the attributes ATTRIBUTE_OFFSET is 11 and the mask for volume label is 0x08. 
			// store the label 
			for(i=0; i<8 ; i++){
				diskVolumeLabel[i]= memory_map_pointer[i]; 
			}
		}
		memory_map_pointer+=32; //jump to next directory entry.
	}
}
// this functiomn will return the value of FAT table at the position of memory_map_pointer.
int goThroughFATEntries(char *memory_map_pointer, int i){
	if ((i % 2) != 0){
        first = memory_map_pointer[SECTOR_SIZE + (int)((3 * i) / 2)] & 240;
        second = memory_map_pointer[SECTOR_SIZE + (int)((3 * i) / 2) + 1] & 255;
        return (first >> 4) + (second << 4);
	}else{
        first = memory_map_pointer[SECTOR_SIZE + ((3 * i) / 2) + 1] & 15;
        second = memory_map_pointer[SECTOR_SIZE + ((3 * i) / 2)] & 255;
        return (first << 8) + second;
    }
}

//get total number of files present in the disk
int totalNumOfFiles(char *memory_map_pointer){
	memory_map_pointer += temp;
	numOfFiles = 0;
	// skip the following
	while (memory_map_pointer[0] != 0x00){ // unused
		if (memory_map_pointer[LOGICAL_CLUSTER_FIELD] == 0x00 && memory_map_pointer[LOGICAL_CLUSTER_FIELD] == 0x01){//the first logical cluster field is 0 or 1 
			if (memory_map_pointer[ATTRIBUTE_OFFSET] == 0x0F){ // he value of it’s atrribute field is 0x0F
				continue;  // we skip all above and continue
			}
		}
		// check the values for hidden, volume label and subdirectory attribute because theu won't be considerd as files. Otherise we increase the numOfFiles by 1
		if((memory_map_pointer[ATTRIBUTE_OFFSET] & 0x08) == 0 && (memory_map_pointer[ATTRIBUTE_OFFSET] & 0x10) == 0 && (memory_map_pointer[ATTRIBUTE_OFFSET] & 0x02) == 0){
			numOfFiles++;
		}
		memory_map_pointer += 32;  // to jump to another directory entry
	}
	return 	numOfFiles;
}