/**
 * NDIPCL.CPP - NDIP Client
 * 2018 By @marssaturn91
 */

#include <stdio.h>

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
void setup_interrupts(void);
void shutdown_client(void);

int main(int argc, char *argv[])
{
    int disk_num;
    char file_name[16];

    printf("NDIPCL - NDIP (Network Disk Imaging Protocol) Client\n");
    printf("2018 by @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    if (argc < 2) {
        usage();
        return 0;
    }

    disk_num = get_disk();
    if (disk_num == -1) {
        return 1;
    } else if (disk_num == -2) {
        return 0;
    }

    printf("Please enter the filename of the image: ");
    //TODO: Get FileName

    if (CLSOCKET::initialize() > 0) {
        return 1;
    }

    setup_interrupts();

    if (CLSOCKET::connect_server(argv[1]) != 0) {
        shutdown_client();
        return 1;    
    }

    //TODO: Send Disk Parameters

    if (get_mbr(disk_num) != 0) {
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

int get_disk(void)
{
    char c;
    char *c2 = "X";
    int8_t drive_num = 0;
    struct diskinfo_t di = DISKREAD::query_disks(0);
    if (di.drive == 0) {
        printf("No hard drives were found. If your system has a hard drive, check and make\n");
        printf("sure your BIOS can access it through software interrupt 0x13h.\n");
        return -1;
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
            return -2;
        }
    } while (c != 'y' && c != 'Y');
    printf("\n");

    return drive_num;
}

int get_mbr(int disk_num)
{
    uint8_t mbr_data = malloc(SECTOR_SIZE);
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




    return 0;
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