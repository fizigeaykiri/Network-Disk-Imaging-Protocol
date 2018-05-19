/**
 * DiskView - displays basic parameters of default hard drive
 * By @marssaturn91
 */
 
#include <stdio.h>
#include <bios.h>
//#include <dos.h>
#include <i86.h>

void query_disks(void);

void main(int argc, char *argv[])
{
	
	printf("DiskView - displays basic disk data\n");
	
	query_disks();
}

void query_disks(void)
{
	struct diskinfo_t di;
	//unsig
	
	union REGS regs;
	struct SREGS sregs;
	
	regs.h.ah = 0x08;
	regs.h.dl = 0x80 | 0;
	
	// set the segment and index to guard against BIOS bugs
	regs.w.di = 0x0000;
	sregs.es = 0x0000;
	
	int86x(0x13, &regs, &regs, &sregs);
	
	//printf("Low Cylinders: %X\n", regs.h.ch);
	//printf("High Cylinders & Sectors: %X\n", regs.h.cl);
	//printf("Heads: %X\n", regs.h.dh);
	//printf("Number of Drives: %X\n", regs.h.dl);
	
	printf("\n");
	
	//di.track = (((regs.h.cl & 0xC0) >> 6) | regs.h.ch) + 1;
	di.track = regs.h.cl & 0xC0;
	di.track = ((regs.h.cl << 2) | regs.h.ch) + 1;
	di.sector = (regs.h.cl & 0x3F) + 1;
	di.head = regs.h.dh + 1;
	di.drive = regs.h.dl;
	
	printf("Number of Drives: %d\n", di.drive);
	printf("Number of Heads: %d\n", di.head);
	printf("Number of Cylinders: %d\n", di.track);
	printf("Number of Sectors per Cylinder: %d\n", di.sector);
}

