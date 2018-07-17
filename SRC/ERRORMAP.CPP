/**
 * ERRORMAP.CPP - NDIP Error Mapper
 * 2018 By @marssaturn91
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "NDIPTYPE.H"
#include "DISKREAD.H"

struct diskinfo_t *get_disk(void);
int read_disk(struct diskinfo_t *dparams);
int save_error_map(void);

char file_path[256] = {0};

int main(int argc, char *argv[])
{
    struct diskinfo_t *dparams;

    printf("ERRORMAP - NDIP Error Mapper\n");
    printf(" - Scans disk for errors and saves a log detailing them.\n");
    printf("2018 by @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    DISKREAD::set_auto_skip_error(true);

    dparams = get_disk();
    if (dparams == NULL) {
        return 1;
    }

    printf("Enter drive and path of error log file.\n");
    printf(">");

    do {
        if (fgets(file_path, 256, stdin) == NULL) {
            fprintf(stderr, "Error processing filepath.\n");
            free(dparams);
            dparams = NULL;
            return 1;
        }
    } while (strcmp(file_path, "\n") == 0);

    strtok(file_path, "\n");

    if (read_disk(dparams) > 1) {
        free(dparams);
        dparams = NULL;
        return 1;
    }

    if (save_error_map() > 1) {
        free(dparams);
        dparams = NULL;
        return 1;
    }

    free(dparams);
    dparams = NULL;

    return 0;
}

struct diskinfo_t *get_disk(void)
{
    char c;
    char *c2 = "X";
    int8_t drive_num = 0;

    struct diskinfo_t *di = (struct diskinfo_t *) malloc(sizeof(struct diskinfo_t));
    if (di == NULL) {
        fprintf(stderr, "Couldn't allocate memory to store the disk parameters. Try freeing up TSRs.\n");
        return NULL;
    }

    memcpy(di, &DISKREAD::query_disks(0), sizeof(struct diskinfo_t));

    if (di->drive == 0) {
        printf("No hard drives were found. If your system has a hard drive, check and make\n");
        printf("sure your BIOS can access it through software interrupt 0x13h.\n");
        free(di);
        di = NULL;
        return NULL;
    }

    printf("%d hard drive(s) were detected. Please select which hard drive you'd like to\n", di->drive);
    printf("scan for errors. (1 through %d): ", di->drive);

    do {
        c = getchar();
        c2[0] = c;
        drive_num = atoi(c2);
    } while (drive_num <= 0 || drive_num > di->drive);

    drive_num--;

    memcpy(di, &DISKREAD::query_disks(drive_num), sizeof(struct diskinfo_t));

    printf("\nYour selected disk has the parameters above. Do you wish to proceed? (Y/N): ");

    do {
        c = getchar();
        if (c == 'n' || c == 'N') {
            printf("\n");
            return NULL;
        }
    } while (c != 'y' && c != 'Y');
    printf("\n");
    fseek(stdin, 0, SEEK_END);

    // Missing this was cause of the Invalid Command Error, O_o
    di->drive = drive_num;
    
    printf("Using disk %d\n", di->drive);
    
    return di;
}

int read_disk(struct diskinfo_t *dparams)
{
    if (dparams == NULL) {
        fprintf(stderr, "read_disk() was called with null-pointer to dparams.\n");
        return 1;
    }

    uint8_t *track_buffer = (uint8_t*) calloc(SECTOR_SIZE * (dparams->sector + 1), sizeof(uint8_t));
    if (track_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for track buffer. Try removing TSRs.\n");
        return 1;
    }

    for (uint16_t t = 0; t < dparams->track; t++) {
    //for (uint16_t t = 0; t < 40; t++) {
        //for (uint8_t h = 0; h < 4; h++) {
        for (uint8_t h = 0; h < dparams->head; h++) {
            if (DISKREAD::get_track(track_buffer, dparams->drive, h, t) != 0) {
                free(track_buffer);
                track_buffer = NULL;
                return 1;
            }
        }
    }

    free(track_buffer);
    track_buffer = NULL;
    
    return 0;
}

int save_error_map(void)
{
    if (DISKREAD::get_disk_error_count() == 0) {
        printf("No errors were found! Won't save map file. Exiting.\n");
        return 0;
    }
    
    printf("Errors were found while imaging the disk. Saving error map to file.\n");
    //_bios_keybrd(_NKEYBRD_READ);
    
    return DISKREAD::save_err_map(file_path);
}

