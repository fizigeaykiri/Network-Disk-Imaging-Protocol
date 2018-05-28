/**
 * DISKPKT.H - Disk Packet Class Header
 *  - Stores Disk Sector data sent in a packet
 */

#ifndef _DISKPKT_H_
#define _DISKPKT_H_

#include "NDIPTYPE.H"
#include <stdio.h>
#include <strings.h>


#define SECTOR_SIZE 512

#define PROTOCOL_VER 1

#define DECODE_PARAMS   0
#define DECODE_SECS     1


#define LOG_SEC_CUR 0
#define LOG_SEC_TOT 1

#define DISK_PARAM_DISK_NUM 0
#define DISK_PARAM_TRACK_NUM 1
#define DISK_PARAM_HEAD_NUM 2
#define DISK_PARAM_SECT_NUM 3

// Transmission status / error code
#define NDIP_CLIENT_SEND    0xFF
#define NDIP_OK             0x00

#define NDIP_ERR_DISK_CHG   0x01
#define NDIP_ERR_NAME_CHG   0x02
#define NDIP_ERR_DUP_SECT   0x03
#define NDIP_ERR_BAD_CRC    0x04
#define NDIP_ERR_OUT_RANGE  0x05
#define NDIP_INCOMP_TRACK   0x06
#define NDIP_INCOMP_HEAD    0x07
#define NDIP_NO_DISK_PARAM  0x08
#define NDIP_ERR_OPEN_FILE  0x09
#define NDIP_INVALID_PARAM  0x0A

#define NDIP_TRACK_COMPLETE 0x10
#define NDIP_HEAD_COMPLETE  0x20
#define NDIP_TRANS_COMPLETE 0x30


struct dsk_params {
    char file_name[16];
    uint16_t protocol_version;
    uint16_t disk_num;
    uint16_t track_count;
    uint16_t head_count;
    uint16_t sector_count;
    uint16_t param_crc;
};

struct dsk_sect_pkt {
    uint8_t sector[SECTOR_SIZE];
    char file_name[16];
    uint16_t protocol_version;
    uint16_t disk_num;
    uint16_t track_num;
    uint16_t head_num;
    uint16_t sector_num;
    //uint32_t logical_sector_num;
    //uint32_t total_sector_count;
    uint16_t packet_crc;
};

class DISKPKT
{
    public:
    DISKPKT(uint8_t *raw_stream, int mode);
    DISKPKT(char *file_name, uint16_t disk_num, uint16_t track_count, uint16_t head_count, uint16_t sector_count);
    DISKPKT(char *file_name, uint8_t *sector_data, uint16_t disk_num, uint16_t track_num, uint16_t head_num, uint16_t sector_num, int total_sector_count);
    uint8_t *get_sector_data(void);
    char *get_filename(void);
    uint32_t get_logical_sectors(int mode);
    //bool verify_checksum(void);
    uint8_t get_status_code(void);
    bool set_status_code(uint8_t code);
    uint8_t *get_raw_stream(int mode);
    uint16_t get_protocol_version(int mode);
    uint16_t get_disk_parameter(int disk_param);
    uint16_t get_sector_loc(int disk_param);

    private:
    struct dsk_params dparams;
    struct dsk_sect_pkt dsp;
    uint8_t status_code;
    
    //unsigned int calculate_checksum(char * data_stream);
    uint16_t calc_crc16(uint8_t *data_stream, int data_len);
    bool valid_crc(int mode);
    bool isLittleEndian(void);
};


#endif
