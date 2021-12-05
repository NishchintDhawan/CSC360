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

void print_filename(unsigned char *temp)
{
    int i = 0;
    char *fullname = malloc(sizeof(char));
    char *filename = malloc(sizeof(char));
    for (i = 0; i < 8; i++)
    {
        if (temp[i] == ' ')
        {
            break;
        }
        filename[i] = temp[i];
    }
    filename[i] = '\0';
    strcpy(fullname,filename);
   // printf("%s", filename);

    char *extension = malloc(sizeof(char));
    for (i = 0; i < 3; i++)
    {
        if (temp[i + 8] == ' ')
        {
            break;
        }
        extension[i] = temp[i + 8];
    }
    extension[i] = '\0';

    if (strlen(extension) > 0)
    {
        strcat(fullname,".");
        strcat(fullname,extension);
        //printf(".%s", extension);
    }
    printf("%20s", fullname);
}

void print_subdir_name(unsigned char *temp)
{
    int i = 0;
    char subdir_name[11];
    for (i = 0; i < 11; i++)
    {
        if (temp[i] == ' ')
        {
            break;
        }
        subdir_name[i] = temp[i];
    }
    subdir_name[i] = '\0';
    printf("%20s", subdir_name);
}

void parse_sub(unsigned char *fat_location, int flc, char dir_path[4096])
{
    int addr = 0;
    int total_count = 0;
    unsigned char sector_data[512];
    unsigned char *temp = p;

    int next_loc = flc;

    while (next_loc <= 0xff5)
    {
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;

        print_directory(temp);
        printf("\n");
        next_loc = get_FAT_entry(fat_table_location, next_loc);
    }

    next_loc = flc;

    while (next_loc <= 0xff5)
    {
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;

        //get the values in the data sector.
        for (int i = 0; i < 512; i += 32)
        {

            if ((temp[i + 26] + (temp[i + 27] << 8)) == 0x00 || (temp[i + 26] + (temp[i + 27] << 8)) == 0x01 || temp[i + 11] == 0x0F || (temp[i + 11] & 0x08) != 0 || temp[i] == '.' || temp[i] == 0xE5)
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
                unsigned char temp2[20];
                int j = 0;
                for (j = 0; j < 11; j++)
                {
                    if (temp[i + j] == ' ')
                    {
                        break;
                    }
                    temp2[j] = temp[i + j];
                }
                temp2[j] = '/';
                temp2[j + 1] = '\0';
                strcat(dir_path, temp2);
                printf("/%s", dir_path);
                printf("\n=================\n");
                parse_sub(fat_location, temp[i + 26] + (temp[i + 27] << 8), dir_path);
            }
        }
        next_loc = get_FAT_entry(fat_table_location, next_loc);
    }
}

void maxRootEntries()
{
    int value = p[17] + (p[18] << 8);
    printf("%d", value);
}

void print_entire_image()
{
    unsigned char *temp = p;
    temp += SECTOR_SIZE * 19; //move to the root directory.
    int count = 0;
    //print contents of current directory
    unsigned char dir_path[4096];
    printf("Root\n=================\n");

    unsigned char *print_root = temp;

    /* print all root directory sectors */
    for (int i = 0; i < 19; i++)
    {
        if (print_root[0] == 0x00)
        {
            break;
        }
        print_directory(print_root);
        printf("\n");
        print_root += SECTOR_SIZE;
    }

    while (temp[0] != 0x00) //directory entry is not free.
    {
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == 0xE5)
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
            int i = 0;
            for (i = 0; i < 11; i++)
            {
                if (temp[i] == ' ')
                {
                    break;
                }

                dir_path[i] = temp[i];
            }

            dir_path[i] = '/';
            dir_path[i + 1] = '\0';
            printf("/%s", dir_path);
            printf("\n=================\n");

            parse_sub(fat_table_location, temp[26] + (temp[27] << 8), dir_path);
        }

        temp += 32;
    }
}

void print_datetime(unsigned char *path)
{
    int time, date;
    int hours, minutes, day, month, year;

    time = *(unsigned short *)(path + 14);
    date = *(unsigned short *)(path + 16);
    //print time stamp
    year = ((date & 0xFE00) >> 9) + 1980;
    //the month is stored in the middle four bits
    month = (date & 0x1E0) >> 5;
    //the day is stored in the low five bits
    day = (date & 0x1F);
    printf(" %d-%02d-%02d ", year, month, day);

    hours = (time & 0xF800) >> 11;
    //the minutes are stored in the middle 6 bits
    minutes = (time & 0x7E0) >> 5;

    printf("%02d:%02d\n", hours, minutes);
}

int filesize(unsigned char *path)
{
    int byte1 = path[28];
    int byte2 = path[29];
    int byte3 = path[30];
    int byte4 = path[31];

    int result = (byte1 & 0xFF) + ((byte2 & 0xFF) << 8) + ((byte3 & 0xFF) << 16) + ((byte4 & 0xFF) << 24);
    return result;
}

void print_directory(unsigned char *path)
{

    unsigned char *temp = path;
    int count = 0;

    while (temp[0] != 0x00 && (temp - path) < 512) //directory entry is not free.
    {
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == '.' || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }

        if ((temp[11] & 0x10) == 0)
        {

            printf("F: ");
            //print file size
            int size = filesize(temp);
            printf("%10d", size);
            print_filename(temp);
            print_datetime(temp);
        }
        else
        {
            printf("D: ");
            //print directory size
            printf("%10d",0);
            print_subdir_name(temp);
            print_datetime(temp);
        }
        temp += 32;
    }
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

    //get free size of disk
    print_entire_image();

    return 0;
}