/**
 * NDIPServ.cpp - NDIP Server
 */

#include <stdio.h>

#include "NDIPTYPE.H"
#include "DISKPKT.H"
#include "SrvSocket.h"

uint8_t mbr[SECTOR_SIZE] = {0};

int main(int argc, char *argv[])
{
    struct dsk_sect_pkt dparams;
    printf("NDIPServ\n");

    if (setup() > 0) {
        fprintf(stderr, "Issue occurred initializing server.\n");
        return 1;
    }

    if (get_parameters(&dparams) > 0) {
        return 1;
    }

    if (get_mbr(&dparams) > 0) {
        return 1;
    }

    printf("Disk MBR:\n");
    display_buffer(mbr, SECTOR_SIZE);

    return 0;
}

int setup()
{
    if (SrvSocket::initialize() > 0) {
        return 1;
    }

    if (SrvSocket::serv_listen() > 0) {
        return 1;
    }

    return 0;
}

int get_parameters(struct dsk_params *dparams)
{
    // set status code
    uint8_t *recv_buffer = malloc(2000);
    int mode = 0;
    uint8_t status_code = NDIP_OK;
    
    if (recv_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for receive buffer.\n");
        return 1;
    }

    if (dparams == NULL) {
        fprintf(stderr, "Null-Pointer passed into get_parameters.\n");
        return 1;
    }

    while (mode < 3) {
        if (SrvSocket::send_data(&status_code, 1) <= 0) {
            fprintf(stderr, "Failed to send packet\n");
        } else {
            mode = 1;
        }

        if (mode == 1) {
            if (SrvSocket::recv_data(&status_code, 1) > 0) {
                if (status_code != NDIP_CLIENT_SEND) {
                    fprintf(stderr, "Unexpected Status Code sent from Client: %0X\n", status_code);
                    free(recv_buffer);
                    recv_buffer = NULL;
                    return 1;
                } else {
                    mode = 2;
                }
            }
        }

        if (mode == 2) {
            if (SrvSocket::recv_data(recv_buffer, 2000) > 0) {
                DISKPKT *dpkt = new DISKPKT(recv_buffer, DECODE_SECS);
                //Todo: check items
                if (dpkt->get_status_code() == NDIP_OK) {
                    *dparams = dpkt->get_sector_struct();
                } else {
                    fprintf(stderr, "An error occurred receiving the data from the client: %0X\n", dpkt->get_status_code());
                    delete dpkt;
                    free(recv_buffer);
                    recv_buffer = NULL;
                    return 1;
                }
                // Send Status Code
                if (SrvSocket::send_data(&(dpkt->get_status_code()), 1) <= 0) {
                    fprintf(stderr, "Failed to send packet\n");
                } else {
                    mode = 3;
                }

                delete dpkt;
            }
        }
    }
    
    
    free(recv_buffer);
    recv_buffer = NULL;
    return 0;
}

int get_mbr(struct dsk_sect_pkt *dparams)
{
    uint8_t *recv_buffer = malloc(2000);
    int mode = 0;
    uint8_t status_code = NDIP_OK;

    if (recv_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for receive buffer.\n");
        return 1;
    }

    while (mode < 2) {
        if (mode == 0) {
            if (SrvSocket::recv_data(&status_code, 1) > 0) {
                if (status_code != NDIP_CLIENT_SEND) {
                    fprintf(stderr, "Unexpected Status Code sent from Client: %0X\n", status_code);
                    free(recv_buffer);
                    recv_buffer = NULL;
                    return 1;
                } else {
                    mode = 1;
                }
            }
        }

        if (mode == 1) {
            if (SrvSocket::recv_data(recv_buffer, 2000) > 0) {
                DISKPKT *dpkt = new DISKPKT(recv_buffer, DECODE_SECS);
                
                if (dpkt->get_status_code() == NDIP_OK) {
                    if (strcmp(dparams->file_name, dpkt->get_filename()) != 0) {
                        dpkt->set_status_code(NDIP_ERR_NAME_CHG);
                    } else if (dparams->disk_num != dpkt->get_sector_loc(DISK_PARAM_DISK_NUM)) {
                        dpkt->set_status_code(NDIP_ERR_DISK_CHG);
                    } else {
                        bool out_of_range = (dparams->track_num <= dpkt->get_sector_loc(DISK_PARAM_TRACK_NUM)) ? true :
                                            (dparams->head_num <= dpkt->get_sector_loc(DISK_PARAM_HEAD_NUM)) ? true :
                                            (dparams->sector_num < dpkt->get_sector_loc(DISK_PARAM_SECT_NUM)) ? true : false;   // Not <= because Sectors start at 1

                        if (out_of_range) {
                            dpkt->set_status_code(NDIP_ERR_OUT_RANGE);
                        }
                    }
                } else {
                    fprintf(stderr, "An error occurred receiving the data from the client: %0X\n", dpkt->get_status_code());
                    delete dpkt;
                    free(recv_buffer);
                    recv_buffer = NULL;
                    return 1;
                }
                // Send Status Code
                if (dpkt->get_status_code() != NDIP_OK) {
                    dpkt->handle_status_error(dpkt->get_status_code());
                }
                if (SrvSocket::send_data(&(dpkt->get_status_code()), 1) <= 0) {
                    fprintf(stderr, "Failed to send packet\n");
                } else {
                    memcpy(mbr, dpkt->get_sector_data(), SECTOR_SIZE);
                    mode = 2;
                }

                delete *dpkt;
            }
        }


    }

    
    free(recv_buffer);
    recv_buffer = NULL
    return 0;
}

void display_buffer(uint8_t *buffer, size_t buffer_len)
{
	int i = 0;
	int j = 0;
	
	for (i = 0; i < buffer_len; i++) {
		printf("%02X ", buffer[i]);
		j += 3;
		
		if (j >= 78) {
			j = 0;
			printf("\n");
		}
	}

    printf("\n");
}

