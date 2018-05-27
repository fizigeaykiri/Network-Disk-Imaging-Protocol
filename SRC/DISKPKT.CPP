/**
 * DISKPKT.CPP - Disk Packet Class
 *  - Stores Disk Sector data sent in a packet
 */

#include "DISKPKT.H"

//#define SECTOR_SIZE 512

/*
DISKPKT::DISKPKT(void)
{
    fprintf(stderr, "Cannot declare DISKPKT class without any arguments.\n");
}
*/

DISKPKT::DISKPKT(char *file_name, uint16_t disk_num, uint16_t track_count, uint16_t head_count, uint16_t sector_count)
{
    strcpy(dsp.file_name, file_name);
    dparams.disk_num = disk_num;
    dparams.track_count = track_count;
    dparams.head_count = head_count;
    dparams.sector_count = sector_count;

    // Networking Standard is Big-Endian, so if on a Little-Endian system, we need to reverse the bytes
    if (isLittleEndian()) {
        dparams.disk_num = swap_uint16(dparams.disk_num);
        dparams.track_count = swap_uint16(dparams.track_count);
        dparams.head_count = swap_uint16(dparams.head_count);
        dparams.sector_count = swap_uint16(dparams.sector_count);
    }

    dparams.param_crc = calc_crc16((uint8_t *) &dparams, sizeof(struct dsk_params) - sizeof(u_int16_t));

    if (isLittleEndian()) {
        dparams.param_crc = swap_uint16(dparams.param_crc);
    }
}

//struct dsk_sect_pkt init(char *file_name, int disk_num, int track_num, int head_num, int sector_num, int total_sector_count)
DISKPKT::DISKPKT(char *file_name, uint8_t *sector_data, uint16_t disk_num, uint16_t track_num, uint16_t head_num, uint16_t sector_num, int total_sector_count)
{
    //struct dsk_sect_pkt dsp;
    //dsp.file_name = file_name;
    strcpy(dsp.file_name, file_name);
    //dsp.sector_data = sector_data;
    memcpy(dsp.sector, sector_data, SECTOR_SIZE);

    dsp.disk_num = disk_num;
    dsp.track_num = track_num;
    dsp.head_num = head_num;
    dsp.sector_num = sector_num;
    dsp.logical_sector_num = SECTOR_SIZE * sector_num * head_num * track_num;
    dsp.total_sector_count = total_sector_count;

    // Networking Standard is Big-Endian, so if on a Little-Endian system, we need to reverse the bytes
    if (isLittleEndian()) {
        dsp.disk_num = swap_uint16(dsp.disk_num);
        dsp.track_num = swap_uint16(dsp.track_num);
        dsp.head_num = swap_uint16(dsp.head_num);
        dsp.sector_num = swap_uint16(dsp.sector_num);
        dsp.logical_sector_num = swap_uint32(dsp.logical_sector_num);
        dsp.total_sector_count = swap_uint32(dsp.total_sector_count);
    }

    dsp.packet_crc = calc_crc16((uint8_t *) &dsp, sizeof(struct dsk_sect_pkt) - sizeof(u_int16_t));

    if (isLittleEndian()) {
        dsp.packet_crc = swap_uint16(dsp.packet_crc);
    }

    status_code = NDIP_CLIENT_SEND;

    //return dsp;
}

uint8_t *DISKPKT::get_sector_data(void)
{
    return dsp.sector;
}

char *DISKPKT::get_filename(void)
{
    return dsp.file_name;
}

uint32_t DISKPKT::get_logical_sectors(int mode)
{
    switch (mode) {
        case LOG_SEC_CUR:
            return dsp.logical_sector_num;
            break;
        case LOG_SEC_TOT:
            return dsp.total_sector_count;
            break;
        default:
            fprintf(stderr, "Invalid mode parameter to DSKPKT::get_logical_sectors: %d", mode);
            return 0;
    }
}

/*
unsigned int calculate_checksum(dsk_sect_pkt *dsp)   // experimental CRC
{
    int crc_table[1024];
    int ret_val = 0xFFFF;
    int lookup, i;

    for (i = 0; i < sizeof(struct dsk_sect_pkt); i++) {
        lookup = (ret_val ^ (unsigned char) (dsp[i])) & 0xFF;
        ret_val = (ret_val << 8) ^ crc_table[lookup];
    }

    ret_val = ret_val ^ 0xFFFF;

    return ret_val;
}
*/

// Reference: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html
uint16_t calc_crc16(uint8_t *data_stream, int data_len)
{
    uint16_t generator = 0x1021;
    uint16_t crc = 0;
    int i, j;

    for (i = 0; i < data_len; i++) {
        crc ^= (data_stream[i] << 8);

        for (j = 0; j < 8; j++) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ generator;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

bool isLittleEndian(void)
{
    uint16_t i = 1;
    uint8_t *c = (uint8_t*) &i;
    if (*c) {
        return true;
    } else {
        return false;
    }
}

