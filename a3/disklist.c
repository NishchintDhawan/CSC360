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

/*Store the location of the FAT table*/
unsigned char *fat_table_location;
unsigned char *p;

/*Get the table table entries*/
int get_FAT_entry(unsigned char *p, int n);

/*Print the contents of the current sector of the directory*/
void print_directory(unsigned char *path);

/*Calculate the entries of the FAT table using the index.*/
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

/*Prints the name of the file.*/
void print_filename(unsigned char *temp)
{
    int i = 0;
    /*Storing the fullname(including extension)*/
    char *fullname = malloc(sizeof(char));
    /*Storing the file name,*/
    char *filename = malloc(sizeof(char));
    for (i = 0; i < 8; i++)
    {
        if (temp[i] == ' ') //if we have a space, we reach the end.
        {
            break;
        }
        filename[i] = temp[i];
    }
    filename[i] = '\0';

    /*Add the file name to the fullname.*/
    strcpy(fullname, filename);

    /*If we have an extnsion, store its value.*/
    char *extension = malloc(sizeof(char));
    for (i = 0; i < 3; i++)
    {
        if (temp[i + 8] == ' ') //if we have a space, we reach the end.
        {
            break;
        }
        extension[i] = temp[i + 8];
    }
    extension[i] = '\0';

    /*Update fullname if we have an extension.*/
    if (strlen(extension) > 0)
    {
        strcat(fullname, ".");
        strcat(fullname, extension);
    }
    /*Print the name.*/
    printf("%20s", fullname);
}

/*Prints the name of the subdirectory*/
void print_subdir_name(unsigned char *temp)
{
    int i = 0;
    char subdir_name[11];
    /*Get the name and store it in a local variable*/
    for (i = 0; i < 11; i++)
    {
        if (temp[i] == ' ') //if we have a space, we reach the end.
        {
            break;
        }
        subdir_name[i] = temp[i];
    }
    subdir_name[i] = '\0';
    /*Print the subdirectory name*/
    printf("%20s", subdir_name);
}

/*Parse the subdirectories recursively and print their contents. */
void parse_sub(unsigned char *fat_location, int flc, char *dir_path)
{
    unsigned char *temp = p;

    /*Store the flc value*/
    int next_loc = flc;

    /*Check if its reserved/last sector. Print contents of each sector for this subdirectory.*/
    while (next_loc <= 0xff5)
    {
        temp = p;
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;
        /*Print contents of the current sector*/
        print_directory(temp);
        printf("\n");
        next_loc = get_FAT_entry(fat_table_location, next_loc);
    }

    /*Reset the flc value.*/
    next_loc = flc;

    /*If it is reserved/last cluster. */
    while (next_loc <= 0xff5)
    {
        temp = p;
        /*Move to the data cluster for this sector*/
        int path = (31 + (int)next_loc) * SECTOR_SIZE;
        temp += path;

        /*Get the values in the data sector. print the contents.*/
        for (int i = 0; i < 512; i += 32)
        {
            /*Check if the input is valid or not. Must be a file or subdirectory.*/
            if ((temp[i + 26] + (temp[i + 27] << 8)) == 0x00 || (temp[i + 26] + (temp[i + 27] << 8)) == 0x01 || temp[i + 11] == 0x0F || (temp[i + 11] & 0x08) != 0 || temp[i] == '.' || temp[i] == 0xE5)
            {
                continue;
            }

            /*If the entry is a file.*/
            if ((temp[i + 11] & 0x10) == 0)
            {
                continue;
            }

            else
            {
                /*Is a subdirectory. Print contents of the subdirectory.*/
                char *temp2 = malloc(sizeof(char));
                int j = 0;
                /*Get the name of the subdirectory.*/
                for (j = 0; j < 11; j++)
                {
                    if (temp[i + j] == ' ') //if we have a space, we reach the end.
                    {
                        break;
                    }
                    temp2[j] = temp[i + j];
                }
                temp2[j] = '/';
                temp2[j + 1] = '\0';
                strcat(dir_path, temp2);
                /*Print the current directory path.*/
                printf("/%s", dir_path);
                printf("\n==================\n");
                /*Move to the next subdirectory.*/
                parse_sub(fat_location, temp[i + 26] + (temp[i + 27] << 8), dir_path);
            }
        }
        /*Get the location of the next sector from the FAT table.*/
        next_loc = get_FAT_entry(fat_table_location, next_loc);
    }
}

/*Get the maximum root directiry entries.*/
void maxRootEntries()
{
    int value = p[17] + (p[18] << 8);
    printf("%d", value);
}

/*Print the entire disk image*/
void print_entire_image()
{
    unsigned char *temp = p;
    //move to the root directory.
    temp += SECTOR_SIZE * 19;

    /*Print contents of current directory (Root) */

    char *dir_path = malloc(sizeof(char)); //Keeps track of the directiry path.

    printf("Root\n==================\n");

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

    while (temp[0] != 0x00) //check for all directory entries till its not free.
    {
        /*Check if the directory entry is valid or not. Must be a file or subdirectory.*/
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }

        /*Check if the entry is a file. */
        if ((temp[11] & 0x10) == 0)
        {
            temp += 32;
            continue;
        }

        /*if its a subdirectory.*/
        else
        {
            /*Get the directory name and update the current subdirectory path*/
            int i = 0;
            /*Get subdirectory name for updating path*/
            for (i = 0; i < 11; i++)
            {
                if (temp[i] == ' ') //if we have a space, we reach the end.
                {
                    break;
                }

                dir_path[i] = temp[i];
            }

            dir_path[i] = '/';
            dir_path[i + 1] = '\0';

            /*Print the path*/
            printf("/%s", dir_path);
            printf("\n==================\n");

            /*Move to the subdirectory*/
            parse_sub(fat_table_location, temp[26] + (temp[27] << 8), dir_path);
        }

        temp += 32;
    }
}

/*print the date and time*/
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

/*Calculate the file size*/
int filesize(unsigned char *path)
{
    int byte1 = path[28];
    int byte2 = path[29];
    int byte3 = path[30];
    int byte4 = path[31];
    /*Value is stored in 4 bytes: sectors 28-31.*/
    int result = (byte1 & 0xFF) + ((byte2 & 0xFF) << 8) + ((byte3 & 0xFF) << 16) + ((byte4 & 0xFF) << 24);
    return result;
}

/*Print the contents of the directory*/
void print_directory(unsigned char *path)
{

    unsigned char *temp = path;

    //directory entry is not free and if we haven't checked all the directory entries
    while (temp[0] != 0x00 && (temp - path) < 512)
    {
        /*Check the directory entry type. Must be a valid file or subdirectory*/
        if ((temp[26] + (temp[27] << 8)) == 0x00 || (temp[26] + (temp[27] << 8)) == 0x01 || temp[11] == 0x0F || (temp[11] & 0x08) != 0 || temp[0] == '.' || temp[0] == 0xE5)
        {
            temp += 32;
            continue;
        }

        /*If we have a file*/
        if ((temp[11] & 0x10) == 0)
        {

            printf("F ");

            //print file size
            int size = filesize(temp);
            printf("%10d", size);

            /*Print the file name*/
            print_filename(temp);

            /*Print the date and time stamp*/
            print_datetime(temp);
        }
        else
        {
            printf("D ");
            //print directory size
            printf("%10d", 0);

            /*Print the subdirectory name*/
            print_subdir_name(temp);

            /*Print the date time stamps*/
            print_datetime(temp);
        }
        temp += 32;
    }
}

int main(int argc, char *argv[])
{
    int fd;
    struct stat sb;

    /*Check if the input file is valid or not*/
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

    /*Open the disk image*/
    fd = open(argv[1], O_RDWR);

    if (fd < 0)
    {
        printf("Invalid input. This program only takes 1 disk image as input.\n");
        return 1;
    }

    /* map disk file to virtual memory using mmap to read it. */
    fstat(fd, &sb);

    p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    /*If map fails*/
    if (p == MAP_FAILED)
    {
        printf("Error: failed to map memory\n");
        exit(1);
    }

    /*Calculate the num of reserved sectors*/
    int reserved_sectors = (p[11] + (p[12] << 8)) * (p[14] + (p[15] << 8));
    fat_table_location = p;

    /*Get the location of FAT table*/
    fat_table_location += reserved_sectors;

    /*Print the all disk files and subdirectories*/
    print_entire_image();

    return 0;
}