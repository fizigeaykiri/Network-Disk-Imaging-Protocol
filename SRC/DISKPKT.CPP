/**
 * DISKPKT.CPP - Disk Packet Class
 *  - Stores Disk Sector data sent in a packet
 */

#define SECTOR_SIZE 512

struct dsk_sect_pkt {
    unsigned char sector[SECTOR_SIZE];
    unsigned char file_name[16];
    unsigned int disk_num;
    unsigned int track_num;
    unsigned int head_num;
    unsigned int sector_num;
    unsigned long logical_sector_num;
    unsigned long total_sector_count;
    unsigned int packet_checksum;
};

struct dsk_sect_pkt init(char *file_name, int disk_num, int track_num, int head_num, int sector_num, int total_sector_count)
{
    struct dsk_sect_pkt dsp;
    dsp.file_name = file_name;
    dsp.disk_num = disk_num;
    dsp.track_num = track_num;
    dsp.head_num = head_num;
    dsp.sector_num = sector_num;
    dsp.logical_sector_num = SECTOR_SIZE * sector_num * head_num * track_num;
    dsp.total_sector_count = total_sector_count;

    return dsp;
}

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

