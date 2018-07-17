

#include "DISKPKT.H"
//#include "DISKREAD.H"
#include <assert.h>


//uint8_t buffer[4096];

uint8_t sector[1024] = {0x20};

int diskpkt_test(void);
int diskread_test(void);
void display_buffer(uint8_t *buffer, size_t buffer_len);

int main(void)
{
    int code = 0;
    printf("TESTS: NDIP Unit Tests\n");
    code = diskpkt_test();
    //code = diskread_test();

    return code;
}

int diskpkt_test(void)
{
    uint16_t i;
    uint8_t *buffer;

    printf("diskpkt_test()\n");

    // Test 1: Set Disk Parameters
    DISKPKT *dp = new DISKPKT("testfile.txt", 0, 80, 2, 18);
    // Verify params
    //assert(dp.get_disk_parameter(DISK_PARAM_DISK_NUM) == 0);
    i = dp->get_disk_parameter(DISK_PARAM_DISK_NUM);
    printf("Disk Number: %d\n", i);
    assert(i == 0);
    //assert(dp.get_disk_parameter(DISK_PARAM_TRACK_NUM) == 80);
    i = dp->get_disk_parameter(DISK_PARAM_TRACK_NUM);
    printf("Number of Tracks: %d\n", i);
    assert(i == 80);
    //assert(dp.get_disk_parameter(DISK_PARAM_HEAD_NUM) == 2);
    i = dp->get_disk_parameter(DISK_PARAM_HEAD_NUM);
    printf("Number of Heads: %d\n", i);
    assert(i == 2);
    //assert(dp.get_disk_parameter(DISK_PARAM_SECT_NUM) == 18);
    i = dp->get_disk_parameter(DISK_PARAM_SECT_NUM);
    printf("Number of Sectors per Track: %d\n", i);
    assert(i == 18);

    printf("DP Status Code: 0x%X\n", dp->get_status_code());

    // Test 2: Get Raw Stream, create new object, and verify CRC
    buffer = dp->get_raw_stream(DECODE_PARAMS);
    printf("Raw Disk Parameters:\n");
    display_buffer(buffer, sizeof(struct dsk_params));
    DISKPKT *dp2 = new DISKPKT(buffer, DECODE_PARAMS);
    delete dp;
    printf("DP2 Status Code: 0x%X\n", dp2->get_status_code());

    // Test 3: Set Sector Data
    DISKPKT *dp3 = new DISKPKT("testfile.txt", sector, 0, 0, 0, 1, 2880);
    delete dp2;
    // Verify params
    i = dp3->get_sector_loc(DISK_PARAM_DISK_NUM);
    printf("Disk Number: %d\n", i);
    assert(i == 0);
    i = dp3->get_sector_loc(DISK_PARAM_TRACK_NUM);
    printf("Track Number: %d\n", i);
    assert(i == 0);
    i = dp3->get_sector_loc(DISK_PARAM_HEAD_NUM);
    printf("Head Number: %d\n", i);
    assert(i == 0);
    i = dp3->get_sector_loc(DISK_PARAM_SECT_NUM);
    printf("Sector Number: %d\n", i);
    assert(i == 1);

    printf("DP3 Status Code: 0x%X\n", dp3->get_status_code());

    // Test 4: Get Raw Stream, create new object, and verify CRC
    buffer = dp3->get_raw_stream(DECODE_SECS);
    printf("Raw Disk Sector:\n");
    display_buffer(buffer, sizeof(struct dsk_sect_pkt));
    DISKPKT *dp4 = new DISKPKT(buffer, DECODE_SECS);
    delete dp3;
    printf("DP4 Status Code: 0x%X\n", dp4->get_status_code());

    delete dp4;
    
    return 0;
}

/*
int diskread_test(void)
{
    printf("diskread_test()\n");
    
    unsigned char *other_buffer = (unsigned char *) malloc(SECTOR_SIZE);
    if (other_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate memory.\n");
        return 1;
    }
    
    struct diskinfo_t dparams = DISKREAD::query_disks(0);
    assert(dparams.drive > 0);
    
    assert(DISKREAD::query_mbr(other_buffer, 0) == 0);

    free(other_buffer);
    other_buffer = NULL;
   
    return 0;
}
*/

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