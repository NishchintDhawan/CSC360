#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512

int get_max_entries(char *p)
{
	printf("%p\n", p[17]);
	printf("%p\n", p[18]);
	return (p[17] >> 8 );
}

void print_entry_name(char *p)
{
	char *name = malloc(sizeof(char));
	//it is a file
	int i = 0;
	for (i = 0; i < 8; i++)
	{
		if (p[i] == 0x20)
		{
			break;
		}
		name[i] = p[i];
	}

	name[i] = '\0';

	printf("%s", name);
}

void directory_parser(char *p, char *diskList, int maxEntries)
{

	p += SECTOR_SIZE * 19; //move to the root directory.
	int i = 0;
	int curr_entry = 0;
	while (p[0] != 0x00 && (curr_entry < maxEntries)) //directory entry is not free.
	{
		if (p[26] == 0x00 && p[26] == 0x01)
		{
			if (p[11] == 0x0F)
			{
				continue;
			}
		}

		if ((p[11] & 0x08) == 0 && (p[11] & 0x10) == 0 && (p[11] & 0x02) == 0)
		{
			printf("F : ");
			print_entry_name(p);
			printf(".");
			//print extension
			for (i = 8; i < 11; i++)
			{
				printf("%c", p[i]);
			}
			printf("\n");
		}

		else if ((p[11] & 0x08) == 0 && (p[11] & 0x10) != 0 && (p[11] & 0x02) == 0)
		{
			// it is a subdirectry.
			printf("S : ");
			print_entry_name(p);
			printf("/\n");
		}

		p += 32;
		curr_entry += 1;
	}
}

int main(int argc, char *argv[])
{
	int fd;
	struct stat sb;

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

	char *p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

	char *diskList = malloc(sizeof(char));

	if (p == MAP_FAILED)
	{
		printf("Error: failed to map memory\n");
		exit(1);
	}

	printf("%d\n", get_max_entries(p));
//	directory_parser(p, diskList, get_max_entries(p) );

	return 0;
}
