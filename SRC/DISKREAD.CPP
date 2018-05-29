/**
 * DISKREAD.CPP - Disk Reading Class
 *   - Reads raw Sectors through BIOS Int 0x13 on Client
 */

#include "DISKREAD.H"

struct diskinfo_t DISKREAD::query_disks(uint8_t disk_num)
{
    //struct diskinfo_t *di = ;
    union REGS regs;
    struct SREGS sregs;

    //uint32_t total_sectors;

    regs.h.ah = 0x08;
    regs.h.dl = 0x80 | disk_num;

    // Set the segment and index to gaurd against BIOS bugs
    regs.w.di = 0x0000;
    sregs.es = 0x0000;

    int86x(0x13, &regs, &regs, &sregs);

    disk_params.track = regs.h.cl & 0xC0;
	disk_params.track = ((regs.h.cl << 2) | regs.h.ch) + 1;
	disk_params.sector = regs.h.cl & 0x3F;
	disk_params.head = regs.h.dh + 1;
	disk_params.drive = regs.h.dl;
	
    total_sectors = disk_params.head * di.track * disk_params.sector;
    total_bytes = total_sectors * SECTOR_SIZE;
    
    printf("Drive parameters:\n");
    printf("Number of Drives: %d\n", disk_params.drive);
	printf("Number of Heads: %d\n", disk_params.head);
	printf("Number of Cylinders: %d\n", disk_params.track);
	printf("Number of Sectors per Cylinder: %d\n", disk_params.sector);
	printf("Total Number of Sectors: %lu\n", total_sectors);
	printf("Estimated number of raw bytes at 512 bytes per cluster: %lu\n", total_bytes);
    
	return disk_params;
}

int DISKREAD::verify_mbr(uint8_t disk_num)
{
    struct diskinfo_t di;
    uint8_t status;

    // Reset disks
	di.drive = 0x80 | drive_num;
    error_countdown = ERR_TIMER_START;
	do {
        status = _bios_disk(_DISK_RESET, &di) >> 8;
        if (status > 0) {
            handle_error(status, false);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -1;
            }
        }
    } while (status > 0);

    /*
	if (status > 0) {
		disk_error(status);
		return -1;
	}
    */

    di.drive = 0x80 | drive_num;
    di.head = di.track = 0;
    di.sector = 1;
    di.nsectors = 1;
    di.buffer = NULL;
    
    error_countdown = ERR_TIMER_START;
    do {
        status = _bios_disk(_DISK_VERIFY, &di) >> 8;
        if (status > 0) {
            handle_error(status, false);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -1;
            }
        }
    } while (status > 0);

    /*
    if (status > 0) {
        disk_error(status);
        return -2;
    }
    */

    return 0;
}

int DISKREAD::query_mbr(uint8_t *mbr_data, int drive_num)
{
    struct diskinfo_t di;
    uint8_t status = 0;

    if (mbr_data == NULL) {
        fprintf(stderr, "*mbr_data must be allocated before DISKREAD::query_mbr() is called\n");
        return -1;
    }

    // Reset Disks
    di.drive = 0x80 | drive_num;
    error_countdown = ERR_TIMER_START;
	do {
        status = _bios_disk(_DISK_RESET, &di) >> 8;
        if (status > 0) {
            handle_error(status, false);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -1;
            }
        }
    } while (status > 0);

    di.drive = 0x80 | drive_num;
	//di.drive = 0;
	di.head = di.track = 0;
	di.sector = 1;
	di.nsectors = 1;
	di.buffer = mbr_data;

    error_countdown = ERR_TIMER_START;
    do {
        status = _bios_disk(_DISK_READ, &di) >> 8;
        if (status > 0) {
            handle_error(status, false);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -2;
            }
        }
    } while (status > 0);

    // Check MBR Signature
    if ((uint16_t) mbr_data[SECTOR_SIZE-2] != 0xAA55) {
        fprintf(stderr, "Couldn't find boot signature 0xAA55 in MBR.\n");
        return -3;
    }

    return 0;
}

void DISKREAD::handle_error(uint8_t status, bool skippable)
{
    char c;
    bool valid_input = false;

    disk_error(status);
    error_countdown--;

    if (error_countdown <= 0) {
        do {
            if (skippable) {
                printf("\aAttempted %d times. Try again? ([Y]es, [C]ancel, [S]kip Operation): ", ERR_TIMER_START);
            } else {
                printf("\aAttempted %d times. Try again? ([Y]es, [C]ancel): ", ERR_TIMER_START);
            }
            c = getchar();

            switch (c) {
                case 'Y': 'y':
                    valid_input = true;
                    error_countdown = ERR_TIMER_START;
                    break;
                case 'C': 'c':
                    valid_input = true;
                    error_countdown = ERR_TIMER_CANCEL;
                    break;
                case 'S': 's':
                    if (skippable) {
                        valid_input = true;
                        error_countdown = ERR_TIMER_SKIP;
                    } else {
                        valid_input = false;
                    }
                    break;
                default:
                    valid_input = false;
            }
        } while(!valid_input);
        printf("\n");
    }
}

void DISKREAD::disk_error(uint8_t status)
{
    fprintf(stderr, "Error 0x%02X: ", status);
    
	switch (status) {
		case 0x01:
			fprintf(stderr, "Bad Command\n");
			break;
		case 0x02:
			fprintf(stderr, "Address mark not found\n");
			break;
		case 0x03:
			fprintf(stderr, "Attempt to write to write-protected disk\n");
			break;
		case 0x04:
			fprintf(stderr, "Sector not found\n");
			break;
		case 0x05:
			fprintf(stderr, "Reset failed\n");
			break;
		case 0x06:
			fprintf(stderr, "Disk changed since last operation\n");
			break;
		case 0x07:
			fprintf(stderr, "Drive parameter activity failed\n");
			break;
		case 0x08:
			fprintf(stderr, "DMA overrun\n");
			break;
		case 0x09:
			fprintf(stderr, "Attempt to DMA across 64K boundary\n");
			break;
		case 0x0A:
			fprintf(stderr, "Bad sector detected\n");
			break;
		case 0x0B:
			fprintf(stderr, "Bad track detected\n");
			break;
		case 0x0C:
			fprintf(stderr, "Unsupported track\n");
			break;
		case 0x0D:
			fprintf(stderr, "Invalid number of sectors on format\n");
			break;
		case 0x0E:
			fprintf(stderr, "Control data address mark detected\n");
			break;
		case 0x0F:
			fprintf(stderr, "DMA arbitration level out of range\n");
			break;
		case 0x10:
			fprintf(stderr, "Data read (CRC/ECC) error\n");
			break;
		case 0x11:
			fprintf(stderr, "CRC/ECC corrected data error\n");
			break;
		case 0x20:
			fprintf(stderr, "Controller failure\n");
			break;
		case 0x31:
			fprintf(stderr, "No media in drive\n");
			break;
		case 0x32:
			fprintf(stderr, "Incorrect drive type stored in CMOS\n");
			break;
		case 0x40:
			fprintf(stderr, "Seek operation failed\n");
			break;
		case 0x80:
			fprintf(stderr, "Disk timed out or failed to respond\n");
			break;
		case 0xAA:
			fprintf(stderr, "Drive not ready\n");
			break;
		case 0xB0:
			fprintf(stderr, "Volume not locked in drive\n");
			break;
		case 0xB1:
			fprintf(stderr, "Volume locked in drive\n");
			break;
		case 0xB2:
			fprintf(stderr, "Volume not removable\n");
			break;
		case 0xB3:
			fprintf(stderr, "Volume in use\n");
			break;
		case 0xB4:
			fprintf(stderr, "Lock count exceeded\n");
			break;
		case 0xB5:
			fprintf(stderr, "Valid eject request failed\n");
			break;
		case 0xB6:
			fprintf(stderr, "Volume present but read protected\n");
			break;
		case 0xBB:
			fprintf(stderr, "Undefined error occurred\n");
			break;
		case 0xCC:
			fprintf(stderr, "Write fault occurred\n");
			break;
		case 0xE0:
			fprintf(stderr, "Status error\n");
			break;
		case 0xFF:
			fprintf(stderr, "Sense operation failed\n");
			break;
		default:
			fprintf(stderr, "Unknown error\n");
	}
}