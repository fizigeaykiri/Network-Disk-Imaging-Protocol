/**
 * NDIPFLPY.CPP - NDIP to Floppy saver
 * 2018 by @marssaturn91
 */

#include <stdio.h>
#include <string.h>

#include "NDIPTYPE.H"
#include "DISKREAD.H"

char file_path[256] = {0};
char file_name[12] = {0};

int main(int argc, char *argv[]) 
{
    struct diskinfo_t *dparams;
    int ret_code;
    uint32_t chunk_size;
    uint sectors_per_chunk;

    printf("NDIPFLPY - NDIP (Network Disk Imaging Protocol) to Floppy\n");
    printf(" - Saves Disk Image to multiple file chunks on other disks\n");
    printf("2018 by @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    dparams = get_disk();
    if (dparams == NULL) {
        return 1;
    }

    printf("Enter drive and path for the new disk image file.\n");
    printf("Note: If you specify a drive other than A or B, it must store the entire image.\n");
    printf(">");

    do {
        if (fgets(file_path, 256, stdin) == NULL) {
            fprintf(stderr, "Error processing filepath.\n");
            free(dparams);
            dparams = NULL;
            return 1;
        }

        if (file_path[1] != ':') {
            printf("Invalid path. You must include the drive letter. Try again.\n");
            printf(">");
        }

        if (strlen(file_path) == 0) {
            printf("You need to enter a file path. Try again.\n");
            printf(">");
        }
    } while (strlen(file_path) == 0 || file_path[1] != ':');

    if (file_path[2] == '\0') {
        file_path[2] = '\\';
    }

    printf("Enter the first eight letters of the image file. Extension will be IMG\n");
    printf("on drives C or above, or XXX starting at 000, 001, etc. on A or B.\n");
    printf(">");

    do {
        if (fgets(file_name, 8, stdin) == NULL) {
            fprintf(stderr, "Error processing filename.\n");
            free(dparams);
            dparams = NULL;
            return 1;
        }

        if (strchr(file_name, '.') > 0) {
            printf("You can't include the extension in the file. Try again.\n");
            printf(">");
        }

        if (strlen(file_name) == 0) {
            printf("You need to enter a filename. Try again.\n");
            printf(">");
        }
    } while (strlen(file_name) == 0 || strchr(file_name, '.' > 0));

    if (file_path[0] == 'a' || file_path[0] == 'A' || file_path[0] == 'b' || file_path[0] == 'B') {
        do {
            printf("Disk Sector Size is %d bytes. Please enter the size of the chunks in kilobytes.\n", SECTOR_SIZE);
            fscanf(stdin, ">%l\n", chunk_size);

        } while (chunk_size <= 0);

        sectors_per_chunk = (chunk_size * 1024) / SECTOR_SIZE;

        printf("%l bytes chunk size with %d byte sized sectors mean %d sectors per chunk.\n", chunk_size * 1024, SECTOR_SIZE, sectors_per_chunk);
    }
    // TODO: Verify the chunk size will fit on the disk

    

    return 0;
}

struct diskinfo_t* get_disk(void)
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
    printf("image. (1 through %d): ", di->drive);

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

    return di;
}

int get_mbr(int disk_num)
{
    int ret_val;
    uint8_t *mbr_data = (uint8_t *) malloc(SECTOR_SIZE);
    if (mbr_data == NULL) {
        fprintf(stderr, "Couldn't allocate memory to store MBR data. Try freeing up TSRs.\n");
        return 1;
    }

    memset(mbr_data, 0, SECTOR_SIZE);

    printf("get_mbr()\n");

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

    // Send to File
    if (save_mbr(mbr_data) != 0) {
        free(mbr_data);
        mbr_data = NULL;
        return 1;
    }

    free(mbr_data);
    mbr_data = NULL;
    return 0;
}

int save_mbr(uint8_t *mbr_data)
{
    char full_filename[12] = {0};
    FILE *mbr_ptr;
    if (mbr_data == NULL) {
        fprintf(stderr, "save_mbr() was called with a null-pointer passed into *mbr_data.\n");
        return 1;
    }

    strcpy(full_filename, file_name, strlen(file_name));
    strcat(full_filename, ".MBR");

    mbr_ptr = fopen(full_filename, "wb");
    if (mbr_ptr == NULL) {
        fprintf(stderr, "Couldn't open %s for writing.\n", full_filename);
        return 1;
    }

    if (fwrite(mbr_data, SECTOR_SIZE, 1, mbr_ptr) < SECTOR_SIZE) {
        fprintf(stderr, "Couldn't write all of the data to file %s.\n", full_filename);
        fclose(mbr_ptr);
        return 1;
    }

    fclose(mbr_ptr);

    return 0;
}