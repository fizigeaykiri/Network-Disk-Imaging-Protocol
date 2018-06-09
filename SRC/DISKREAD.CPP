/**
 * DISKREAD.CPP - Disk Reading Class
 *   - Reads raw Sectors through BIOS Int 0x13 on Client
 */

#include "DISKREAD.H"

uint8_t DISKREAD::sec_buffer[SECTOR_SIZE] = {0};
static struct disk_error_map *DISKREAD::disk_err_map = NULL;
struct diskinfo_t DISKREAD::disk_params;
uint64_t DISKREAD::total_sectors = 0;
uint64_t DISKREAD::cur_log_sector = 0;
uint64_t DISKREAD::total_bytes = 0; 
int DISKREAD::error_countdown = 0;
int DISKREAD::disk_error_count = 0;
int DISKREAD::disk_err_map_len = 0;

struct diskinfo_t DISKREAD::query_disks(uint8_t disk_num)
{
    //struct diskinfo_t *di = ;
    union REGS regs;
    struct SREGS sregs;

    //uint32_t total_sectors;

    regs.h.ah = 0x08;
    regs.h.dl = 0x80 | disk_num;
    //regs.h.dl = disk_num;

    // Set the segment and index to gaurd against BIOS bugs
    regs.w.di = 0x0000;
    sregs.es = 0x0000;

    int86x(0x13, &regs, &regs, &sregs);

    disk_params.track = regs.h.cl & 0xC0;
	disk_params.track = ((regs.h.cl << 2) | regs.h.ch) + 1;
	disk_params.sector = regs.h.cl & 0x3F;
	disk_params.head = regs.h.dh + 1;
	disk_params.drive = regs.h.dl;
	
    total_sectors = (uint64_t) (disk_params.head) * disk_params.track * disk_params.sector;
    total_bytes = total_sectors * SECTOR_SIZE;
    
    printf("Drive %d parameters:\n", disk_num);
    printf("Number of Drives: %d\n", disk_params.drive);
	printf("Number of Heads: %d\n", disk_params.head);
	printf("Number of Cylinders: %d\n", disk_params.track);
	printf("Number of Sectors per Cylinder: %d\n", disk_params.sector);
	printf("Total Number of Sectors: %lu\n", total_sectors);
	printf("Estimated number of raw bytes at 512 bytes per cluster: %lu\n", total_bytes);
    
	return disk_params;
}

int DISKREAD::verify_mbr(uint8_t drive_num)
{
    struct diskinfo_t di;
    uint8_t status;

    // Reset disks
	di.drive = 0x80 | drive_num;
    error_countdown = ERR_TIMER_START;
    printf("Resetting disk\n");
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
    printf("Verifying MBR\n");
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
    uint8_t fake_buf[1024] = {0};

    if (mbr_data == NULL) {
        fprintf(stderr, "*mbr_data must be allocated before DISKREAD::query_mbr() is called\n");
        return -1;
    }

    // Reset Disks
    di.drive = 0x80 | drive_num;
    //di.drive = drive_num;
    error_countdown = ERR_TIMER_START;
    printf("Resetting disk\n");
	do {
        status = _bios_disk(_DISK_RESET, &di) >> 8;
        if (status > 0) {
            handle_error(status, true);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -1;
            }
        }
    } while (status > 0 && error_countdown != ERR_TIMER_SKIP);

    di.drive = 0x80 | drive_num;
    //di.drive = drive_num;
	//di.drive = 0;
	di.head = 0;
    di.track = 0;
	di.sector = 1;
	di.nsectors = 1;
	di.buffer = mbr_data;

    error_countdown = ERR_TIMER_START;
    printf("Getting MBR\n");
    do {
        status = _bios_disk(_DISK_READ, &di) >> 8;
        //status = bios_disk_2(_DISK_READ, di.drive, di.head, di.track, di.sector, di.nsectors, fake_buf);
        if (status > 0) {
            handle_error(status, false);
            if (error_countdown == ERR_TIMER_CANCEL) {
                return -2;
            }
        }
    } while (status > 0);

    //display_buffer(mbr_data, SECTOR_SIZE);
    
    // Check MBR Signature
    //if ((uint16_t) mbr_data[SECTOR_SIZE-2] != 0xAA55) {
    //if ((uint16_t) mbr_data[SECTOR_SIZE-1] << 8 | mbr_data[SECTOR_SIZE-2] != (uint16_t) 0xAA55) {
    if (mbr_data[SECTOR_SIZE-2] != 0x55 || mbr_data[SECTOR_SIZE-1] != 0xAA) {
        //fprintf(stderr, "Couldn't find boot signature 0xAA55 in MBR. Found: %0X\n", (uint16_t) mbr_data[SECTOR_SIZE-2]);
        //fprintf(stderr, "Couldn't find boot signature 0xAA55 in MBR. Found: %0X\n", 
        //    (uint16_t) mbr_data[SECTOR_SIZE-1] << 8 | mbr_data[SECTOR_SIZE-2]);
        fprintf(stderr, "Couldn't find boot signature 0xAA55 in MBR. Found: %0X%0X\n", mbr_data[SECTOR_SIZE-1], mbr_data[SECTOR_SIZE-2]);
        return -3;
    }

    return 0;
}

int DISKREAD::get_track(uint8_t *track_data, int disk_num, int head_num, uint8_t track_num)
{
    if (track_data == NULL) {
        fprintf(stderr, "DISKREAD::get_track() called with a null-pointer in *track_data.\n");
        return -1;
    }

    if (disk_err_map == NULL) {
        disk_err_map = (struct disk_error_map*) malloc(DISK_ERR_ARRAY_LEN);
        if (disk_err_map == NULL) {
            fprintf(stderr, "Couldn't allocate memory to store disk error map. Try freeing TSRs.\n");
            return -1;
        }
        disk_error_count = 0;
        disk_err_map_len = DISK_ERR_ARRAY_LEN;
    } /*else if (disk_error_count == disk_err_map_len) {
        struct disk_error_map *old_ptr = disk_err_map;
        disk_err_map = realloc(disk_err_map, disk_err_map_len + DISK_ERR_ARRAY_LEN);
        if (disk_err_map == NULL) {
            free(old_ptr);
            old_ptr = NULL;
            fprintf(stderr, "Couldn't reallocate memory to store disk error map. Try freeing TSRs.\n");
            return -1;
        }
    }*/

    struct diskinfo_t di;
    uint8_t status = 1;

    di.drive = 0x80 | disk_num;
    di.head = head_num;
    di.track = track_num;
    di.sector = 1;
    di.nsectors = disk_params.sector;
    di.buffer = track_data;

    printf("Reading Track %d, Head %d\n", di.track, di.head);

    for (error_countdown = ERR_TIMER_START; status > 0 && error_countdown != ERR_TIMER_SKIP; handle_error(status, true)) {
    //error_countdown = ERR_TIMER_CANCEL;
    //do {
        if (error_countdown == ERR_TIMER_CANCEL) {
            free(disk_err_map);
            disk_err_map = NULL;
            return -2;
        }

        status = _bios_disk(_DISK_READ, &di) >> 8;
        //printf("Status: %0X\n", status);
        
        if (status > 0) {
            _bios_disk(_DISK_RESET, &di);
            //handle_error(status, true);
        }
    } //while (status > 0 && error_countdown != ERR_TIMER_SKIP);

    // If error is encountered, try to read each sector independently
    if (status > 0) {
        printf("Attempting to read each sector individually on track\n");
        di.nsectors = 1;
        for (int i = 1; i <= disk_params.sector; i++) {
            di.sector = i;
            di.buffer = track_data + (SECTOR_SIZE * i);

            printf("Sector %d of %d\n", di.sector, disk_params.sector);

            for (error_countdown = ERR_TIMER_CANCEL; status > 0 && error_countdown > 0; error_countdown--) {
                status = _bios_disk(_DISK_READ, &di) >> 8;

                if (status > 0) {
                    _bios_disk(_DISK_RESET, &di);
                }
            }

            if (status > 0) {
                disk_error(status);
                memset(di.buffer, 0x90, SECTOR_SIZE);   // Fill in sector with 0x90 (NOP Instruction for safety)

                disk_err_map[disk_error_count].head = di.head;
                disk_err_map[disk_error_count].track = di.track;
                disk_err_map[disk_error_count].sector = di.sector;
                disk_err_map[disk_error_count].err_val = status;
                disk_error_count++;

                if (disk_error_count == disk_err_map_len) {
                    struct disk_error_map *old_ptr = disk_err_map;
                    disk_err_map = (struct disk_error_map*) realloc(disk_err_map, disk_err_map_len + DISK_ERR_ARRAY_LEN);
                    if (disk_err_map == NULL) {
                        free(old_ptr);
                        old_ptr = NULL;
                        fprintf(stderr, "Couldn't reallocate memory to store disk error map. Try freeing TSRs.\n");
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

int DISKREAD::save_err_map(char *filename)
{
    char full_filename[270] = {0};
    FILE *file_ptr;
    if (strlen(filename) == 0) {
        fprintf(stderr, "save_err_map() called without specifying filename.\n");
    }

    strcpy(full_filename, filename);
    strcat(full_filename, ".TXT");

    printf("Writing %s to store disk error map. It lists the locations where errors were found.\n");

    file_ptr = fopen(full_filename, "w");
    if (file_ptr == NULL) {
        //TODO: Let user try to save multiple times
        fprintf(stderr, "Couldn't open %s for writing. Check to make sure you have the right disk.\n", full_filename);
        return 1;
    }

    for (int i = 0; i < disk_error_count; i++) {
        fprintf(file_ptr, "Track/Cylinder: %d, Head: %d, Sector: %d, Error: %d\n", 
            disk_err_map[i].track, disk_err_map[i].head, disk_err_map[i].sector, disk_err_map[i].err_val);
    }

    fclose(file_ptr);

    return 0;
}

int DISKREAD::get_disk_error_count(void)
{
    return disk_error_count;
}

void DISKREAD::handle_error(uint16_t status, bool skippable)
{
    char c;
    bool valid_input = false;

    if (status == 0) {
        return;
    }

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
            printf("\n");

            switch (c) {
                case 'Y': 
                case 'y':
                    valid_input = true;
                    error_countdown = ERR_TIMER_START;
                    break;
                case 'C': 
                case 'c':
                    valid_input = true;
                    error_countdown = ERR_TIMER_CANCEL;
                    break;
                case 'S': 
                case 's':
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
        
        fseek(stdin, 0, SEEK_END);
    }
}

void DISKREAD::disk_error(uint16_t status)
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

void DISKREAD::display_buffer(unsigned char *disk_buffer, uint16_t buffer_len)
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
    printf("\n");
}

uint16_t DISKREAD::bios_disk_2(uint8_t cmd, uint8_t drive, uint8_t head, uint16_t track, uint8_t sector, 
                                uint8_t nsectors, void *buffer)
{
    uint16_t i, err;
    struct SREGS sregs;
    union REGS regs;
    
    sregs.es = FP_SEG(buffer);
    regs.h.dh = head;
    regs.h.dl = drive;
    regs.h.ch = track;
    regs.h.cl = ((track >> 2) & 0xC0) | sector;
    regs.x.bx = FP_OFF(buffer);
    
    for (i = 0; i < 3; i++) {
        switch (cmd) {
            case _DISK_RESET:
                regs.h.ah = 0x00;
                break;
            case _DISK_STATUS:
                regs.h.ah = 0x01;
                break;
            case _DISK_READ:
                regs.h.ah = 0x02;
                break;
            case _DISK_WRITE:
                regs.h.ah = 0x03;
                break;
            case _DISK_VERIFY:
                regs.h.ah = 0x04;
                break;
            case _DISK_FORMAT:
                regs.h.ah = 0x05;
                break;
            default:
                return 1;   // Invalid Command
        }
        
        regs.h.al = nsectors;
        
        int86x(0x13, &regs, &regs, &sregs);
        
        err = regs.h.ah;
        
        if (err == 0) {
            return 0;
        }
        
        // Reset Disk and Try again
        regs.h.ah = 0;
        int86x(0x13, &regs, &regs, &sregs);
    }
    
    return err;
}