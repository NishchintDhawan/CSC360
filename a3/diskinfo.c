#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512

/*Store the location of FAT table for easy accessing*/
unsigned char *fat_table_location;

/*map memory of disk.*/
unsigned char *p;

/*Get the FAT entry values for calculating the free space.*/
int get_free_space_FAT_entry(unsigned char *p, int n);

/*Get the FAT entry values for directory parsing.*/
int get_FAT_entry(unsigned char *p, int n);

/*Print the name of the OS.*/
void print_OS_Name(unsigned char *osName)
{
    /*Read values from boot sector*/
    for (int i = 0; i < 8; i++)
    {
        osName[i] = p[i + 3];
    }

    /*Print the name*/
    printf("OS Name: %s\n", osName);
}

/*Print the label of the disk.*/
void print_diskLabel(unsigned char *diskLabel)
{
    // char extension[3];  /*get the extension values of disk label*/
    /*Create a temp pointer*/
    unsigned char *temp = p;
    //go to root directory.
    temp += SECTOR_SIZE * 19;

    //search the entries. If first byte is 0, then all available entries visited.
    while (temp[0] != 0x00)
    {
        //if 11th byte has 3rd bit set, it is the volume label.
        if ((temp[11] != 0x0F) && ((temp[11] & 0x08) == 0x08))
        {
            int i = 0;
            // get the filename
            for (i = 0; i < 8; i++)
            { //store byte data into diskLabel.
                diskLabel[i] = temp[i];
            }
            //get the extension
            // for (i = 0; i < 3; i++)
            // {
            //     extension[i] = temp[i];
            // }
        }
        temp += 32;
    }

    // if (extension[0]!= ' ')
    // {
    //     printf("Label of the disk: \"%s.%s\"\n", diskLabel, extension);
    // }
    // else
    // {
    printf("Label of the disk: %s\n", diskLabel);
    // }
}

/*Get the total size of the disk.*/
int totalDiskSize()
{
    //defined as number of sectors * total sector count.
    //endianness in order of bytes. move MSByte by 8 places to left and then add LSByte.
    int bytesPerSector = p[11] + (p[12] << 8);   // bytes 11-12
    int totalSectorCount = p[19] + (p[20] << 8); // bytes 19-20
    return bytesPerSector * totalSectorCount;
}

/*Count the empty FAT entries*/
int countEmptyFATentries()
{
    int freeSizeOfDisk = 0;
    int SizeOfDisk = totalDiskSize();
    /*Check the values in the FAT sectors*/
    for (int i = 2; i < (SizeOfDisk / SECTOR_SIZE) - 31; i++)
    {
        // Check if the FAT entry is 0x00 (empty sector)
        if (get_free_space_FAT_entry(p, i) == 0x00)
        {
            freeSizeOfDisk++;
        }
    }
    /*Return the value in bytes.*/
    return freeSizeOfDisk * SECTOR_SIZE;
}

/*Print the free size of the disk.*/
void print_freesize()
{
    printf("Free size of the disk: %d bytes\n\n", countEmptyFATentries());
}

/* Calculate the values of the FAT entry for calculating the free size. */
int get_free_space_FAT_entry(unsigned char *p, int n)
{

    int result, low4bits, high4bits, _8bits;

    if (n % 2 == 0)
    {
        /*  Read even values. 
        If n is even, then the physical location of the entry is the low four bits in location 1+(3*n)/2 
        and the 8 bits in location (3*n)/2 */

        low4bits = p[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0x0F; // stored first
        _8bits = p[SECTOR_SIZE + ((3 * n) / 2)] & 0xFF;       // stored second.
        result = (low4bits << 8) + _8bits;
    }

    else
    {
        /*  Read odd values. 
        If n is odd, then the physical location of the entry is the high four bits in location (3*n)/2 and 
        the 8 bits in location 1+(3*n)/2  */

        high4bits = p[SECTOR_SIZE + ((3 * n) / 2)] & 0xF0;
        _8bits = p[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0xFF;
        result = (high4bits >> 4) + (_8bits << 4);
    }

    return result;
}

/* Calculates the values of the FAT entries while parsing over the subdirectories. */
int get_FAT_entry(unsigned char *p, int n)
{

    int result, low4bits, high4bits, _8bits;

    if (n % 2 == 0)
    {
        /*  Read even values. 
        If n is even, then the physical location of the entry is the low four bits in location 1+(3*n)/2 
        and the 8 bits in location (3*n)/2 */

        low4bits = p[((3 * n) / 2) + 1] & 0x0F; // stored first
        _8bits = p[((3 * n) / 2)] & 0xFF;       // stored second.
        result = (low4bits << 8) + _8bits;
    }

    else
    {
        /*  Read odd values. 
        If n is odd, then the physical location of the entry is the high four bits in location (3*n)/2 and 
        the 8 bits in location 1+(3*n)/2  */

        high4bits = p[(int)((3 * n) / 2)] & 0xF0;
        _8bits = p[(int)((3 * n) / 2) + 1] & 0xFF;
        result = (high4bits >> 4) + (_8bits << 4);
    }

    return result;
}

void print_totalDiskSize()
{
    /* size of one sector * total sector count.*/
    printf("Total size of the disk: %d bytes\n", totalDiskSize());
}

/*Parse all the subdirectories and count the files*/
int parse_sub(int flc)
{
    int total_count = 0;
    unsigned char *temp = p;
    int next_loc = flc;

    /*If the cluster is reserved or is last */
    while (next_loc <= 0xff0)
    {
        /*Create local variable to calculate the path*/
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;   

        /*Parse over the sector*/
        for (int i = 0; i < 512; i += 32)
        {
            /*Check if the directory entry is valid/empty/volume label or not. If so, skip*/
            if ((temp[i + 26] + (temp[i + 27] << 8)) == 0x00 || (temp[i + 26] + (temp[i + 27] << 8)) == 0x01 || temp[i + 11] == 0x0F || (temp[i + 11] & 0x08) != 0 || temp[i] == '.' || temp[i] == 0xE5)
            {
                continue;
            }
            /*If directory entry is a file*/
            if ((temp[i + 11] & 0x10) == 0)
            {
                /*increment count*/
                total_count++;
                continue;
            }

            else
            {
                /*go to the next subdirectory.*/
                total_count += parse_sub(temp[i + 26] + (temp[i + 27] << 8));
            }
        }
        /*Get the next sector using the FAT table.*/
        next_loc = get_FAT_entry(fat_table_location, next_loc);
    }
    return total_count;
}

/*Find the value if the maximum number of root entries*/
void maxRootEntries()
{
    int value = p[17] + (p[18] << 8);
    printf("%d", value);
}

/*Calculate the num of files in our disk. */
void print_num_of_files()
{
    unsigned char *temp = p;
    //move to the root directory.
    temp += SECTOR_SIZE * 19;
    int count = 0;
   
    while (temp[0] != 0x00) //check for all entries till its not free.
    {
        /*Check if the directory entry is valid/empty/volume label or not*/
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }
        //check if the entry is a file..
        if ((temp[11] & 0x10) == 0)
        {
            count++;
        }

        else
        {
            /*calculate num of files in the subdirectories.*/
            count += parse_sub(temp[26] + (temp[27] << 8));
        }
        /*move to next directory entry*/
        temp += 32;
    }
    printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n\n", count);
}

/*Calculate num of FAT copies*/
int num_fat_copies()
{
    return p[16];
}

/*Print num of FAT copies*/
void print_num_fat_copies(int num)
{
    printf("Number of FAT copies: %d\n", num);
}

/*Num of sectors per FAT*/
int sectors_per_FAT()
{
    return p[22] + (p[23] << 8);
}

/*Print num of sectors per FAT*/
void print_sectors_per_fat(int num)
{
    printf("Sectors per FAT: %d\n", num);
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat sb;

    /*Check if the arguments are valid or not.*/
    if (argc > 2)
    {
        printf("Too many arguments. Need 1\n");
        return 1;
    }

    if (argc < 2)
    {
        printf("Too few arguments. Need 1\n");
        return 1;
    }

    /*Open the disk file*/
    fd = open(argv[1], O_RDWR);

    if (fd < 0)
    {
        printf("Invalid input. This program only takes 1 disk image as input.\n");
        return 1;
    }

    /* map disk file to virtual memory using mmap to read it. */
    fstat(fd, &sb);

    p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    unsigned char *osName = malloc(sizeof(char));
    unsigned char *diskLabel = malloc(sizeof(char));
    
    /*If map fails*/
    if (p == MAP_FAILED)
    {
        printf("Error: failed to map memory\n");
        exit(1);
    }

    /*Calculate the num of reserved sectors*/
    int reserved_sectors = (p[11] + (p[12] << 8)) * (p[14] + (p[15] << 8));
    fat_table_location = p;
    /*Store the location of the FAT table*/
    fat_table_location += reserved_sectors;

    // get OS name
    print_OS_Name(osName);

    // get label of disk
    print_diskLabel(diskLabel);

    // get total size of disk
    print_totalDiskSize();

    // get free size of disk
    print_freesize();

    printf("==============\n");

    // print num of files in disk
    print_num_of_files();

    printf("==============\n");
    // number of fat copies
    print_num_fat_copies(num_fat_copies());
    // number of sectors per fat
    print_sectors_per_fat(sectors_per_FAT());

    return 0;
}