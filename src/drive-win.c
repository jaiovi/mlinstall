// Windows fileapi based code

// Don't include me unless we're windows!
#ifdef WIN32

#include <stdio.h>
#include <windows.h>

#include "drive.h"

// Bootsector size
#define SIZE 512

char bootsector[SIZE];
DWORD bytesRead = 0;

HANDLE d;

int flag_getfs()
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	if (!strncmp(bootsector + 54, "FAT16   ", 8)) {
		return FAT16;
	}

	if (!strncmp(bootsector + 82, "FAT32   ", 8)) {
		return FAT32;
	}

	if (!strncmp(bootsector + 82, "EXFAT   ", 8)) {
		return EXFAT;
	}

	return DRIVE_BADFS;
}

void flag_write(long offset, char string[])
{
	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	ReadFile(d, bootsector, SIZE, &bytesRead, NULL);

	printf("Current Flag: %s\n", bootsector + offset);
	memcpy(bootsector + offset, string, strlen(string));
	printf("New Flag:     %s\n", bootsector + offset);

	SetFilePointer(d, 0, NULL, FILE_BEGIN);
	WriteFile(d, bootsector, SIZE, &bytesRead, NULL);
}

int flag_getdrive()
{
	char id;
	char command[128];

	// List info usb type mounted filesystems
	FILE *f = popen("wmic logicaldisk where drivetype=2 get deviceid, volumename", "r");

	// Skip first line (title)
	fgets(command, 128, f);

	// Look for EOS_DIGITAL drive
	while (fgets(command, 128, f) != NULL) {
		if (!strncmp(command + 10, "EOS_DIGITAL", 11)) {
			printf("Found EOS_DIGITAL at drive %c\n", command[0]);
			id = command[0];
			goto found;
		}
	}

	return DRIVE_NONE;

found:
	if (id == 'C' || id == 'c') {
		puts("Somehow got C drive...");
		return DRIVE_NONE;
	}

	return (int)id;
}

// Should never buffer overflow
int flag_usable_drive(char buffer[])
{
	int drive = flag_getdrive();
	if (drive < 0) {
		return drive;
	}

	strcpy(buffer, " :");
	buffer[0] = (char)drive;
	return 0;
}

int flag_openfs(int mode)
{
	// Windows filesystems must be opened like this: \\.\\E
	char buffer[64] = "\\\\.\\\0:";
	int drive = flag_getdrive(buffer);
	if (drive < 0) {
		return drive;
	}

	buffer[4] = (char)drive;

	d = CreateFile(buffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);

	if (d == INVALID_HANDLE_VALUE) {
		puts("Couldn't open the filesystem. Try running as Administrator.\n"
			"Check file explorer and make sure EOS_DIGITAL is mounted.");
		return -1;
	}

	return flag_getfs();
}

void flag_close()
{
	// TODO: figure out FileClose (?)
}

#endif
