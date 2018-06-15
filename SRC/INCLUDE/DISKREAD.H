/**
 * DISKREAD.H - Disk Reading Class Header
 *   - Reads raw Sectors through BIOS Int 0x13 on Client
 */

#ifndef _DISKREAD_H_
#define _DISKREAD_H_

// Watcom Compiler doesn't like this - figure out later
//#ifdef defined(_WIN32) || defined(__APPLE__) || defined(__linux__) || defined(__unix__)
//#error DISKREAD class is designed to only be used with the client on a DOS target
//#endif

#include "NDIPTYPE.H"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <bios.h>
#include <i86.h>

#define ERR_TIMER_START       5
#define ERR_TIMER_CANCEL     -1
#define ERR_TIMER_SKIP       -2

#define DISK_ERR_ARRAY_LEN    5000

struct disk_error_map {
    uint16_t track;
    uint8_t head;    
    uint8_t sector;
    uint8_t err_val;
};

class DISKREAD
{
    public:
        static struct diskinfo_t query_disks(uint8_t disk_num);
        static int verify_mbr(uint8_t drive_num);
        static int query_mbr(uint8_t *mbr_data, int drive_num);
        static int get_track(uint8_t *track_data, uint8_t disk_num, uint8_t head_num, uint16_t track_num);
        static int save_err_map(char *filename);
        static int get_disk_error_count(void);

    private:
        DISKREAD(void);
        
        static uint8_t sec_buffer[SECTOR_SIZE];
        //static uint8_t sec_buffer[];
        static struct disk_error_map *disk_err_map;
        static struct diskinfo_t disk_params;
        static uint64_t total_sectors;
        static uint64_t cur_log_sector;
        static uint64_t total_bytes; 
        static int error_countdown;
        static int disk_error_count;
        static int disk_err_map_len;
    
        static void handle_error(uint16_t status, bool skippable);
        static void disk_error(uint16_t status);
        static void display_buffer(uint8_t *buffer, uint16_t buffer_len);
        static uint16_t bios_disk_2(uint8_t cmd, uint8_t drive, uint8_t head, uint16_t track, uint8_t sector, 
                                    uint8_t nsectors, void *buffer);
};

#endif