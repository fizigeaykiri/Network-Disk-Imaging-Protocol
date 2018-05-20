/**
 * MbrView - displays MBR of default hard drive in Hex
 * By @marssaturn91
 */
 
#include <stdio.h>
#include <bios.h>
#include <i86.h>

struct diskinfo_t query_disks(void);
int query_mbr(int drive_num);
void display_buffer(unsigned char *disk_buffer, unsigned int buffer_len);
void disk_error(unsigned short status);


int main(int argc, char *argv[])
{
	struct diskinfo_t di;
	
	printf("MbrView - displays MBR of default hard drive in Hex\n\n");
	
	di = query_disks();
	if (di.drive == 0) {
		printf("No Hard Drives were found.\n");
		return -1;
	}
	
	query_mbr(0);
	
	return 0;
}

struct diskinfo_t query_disks(void)
{
	struct diskinfo_t di;
	
	union REGS regs;
	struct SREGS sregs;
	
	regs.h.ah = 0x08;
	regs.h.dl = 0x80 | 0;
	
	// set the segment and index to guard against BIOS bugs
	regs.w.di = 0x0000;
	sregs.es = 0x0000;
	
	int86x(0x13, &regs, &regs, &sregs);
	
	di.track = regs.h.cl & 0xC0;
	di.track = ((regs.h.cl << 2) | regs.h.ch) + 1;
	di.sector = (regs.h.cl & 0x3F) + 1;
	di.head = regs.h.dh + 1;
	di.drive = regs.h.dl;
	
	return di;
}

int query_mbr(int drive_num)
{
	unsigned char disk_buffer[1024];
	
	struct diskinfo_t di;
	unsigned short status = 0;
	
	// Reset disks
	/*
	di.drive = 0x80 | drive_num;
	status = _bios_disk(_DISK_RESET, &di);
	if (status > 0) {
		disk_error(status);
		//return -1;
	}
	*/
	
	//di.drive = 0x80 | drive_num;
	di.drive = 0;
	di.head = 0;
	di.track = 0;
	di.sector = 0;
	di.nsectors = 1;
	di.buffer = disk_buffer;
	
	printf("Querying boot sector...\n");
	
	status = _bios_disk(_DISK_READ, &di);
	if (status > 0) {
		disk_error(status);
		//return -1;
	}
	
	printf("Master Boot Record:\n");
	display_buffer(disk_buffer, 512);
	
	return 0;
}

void display_buffer(unsigned char *disk_buffer, unsigned int buffer_len)
{
	int i = 0;
	int j = 0;
	
	for (i = 0; i < buffer_len; i++) {
		printf("%02X ", disk_buffer[i]);
		j += 3;
		
		if (j >= 78) {
			j = 0;
			printf("\n");
		}
	}
}

void disk_error(unsigned short status)
{
	switch (status) {
		case 0x01:
			printf("Bad Command\n");
			break;
		case 0x02:
			printf("Address mark not found\n");
			break;
		case 0x03:
			printf("Attempt to write to write-protected disk\n");
			break;
		case 0x04:
			printf("Sector not found\n");
			break;
		case 0x05:
			printf("Reset failed\n");
			break;
		case 0x06:
			printf("Disk changed since last operation\n");
			break;
		case 0x07:
			printf("Drive parameter activity failed\n");
			break;
		case 0x08:
			printf("DMA overrun\n");
			break;
		case 0x09:
			printf("Attempt to DMA across 64K boundary\n");
			break;
		case 0x0A:
			printf("Bad sector detected\n");
			break;
		case 0x0B:
			printf("Bad track detected\n");
			break;
		case 0x0C:
			printf("Unsupported track\n");
			break;
		case 0x0D:
			printf("Invalid number of sectors on format\n");
			break;
		case 0x0E:
			printf("Control data address mark detected\n");
			break;
		case 0x0F:
			printf("DMA arbitration level out of range\n");
			break;
		case 0x10:
			printf("Data read (CRC/ECC) error\n");
			break;
		case 0x11:
			printf("CRC/ECC corrected data error\n");
			break;
		case 0x20:
			printf("Controller failure\n");
			break;
		case 0x31:
			printf("No media in drive\n");
			break;
		case 0x32:
			printf("Incorrect drive type stored in CMOS\n");
			break;
		case 0x40:
			printf("Seek operation failed\n");
			break;
		case 0x80:
			printf("Disk timed out or failed to respond\n");
			break;
		case 0xAA:
			printf("Drive not ready\n");
			break;
		case 0xB0:
			printf("Volume not locked in drive\n");
			break;
		case 0xB1:
			printf("Volume locked in drive\n");
			break;
		case 0xB2:
			printf("Volume not removable\n");
			break;
		case 0xB3:
			printf("Volume in use\n");
			break;
		case 0xB4:
			printf("Lock count exceeded\n");
			break;
		case 0xB5:
			printf("Valid eject request failed\n");
			break;
		case 0xB6:
			printf("Volume present but read protected\n");
			break;
		case 0xBB:
			printf("Undefined error occurred\n");
			break;
		case 0xCC:
			printf("Write fault occurred\n");
			break;
		case 0xE0:
			printf("Status error\n");
			break;
		case 0xFF:
			printf("Sense operation failed\n");
			break;
		default:
			printf("Unknown error %0X\n", status);
	}
}

