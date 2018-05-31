/**
 * NDIPCL.CPP - NDIP Client
 * 2018 By @marssaturn91
 */

#include <stdio.h>
#include <bios.h>

#include "NDIPTYPE.H"
#include "CLSOCKET.H"
#include "DISKREAD.H"

// Trap Ctrl-Break and Ctrl-C to that we can unhook the timer interrupt
//  and shutdown cleanly.

// Check this flag once in a while to see if the user wants out.
volatile uint8_t CtrlBreakDetected = 0;

void (__interrupt __far *oldCtrlBreakHandler)();

void __interrupt __far ctrlBreakHandler()
{
    CtrlBreakDetected = 1;
}

void __interrupt __far ctrlCHandler()
{
    // Do Nothing
}

void usage(void);
struct diskinfo_t get_disk(void);
int send_disk_params(DISKPKT *dParamPkt);
int get_mbr(int disk_num, char *file_name);
int send_mbr(uint8_t *mbr_data, char *file_name, int disk_num);
void setup_interrupts(void);
void shutdown_client(void);

int main(int argc, char *argv[])
{
    struct diskinfo_t dparams;
    //int disk_num;
    int ret_code;
    char file_name[16];

    printf("NDIPCL - NDIP (Network Disk Imaging Protocol) Client\n");
    printf("2018 by @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    if (argc < 2) {
        usage();
        return 0;
    }

    //disk_num = get_disk();
    dparams = get_disk();
    /*
    if (disk_num == -1) {
        return 1;
    } else if (disk_num == -2) {
        return 0;
    }
    */

    printf("Please enter the filename of the image: ");
    if (fgets(file_name, 16, stdin) == NULL) {
        fprintf(stderr, "Error processing filename.\n");
        return 1;
    }
    
    if (CLSOCKET::initialize() > 0) {
        return 1;
    }

    setup_interrupts();

    if (CLSOCKET::connect_server(argv[1]) != 0) {
        shutdown_client();
        return 1;    
    }

    DISKPKT dParamPkt = new DISKPKT(file_name, NULL, di.drive, di.track, di.head, di.sector, 0);
    ret_code = send_disk_params(dParamPkt);
    if (ret_code >= 4) {
        delete dParamPkt;
        shutdown_client();
        return 1;
    }

    delete dParamPkt;

    if (get_mbr(disk_num, file_name) != 0) {
        shutdown_client();
        return 1;
    }
    

    shutdown_client();

    return 0;
}

void usage(void)
{
    printf("Usage:\n");
    printf("ndipcl <server IP address | server domain name>\n");
}

struct diskinfo_t get_disk(void)
{
    char c;
    char *c2 = "X";
    int8_t drive_num = 0;
    struct diskinfo_t di = DISKREAD::query_disks(0);
    if (di.drive == 0) {
        printf("No hard drives were found. If your system has a hard drive, check and make\n");
        printf("sure your BIOS can access it through software interrupt 0x13h.\n");
        //return -1;
        return NULL;
    }

    printf("%d hard drives were detected. Please select which hard drive you'd like to\n", di.drive);
    printf("image. (1 through %d): ", di.drive);

    do {
        c = getchar();
        c2[0] = c;
        drive_num = atoi(c2);
    } while (drive_num <= 0 || drive_num > di.drive);
    
    drive_num--;

    di = DISKREAD::query_disk(drive_num);

    printf("\nYour selected disk has the parameters above. Do you wish to proceed? (Y/N): ");

    do {
        c = getchar();
        if (c == 'n' || c == 'N') {
            printf("\n");
            //return -2;
            return NULL;
        }
    } while (c != 'y' && c != 'Y');
    printf("\n");

    //return drive_num;
    return di;
}

int send_disk_params(DISKPKT *dParamPkt)
{
    // Modes
    // 0 - wait for OK status from server
    // 1 - send disk parameters
    // 2 - wait for status from server
    // 3 - done, success
    // 4 - done, fail
    // 5 - break or termination


    //TODO: Set Modes to control order of receiving and sending
    uint8_t status_code;
    uint8_t done = 0;
    uint8_t mode = 0;
    uint8_t *recv_buffer = malloc(2000);
    if (recv_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for receive buffer.\n");
        return -1;
    }

    while (!done) {
        if (CtrlBreakDetected) {
            printf("\nCtrl-Break detected\n");
            done = 1;
            mode = 5;
        }

        // Service the connection
        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();
        
        if (mySocket->isRemoteClosed()) {
            done = 1;
            mode = 5;
        }

        // Get Status Code
        if (mode == 0 || mode == 2) {
            int16_t recvRc = CLSOCKET::recv_data(recv_buffer, 2000);
            if (recvRc > 0) {
                status_code = recv_buffer[0];

                if (status_code > NDIP_OK) {
                    fprintf(stderr, "Error from server: ");
                    dParamPkt->handle_status_error(status_code);
                    mode = 4;
                    done = 1;
                } else {
                    mode++;
                }
            }
        }

        // Send Disk Parameters
        if (mode == 2) {
            status_code = dParamPkt->get_status_code();
            if (CLSOCKET::send_data(&status_code) < 1) {
                fprintf(stderr, "Unable to send packet to server.\n");
            }
            if (CLSOCKET::send_data(dParamPkt->get_raw_stream(DECODE_SECS)) < 1) {
                fprintf(stderr, "Unable to send packet to server.\n");
            } else {
                mode = 3;
            }
        }

        if (mode == 3) {
            done = 1;
        }

        /*
        // Get Status Code
        int16_t recvRc = CLSOCKET::recv_data(recv_buffer, 2000);
        if (recvRc > 0) {
            status_code = recv_buffer[0];
        }
        */
    }
    
    free(recv_buffer);
    recv_buffer = NULL;

    return mode;
}

int get_mbr(int disk_num, char *file_name)
{
    int ret_val;
    uint8_t *mbr_data = malloc(SECTOR_SIZE);
    if (mbr_data == NULL) {
        fprintf(stderr, "Couldn't allocate memory to store MBR data. Try removing some TSRs.\n");
        return 1;
    }

    if (DISKREAD::verify_mbr(disk_num) != 0) {
        free(mbr_data);
        mbr_data = NULL;
        return 1;
    }

    if (DISKREAD::query_mbr(mbr_data, disk_num) != 0) {
        free(mbr_data);
        mbr_data = NULL;
        return 1;
    }

    ret_val = send_mbr(mbr_data, file_name, disk_num);

    free(mbr_data);
    mbr_data = NULL;
}

int send_mbr(uint8_t *mbr_data, char *file_name, int disk_num)
{
    // Modes
    // 0 - send MBR data
    // 1 - wait for status
    // 2 - done, success
    // 3 - done, fail
    // 4 - done, break or termination

    if (mbr_data == NULL) {
        fprintf(stderr, "send_mbr() was called with a Null-Pointer for mbr_data parameter.\n");
        return 1;
    }

    uint8_t *recv_buffer = malloc(2000);
    if (recv_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for receiv buffer.\n");
        return 1;
    }

    DISKPKT mbrPkt = new DISKPKT(file_name, mbr_data, disk_num, 0, 0, 1, 1);
    int done = 0;
    int mode = 0;
    uint8_t status_code;

    while (!done) {
        if (CtrlBreakDetected) {
            printf("\nCtrl-Break detected\n");
            done = 1;
            mode = 4;
        }

        // Service the connection
        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();
        
        if (mySocket->isRemoteClosed()) {
            done = 1;
            mode = 4;
        }

        if (mode == 0) {
            status_code = mbrPkt->get_status_code();
            if (CLSOCKET::send_data(&status_code) < 1) {
                fprintf(stderr, "Unable to send packet to server.\n");
            }
            if (CLSOCKET::send_data(mbrPkt->get_raw_stream(DECODE_SECS)) < 1) {
                fprintf(stderr, "Unable to send packet to server.\n");
            } else {
                mode = 1;
            }
        }

        if (mode == 1) {
            int16_t recvRc = CLSOCKET::recv_data(recv_buffer, 2000);
            if (recvRc > 0) {
                status_code = recv_buffer[0];

                if (status_code > NDIP_OK) {
                    fprintf(stderr, "Error from server: ");
                    dParamPkt->handle_status_error(status_code);
                    mode = 3;
                    done = 1;
                } else {
                    mode++;
                }
            }
        }

        if (mode == 2) {
            done = 1;
        }
    }


    delete mbrPkt;
    free(recv_buffer);
    recv_buffer = NULL;

    return mode;
}

void setup_interrupts(void)
{
    // Save off the oldCtrlBreakHandler and put our own in. Shutdown() 
    //  will restore the original handler for us.
    oldCtrlBreakHandler = getvect(0x1B);
    setvect(0x1B, ctrlBreakHandler);

    setvect(0x23, ctrlCHandler);

}

void shutdown_client(void)
{
    CLSOCKET::shutdown();
    setvect(0x1B, oldCtrlBreakhandler);
}