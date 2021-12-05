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

unsigned char *fat_table_location;
FILE *diskfile;
unsigned char *p;

int get_FAT_entry(unsigned char *p, int n);

void print_directory(unsigned char *path);

void print_OS_Name(unsigned char *osName)
{

    for (int i = 0; i < 8; i++)
    {
        osName[i] = p[i + 3];
    }

    printf("OS Name: %s\n", osName);
}

void print_diskLabel(unsigned char *diskLabel)
{

    int i = 0;
    unsigned char extension[3];
    unsigned char *temp = p;
    temp += SECTOR_SIZE * 19; //go to root directory.
    while (temp[0] != 0x00)
    { //search the entries. If first byte is 0, then all available entries visited.
        if ((temp[11] != 0x0F) && ((temp[11] & 0x08) == 0x08))
        { //if 11th byte has 3rd bit set, it is the volume label.
            // get the filename
            for (i = 0; i < 8; i++)
            { //store byte data into diskLabel.
                diskLabel[i] = temp[i];
            }
            //get the extension
            for (i = 0; i < 3; i++)
            {
                extension[i] = temp[i];
            }
        }
        temp += 32;
    }

    if (!strcmp(extension, "   "))
    {
        printf("label of the disk: %s.%s\n", diskLabel, extension);
    }
    else
    {
        printf("label of the disk: %s\n", diskLabel);
    }
}

int countEmptyFATentries(int totalSize)
{

    //iterate over FAT entries.
    // As first 2 indices are reserved, we start from 2.
    int count = 0;
    //  physical sector number = 33 + FAT entry number -2 or FAT entry number = physical sector number -31.
    //  printf("====== Num of entries in FAT: %d ======\n", (totalSize/SECTOR_SIZE)-31);
    for (int i = 2; i < (totalSize / SECTOR_SIZE) - 31; i++)
    {
        if (get_FAT_entry(p, i) == 0x000)
        {
            count++;
        }
    }

    return count * SECTOR_SIZE;
}

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

int totalDiskSize()
{
    //defined as number of sectors * total sector count.
    //endianness in order of bytes. move MSByte by 8 places to left and then add LSByte.
    int bytesPerSector = p[11] + (p[12] << 8);   // bytes 11-12
    int totalSectorCount = p[19] + (p[20] << 8); // bytes 19-20
    return bytesPerSector * totalSectorCount;
}

void print_totalDiskSize()
{
    //defined as number of sectors * total sector count.
    //endianness in order of bytes. move MSByte by 8 places to left and then add LSByte.
    printf("Total size of the disk: %d bytes\n", totalDiskSize());
}

void print_freesize()
{
    printf("Free size of the disk: %d bytes\n\n", countEmptyFATentries(totalDiskSize()));
}

void parse_sub(unsigned char *fat_location, int flc)
{
    int addr = 0;
    int total_count = 0;
    unsigned char sector_data[512];
    unsigned char *temp = p;
    // printf("first_logical_cluster: %p\n", get_FAT_entry(p, flc));
    // int path = (31 + (int)flc) * SECTOR_SIZE;
    // temp += path;
    //printf("\nCurrent location: %p\n", flc);
    int next_loc = flc;
    // printf("\nNext loc:%p\n", next_loc);
   
    while (next_loc <= 0xff5)
    {
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;
       // printf("%s\n", &temp[32]+32);
        print_directory(temp);
        printf("\n");
        next_loc = get_FAT_entry(fat_table_location,next_loc);
    }

    next_loc = flc;

    while (next_loc <= 0xff5)
    {
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;
        //printf("%s\n", temp);
        //get the values in the data sector.
        for (int i = 0; i < 512; i += 32)
        {
           // printf("%s", &temp[i]);
           
            if ((temp[i + 26] + (temp[i+27] << 8)) == 0x00 || (temp[i + 26] + (temp[i+27] << 8)) == 0x01 || temp[i + 11] == 0x0F || (temp[i + 11] & 0x08) != 0 || temp[i] == '.' || temp[i] == 0xE5)
            {
                continue;
            }

            if ((temp[i + 11] & 0x10) == 0)
            {
                total_count++;
                continue;
            }

            else
            {
                //printf("\n%p\n", get_FAT_entry(fat_location, next_loc));
                // total_count += parse_sub(fat_location, get_FAT_entry(fat_location, next_loc));
                //go to the next subdirectory.
                printf("\n%s/", &temp[i]);
                printf("\n=================\n");
                parse_sub(fat_location, temp[i+26] + (temp[i+27] << 8));
            }
        }
        next_loc = get_FAT_entry(fat_table_location,next_loc);
    }
    // return total_count;
    // printf("\nnext sector: %p\n", next_loc);
    // while(next_loc<=0xff5){
    //     next_loc = get_FAT_entry(fat_table_location, next_loc);
    //     printf("\nnextsector: %p\n", next_loc);
    // }
}

void maxRootEntries(){
    int value = p[17] + (p[18] << 8);
    printf("%d", value);
}

void print_num_of_files()
{
    unsigned char *temp = p;
    temp += SECTOR_SIZE * 19; //move to the root directory.
    int count = 0;
    //print contents of current directory

    printf("ROOT/\n=================\n");

    unsigned char* print_root = temp;

    for(int i=0; i<19; i++){
        if(print_root[0]==0x00){
            break;
        }
        print_directory(print_root);
        printf("\n");
        print_root += SECTOR_SIZE;
    }

    while (temp[0] != 0x00) //directory entry is not free.
    {
        if ( (temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }
        //check if the entry is a subdirectory, entry is hidden.
        if ((temp[11] & 0x10) == 0)
        {
            count++;
        }
        else
        {
            printf("%s/", &temp[0]);
            printf("\n=================\n");
            // count += parse_sub(fat_table_location, temp[26] + (temp[27] << 8));
            parse_sub(fat_table_location, temp[26] + (temp[27] << 8));
        }

        temp += 32;
    }
    // printf("\nval : %d\n", count);
    //printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n\n", count);
}

void print_directory(unsigned char *path)
{

    unsigned char *temp = path;
    //temp += SECTOR_SIZE * 19; //move to the root directory.
    int count = 0;

    while (temp[0] != 0x00 && (temp-path)<512) //directory entry is not free.
    {
       // printf("temp -> %s\n", temp);
        // for(int i=0;i<32;i++) printf("%p ", temp[i]);
        // printf("\n");
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == '.' || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }
        
       // printf("Value -> %p\n", (temp[26] + (temp[27] << 8)));
        //printf("\n%s\n", &temp[0]);
        //check if the entry is a subdirectory, entry is hidden.
        //printf("temp\n");
        if ((temp[11] & 0x10) == 0)
        {
            printf("F: %s\n", &temp[0]);
        }
        else
        {
            printf("D: %s\n", &temp[0]);
        }
        temp += 32;
    }
}

int num_fat_copies()
{
    return p[16];
}

void print_num_fat_copies(int num)
{
    printf("Number of FAT copies: %d\n", num);
}

int sectors_per_FAT()
{
    return p[22] + (p[23] << 8);
}

void print_sectors_per_fat(int num)
{
    printf("Sectors per FAT: %d\n", num);
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat sb;
    diskfile = fopen(argv[1], "r");

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

    fd = open(argv[1], O_RDWR);

    if (fd < 0)
    {
        printf("Invalid input. This program only takes 1 disk image as input.\n");
        return 1;
    }

    // map disk file to virtual memory to read it using file descriptor.
    fstat(fd, &sb);

    p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    unsigned char *osName = malloc(sizeof(char));
    unsigned char *diskLabel = malloc(sizeof(char));

    if (p == MAP_FAILED)
    {
        printf("Error: failed to map memory\n");
        exit(1);
    }

    int reserved_sectors = (p[11] + (p[12] << 8)) * (p[14] + (p[15] << 8));
    // printf(" Bytes per sector : %d , Boot sectors:  %d\n", (p[11]+(p[12]<<8)), (p[14]+(p[15]<<8)) );
    fat_table_location = p;
    fat_table_location += SECTOR_SIZE; // + reserved_sectors;

    //get OS name
    //print_OS_Name(osName);

    // //get label of disk
    // print_diskLabel(diskLabel);

    // //get total size of disk
    // print_totalDiskSize();

    //print_freesize();

    //printf("==============\n");
    //get free size of disk
    print_num_of_files();
    //printf("value-> %p\n",get_FAT_entry(fat_table_location, 0));
    // printf("==============\n");
    // //number of files in disk
    // //number of fat copies
    // print_num_fat_copies(num_fat_copies());
    // //number of sectors per fat
    // print_sectors_per_fat(sectors_per_FAT());

    return 0;
}