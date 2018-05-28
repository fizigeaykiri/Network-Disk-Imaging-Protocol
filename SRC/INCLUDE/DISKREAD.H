/**
 * DISKREAD.H - Disk Reading Class Header
 *   - Reads raw Sectors through BIOS Int 0x13 on Client
 */

#ifndef _DISKREAD_H_
#define _DISKREAD_H_

#ifdef defined(_WIN32) || defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#error DISKREAD class is designed to only be used with the client on a DOS target
#endif

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
    struct diskinfo_t query_disks(uint8_t disk_num);
    int verify_mbr(int drive_num);
    int query_mbr(int drive_num);
    uint8_t* get_track(int disk_num, int track_num);

    private:
    struct diskinfo_t disk_params;
    uint32_t total_sectors;
    uint32_t cur_log_sector;
    uint32_t total_bytes; 
    int error_countdown;
    
    void handle_error(uint8_t status, bool skippable);
    void disk_error(uint8_t status);
    void display_buffer(uint8_t *buffer, uint buffer_len);
}

#endif