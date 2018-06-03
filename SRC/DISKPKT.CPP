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

DISKPKT::DISKPKT(uint8_t *raw_stream, int mode)
{
    //int i = 0;
    switch (mode) {
        case DECODE_PARAMS:
            memset(&dparams, 0, sizeof(struct dsk_params));
            memcpy(&dparams, raw_stream, sizeof(struct dsk_params));
            if (!valid_crc(DECODE_PARAMS)) {
                fprintf(stderr, "Bad CRC on Disk Parameters\n");
                status_code = NDIP_ERR_BAD_CRC;
            } else {
                status_code = NDIP_OK;
            }
            break;
        case DECODE_SECS:
            memset(&dsp, 0, sizeof(struct dsk_sect_pkt));
            memcpy(&dsp, raw_stream, sizeof(struct dsk_sect_pkt));
            if (!valid_crc(DECODE_SECS)) {
                fprintf(stderr, "Bad CRC on Disk Sector Packet\n");
                status_code = NDIP_ERR_BAD_CRC;
            } else {
                status_code = NDIP_OK;
            }
            break;
        default:
            fprintf(stderr, "Invalid Decoding mode parameter to DISKPKT::DISKPKT: %d\n", mode);
            status_code = NDIP_INVALID_PARAM;
    }
}

DISKPKT::DISKPKT(char *file_name, uint16_t disk_num, uint16_t track_count, uint16_t head_count, uint16_t sector_count)
{
    strcpy(dsp.file_name, file_name);
    dparams.protocol_version = PROTOCOL_VER;
    dparams.disk_num = disk_num;
    dparams.track_count = track_count;
    dparams.head_count = head_count;
    dparams.sector_count = sector_count;

    // Networking Standard is Big-Endian, so if on a Little-Endian system, we need to reverse the bytes
    if (isLittleEndian()) {
        dparams.protocol_version = swap_uint16(dparams.protocol_version);
        dparams.disk_num = swap_uint16(dparams.disk_num);
        dparams.track_count = swap_uint16(dparams.track_count);
        dparams.head_count = swap_uint16(dparams.head_count);
        dparams.sector_count = swap_uint16(dparams.sector_count);
    }

    dparams.param_crc = calc_crc16((uint8_t *) &dparams, sizeof(struct dsk_params) - sizeof(uint16_t));

    if (isLittleEndian()) {
        dparams.param_crc = swap_uint16(dparams.param_crc);
    }

    status_code = NDIP_CLIENT_SEND;
}

//struct dsk_sect_pkt init(char *file_name, int disk_num, int track_num, int head_num, int sector_num, int total_sector_count)
DISKPKT::DISKPKT(char *file_name, uint8_t *sector_data, uint16_t disk_num, uint16_t track_num, uint16_t head_num, uint16_t sector_num, int total_sector_count)
{
    //struct dsk_sect_pkt dsp;
    //dsp.sector_data = sector_data;
    memcpy(dsp.sector, sector_data, SECTOR_SIZE);
    //dsp.file_name = file_name;
    strcpy(dsp.file_name, file_name);

    dsp.protocol_version = PROTOCOL_VER;
    dsp.disk_num = disk_num;
    dsp.track_num = track_num;
    dsp.head_num = head_num;
    dsp.sector_num = sector_num;
    //dsp.logical_sector_num = sector_num * head_num * track_num;
    //dsp.total_sector_count = total_sector_count;
    //dsp.total_sector_count = 0;

    // Networking Standard is Big-Endian, so if on a Little-Endian system, we need to reverse the bytes
    if (isLittleEndian()) {
        dsp.protocol_version = swap_uint16(dsp.protocol_version);
        dsp.disk_num = swap_uint16(dsp.disk_num);
        dsp.track_num = swap_uint16(dsp.track_num);
        dsp.head_num = swap_uint16(dsp.head_num);
        dsp.sector_num = swap_uint16(dsp.sector_num);
        //dsp.logical_sector_num = swap_uint32(dsp.logical_sector_num);
        //dsp.total_sector_count = swap_uint32(dsp.total_sector_count);
    }

    dsp.packet_crc = calc_crc16((uint8_t *) &dsp, sizeof(struct dsk_sect_pkt) - sizeof(uint16_t));

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

//TODO: FIXME
uint32_t DISKPKT::get_logical_sectors(int mode)
{
    switch (mode) {
        case LOG_SEC_CUR:
            //return dsp.logical_sector_num;
            return 1;
            break;
        case LOG_SEC_TOT:
            //return dsp.total_sector_count;
            return 1;
            break;
        default:
            fprintf(stderr, "Invalid mode parameter to DSKPKT::get_logical_sectors: %d\n", mode);
            return 0;
    }
}

uint8_t DISKPKT::get_status_code(void)
{
    return status_code;
}

bool DISKPKT::set_status_code(uint8_t code)
{
    if (code != NDIP_CLIENT_SEND && ((code & 0x0F) <= NDIP_INVALID_PARAM && (code & 0xF0) <= NDIP_TRANS_COMPLETE)) {
        status_code = code;
        return false;
    }

    return true;
}

uint8_t* DISKPKT::get_raw_stream(int mode)
{
    switch (mode) {
        case DECODE_PARAMS:
            return (uint8_t*) &dparams;
            break;
        case DECODE_SECS:
            return (uint8_t*) &dsp;
            break;
        default:
            fprintf(stderr, "Invalid Decoding mode parameter to DISKPKT::get_raw_stream: %d\n", mode);
    }

    return NULL;
}


uint16_t DISKPKT::get_protocol_version(int mode)
{
    switch (mode) {
        case DECODE_PARAMS:
            return (isLittleEndian()) ? swap_uint16(dparams.protocol_version) : dparams.protocol_version;
            break;
        case DECODE_SECS:
            return (isLittleEndian()) ? swap_uint16(dsp.protocol_version) : dsp.protocol_version;
            break;
        default:
            fprintf(stderr, "Invalid Decoding mode parameter to DISKPKT::get_protocol_version: %d\n", mode);
    }

    return 0;
}


uint16_t DISKPKT::get_disk_parameter(int disk_param)
{
    switch (disk_param) {
        case DISK_PARAM_DISK_NUM:
            return (isLittleEndian()) ? swap_uint16(dparams.disk_num) : dparams.disk_num;
            break;
        case DISK_PARAM_TRACK_NUM:
            return (isLittleEndian()) ? swap_uint16(dparams.track_count) : dparams.track_count;
            break;
        case DISK_PARAM_HEAD_NUM:
            return (isLittleEndian()) ? swap_uint16(dparams.head_count) : dparams.head_count;
            break;
        case DISK_PARAM_SECT_NUM:
            return (isLittleEndian()) ? swap_uint16(dparams.sector_count) : dparams.sector_count;
            break;
        default:
            fprintf(stderr, "Invalid disk parameter to DISKPKT::get_disk_parameter: %d\n", disk_param);
    }

    return 0xFFFF;
}

uint16_t DISKPKT::get_sector_loc(int disk_param)
{
    switch (disk_param) {
        case DISK_PARAM_DISK_NUM:
            return (isLittleEndian()) ? swap_uint16(dsp.disk_num) : dsp.disk_num;
            break;
        case DISK_PARAM_TRACK_NUM:
            return (isLittleEndian()) ? swap_uint16(dsp.track_num) : dsp.track_num;
            break;
        case DISK_PARAM_HEAD_NUM:
            return (isLittleEndian()) ? swap_uint16(dsp.head_num) : dsp.head_num;
            break;
        case DISK_PARAM_SECT_NUM:
            return (isLittleEndian()) ? swap_uint16(dsp.sector_num) : dsp.sector_num;
            break;
        default:
            fprintf(stderr, "Invalid disk parameter to DISKPKT::get_sector_loc: %d\n", disk_param);
    }

    return 0xFFFF;
}

struct dsk_sect_pkt *DISKPKT::get_sector_struct(void)
{
    return &dsp;
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
uint16_t DISKPKT::calc_crc16(uint8_t *data_stream, int data_len)
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

    printf("CRC Return: 0x%X\n", crc);

    return crc;
}

bool DISKPKT::valid_crc(int mode)
{
    uint16_t real_crc = 0;

    switch (mode) {
        case DECODE_PARAMS:
            real_crc = calc_crc16((uint8_t*) &dparams, sizeof(struct dsk_params) - sizeof(uint16_t));
            if (real_crc == (isLittleEndian()) ? swap_uint16(dparams.param_crc) : dparams.param_crc) {
                printf("dparams CRC is good\n");
                return true;
            }
            return false;
            break;
        case DECODE_SECS:
            real_crc = calc_crc16((uint8_t*) &dsp, sizeof(struct dsk_sect_pkt) - sizeof(uint16_t));
            if (real_crc == (isLittleEndian()) ? swap_uint16(dsp.packet_crc) : dsp.packet_crc) {
                printf("dsp CRC is good\n");
                return true;
            }
            return false;
            break;
        default:
            fprintf(stderr, "Invalid Decoding mode parameter to DISKPKT::valid_crc: %d\n", mode);
    }

    return false;
}

bool DISKPKT::isLittleEndian(void)
{
    uint16_t i = 1;
    uint8_t *c = (uint8_t*) &i;
    if (*c) {
        return true;
    } else {
        return false;
    }
}

void DISKPKT::handle_status_error(uint8_t status_code)
{
    switch (status_code) {
        case NDIP_CLIENT_SEND: 
        case NDIP_OK: 
        case NDIP_TRACK_COMPLETE: 
        case NDIP_HEAD_COMPLETE: 
        case NDIP_TRANS_COMPLETE:
            break;
        case NDIP_ERR_DISK_CHG:
            fprintf(stderr, "Detected disk number change.\n");
            break;
        case NDIP_ERR_NAME_CHG:
            fprintf(stderr, "Detected filename change.\n");
            break;
        case NDIP_ERR_DUP_SECT:
            fprintf(stderr, "Duplicate sector data was sent again.\n");
            break;
        case NDIP_ERR_BAD_CRC:
            fprintf(stderr, "Sent data had a bad CRC number.\n");
            break;
        case NDIP_ERR_OUT_RANGE:
            fprintf(stderr, "Sent sector data was out of the range of disk geometry.\n");
            break;
        case NDIP_INCOMP_TRACK:
            fprintf(stderr, "Data was sent on next track/cylinder before previous one finished.\n");
            break;
        case NDIP_INCOMP_HEAD:
            fprintf(stderr, "Data was sent on next head before previous one finished.\n");
            break;
        case NDIP_NO_DISK_PARAM:
            fprintf(stderr, "Disk parameters need to be sent to server before sector data.\n");
            break;
        case NDIP_ERR_OPEN_FILE:
            fprintf(stderr, "Server couldn't open new image file for writing.\n");
            break;
        case NDIP_INVALID_PARAM:
            fprintf(stderr, "Server received invalid disk parameters.\n");
            break;
        default:
            return;
    }
}