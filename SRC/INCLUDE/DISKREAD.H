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
#include <bios.h>
#include <i86.h>

#define ERR_TIMER_START       5
#define ERR_TIMER_CANCEL     -1
#define ERR_TIMER_SKIP       -2

class DISKREAD
{
    public:
        static struct diskinfo_t query_disks(uint8_t disk_num);
        static int verify_mbr(uint8_t drive_num);
        static int query_mbr(uint8_t *mbr_data, int drive_num);
        static int get_track(uint8_t *track_data, int disk_num, int track_num);

    private:
        DISKREAD(void);
        
        static uint8_t sec_buffer[SECTOR_SIZE];
        //static uint8_t sec_buffer[];
        static struct diskinfo_t disk_params;
        static uint32_t total_sectors;
        static uint32_t cur_log_sector;
        static uint32_t total_bytes; 
        static int error_countdown;
    
        static void handle_error(uint8_t status, bool skippable);
        static void disk_error(uint8_t status);
        //static void display_buffer(uint8_t *buffer, uint buffer_len);
};

#endif