/**
 * NDIPFLPY.CPP - NDIP to Floppy saver
 * 2018 by @marssaturn91
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "NDIPTYPE.H"
#include "DISKREAD.H"

struct diskinfo_t* get_disk(void);
int get_mbr(int disk_num);
//int save_mbr(uint8_t *mbr_data);
int save_mbr(unsigned char *mbr_data);
//int read_disk(struct diskinfo_t *dparams, char *filename, char *file_path, int sectors_per_chunk);
int read_disk(struct diskinfo_t *dparams, int sectors_per_chunk);
int save_error_map(void);

int num_disk_errors = 0;
char file_path[256] = {0};
char file_name[12] = {0};


int main(int argc, char *argv[]) 
{
    struct diskinfo_t *dparams;
    int ret_code;
    uint16_t chunk_size = 0;
    //unsigned long chunk_size = 0;
    uint32_t sectors_per_chunk = 0;
    //unsigned int sectors_per_chunk = 0;

    printf("NDIPFLPY - NDIP (Network Disk Imaging Protocol) to Floppy\n");
    printf(" - Saves Disk Image to multiple file chunks on other disks\n");
    printf("2018 by @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    dparams = get_disk();
    //struct diskinfo_t dparams = DISKREAD::query_disks(0);    
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

        //if (strlen(file_path) == 0) {
        if (strcmp(file_path, "\n") == 0) {
            printf("You need to enter a file path. Try again.\n");
            printf(">");
        }
    //} while (strlen(file_path) == 0 || file_path[1] != ':');
    } while (strcmp(file_path, "\n") == 0 || file_path[1] != ':');

    strtok(file_path, "\n");
    
    if (file_path[2] == '\0') {
        file_path[2] = '\\';
    }

    printf("Enter the first eight letters of the image file. Extension will be IMG\n");
    printf("on drives C or above, or XXX starting at 000, 001, etc. on A or B.\n");
    printf(">");

    do {
        if (fgets(file_name, 9, stdin) == NULL) {
            fprintf(stderr, "Error processing filename.\n");
            free(dparams);
            dparams = NULL;
            return 1;
        }
        

        if (strchr(file_name, '.') > 0) {
            printf("You can't include the extension in the file. Try again.\n");
            printf(">");
        }

        //if (strlen(file_name) == 0) {
        if (strcmp(file_name, "\n") == 0) {
            printf("You need to enter a filename. Try again.\n");
            printf(">");
        }
    //} while (strlen(file_name) == 0 || strchr(file_name, '.' > 0));
    } while (strcmp(file_name, "\n") == 0 || strchr(file_name, '.') > 0);

    strtok(file_name, "\n");
    fseek(stdin, 0, SEEK_END);
    
    if (file_path[0] == 'a' || file_path[0] == 'A' || file_path[0] == 'b' || file_path[0] == 'B') {
        do {
            printf("Disk Sector Size is %d bytes. Please enter the size of the chunks in kilobytes.>", SECTOR_SIZE);
            scanf("%d", &chunk_size);

        } while (chunk_size <= 0);

        printf("Entered %d\n", chunk_size);
        
        uint32_t chunk_size_bytes = chunk_size;
        
        //sectors_per_chunk = (chunk_size * 1024) / SECTOR_SIZE;
        sectors_per_chunk = chunk_size;
        printf("%lu\n", chunk_size_bytes);
        chunk_size_bytes *= 1024;
        //chunk_size_bytes = chunk_size * 1024;
        printf("%lu\n", chunk_size_bytes);
        sectors_per_chunk = chunk_size_bytes / SECTOR_SIZE;

        printf("%lu bytes chunk size with %d byte sized sectors mean %d sectors per chunk.\n", chunk_size_bytes, SECTOR_SIZE, sectors_per_chunk);
    }
    
    // TODO: Verify the chunk size will fit on the disk
    
    if (get_mbr(dparams->drive) > 1) {
    //if (get_mbr(0) > 1) {
        free(dparams);
        dparams = NULL;
        return 1;
    } 
    
    
    if (read_disk(dparams, sectors_per_chunk) > 1) {
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
    fseek(stdin, 0, SEEK_END);

    // Missing this was cause of the Invalid Command Error, O_o
    di->drive = drive_num;
    
    printf("Using disk %d\n", di->drive);
    
    return di;
}


int get_mbr(int disk_num)
{
    int ret_val;
    uint8_t *mbr_data = (uint8_t *) malloc(SECTOR_SIZE);
    //unsigned char *mbr_data = (unsigned char*) malloc(SECTOR_SIZE);
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
    /*
    if (save_mbr(mbr_data) != 0) {
        free(mbr_data);
        mbr_data = NULL;
        return 1;
    }
    */

    free(mbr_data);
    mbr_data = NULL;
    return 0;
}

int save_mbr(uint8_t *mbr_data)
//int save_mbr(unsigned char *mbr_data)
{
    char full_filename[12] = {0};
    FILE *mbr_ptr;
    if (mbr_data == NULL) {
        fprintf(stderr, "save_mbr() was called with a null-pointer passed into *mbr_data.\n");
        return 1;
    }

    //strcpy(full_filename, file_name, strlen(file_name));
    strcpy(full_filename, file_name);
    strcat(full_filename, ".MBR");

    mbr_ptr = fopen(full_filename, "wb");
    if (mbr_ptr == NULL) {
        fprintf(stderr, "Couldn't open %s for writing.\n", full_filename);
        return 1;
    }

    if (fwrite(mbr_data, sizeof(unsigned char), SECTOR_SIZE, mbr_ptr) < SECTOR_SIZE) {
        fprintf(stderr, "Couldn't write all of the data to file %s.\n", full_filename);
        fclose(mbr_ptr);
        return 1;
    }

    fclose(mbr_ptr);

    return 0;
}

int read_disk(struct diskinfo_t *dparams, int sectors_per_chunk)
{
    if (dparams == NULL) {
        fprintf(stderr, "read_disk() was called with null-pointer in dparams.\n");
        return 1;
    }

    if (strlen(file_name) == 0) {
        fprintf(stderr, "read_disk() was called with no string passed in for file_name.\n");
        return 1;
    }

    if (strlen(file_path) == 0) {
        fprintf(stderr, "read_disk() was called with no string passed in for file_path.\n");
        return 1;
    }

    uint8_t *track_buffer = (uint8_t*) calloc(SECTOR_SIZE * (dparams->sector + 1), sizeof(uint8_t));
    //unsigned char *track_buffer = (unsigned char *) malloc(SECTOR_SIZE * (dparams->sector + 1));
    if (track_buffer == NULL) {
        fprintf(stderr, "Couldn't allocate enough memory for track buffer. Try removing TSRs.\n");
        return 1;
    }

    char full_filename[270] = {0};

    int sector_count_in_chunk = 0;
    int chunk_count = 0;
    int ext_loc = 0;
    FILE *image_file_ptr = NULL;

    strcpy(full_filename, file_path);
    if (full_filename[strlen(file_path)-1] != '\\') {
        full_filename[strlen(file_path)] = '\\';
    }
    strcat(full_filename, file_name);
    ext_loc = strlen(full_filename) + 1;

    if (sectors_per_chunk > 0) {
        strcat(full_filename, ".000");
    } else {
        strcat(full_filename, ".IMG");
    }

    printf("Opening %s\n", full_filename);
    
    // Test just track 0 at first
    //int t = 0;
    for (uint16_t t = 0; t < dparams->track; t++) {
    //for (int t = 0; t < 1; t++) {
        for (uint8_t h = 0; h < dparams->head; h++) {
        //for (int h = 0; h < 5; h++) {
            if (DISKREAD::get_track(track_buffer, dparams->drive, h, t) != 0) {
                free(track_buffer);
                track_buffer = NULL;
                return 1;
            }

            image_file_ptr = fopen(full_filename, "ab");
            if (image_file_ptr == NULL) {
                fprintf(stderr, "\aCould not open image file for writing. Make sure you have the right disk!\n");
                //TODO: Let user try again
                free(track_buffer);
                track_buffer = NULL;
                return 1;
            }

            if (fwrite(track_buffer, sizeof(uint8_t), SECTOR_SIZE * dparams->sector, image_file_ptr) < SECTOR_SIZE * dparams->sector) {
            //if (fwrite(track_buffer, sizeof(unsigned char), SECTOR_SIZE * dparams->sector, image_file_ptr) < SECTOR_SIZE * dparams->sector) {
                fprintf(stderr, "\aProblem occurred writing to file. Make sure you have the right disk!\n");
                //TODO: Let user try again
                free(track_buffer);
                track_buffer = NULL;
                return 1;
            }

            fclose(image_file_ptr);
            
            if (sectors_per_chunk > 0) {
                sector_count_in_chunk += dparams->sector;

                if (sector_count_in_chunk >= sectors_per_chunk) {
                    fseek(stdin, 0, SEEK_END);
                    chunk_count++;
                    sector_count_in_chunk = 0;

                    sprintf(full_filename + ext_loc, "%03d", chunk_count);

                    printf("\aChunk file on disk now full. Please replace disk and press any key to continue.\n");
                    //_bios_keybrd(_NKEYBRD_READ);
                    _bios_keybrd(_KEYBRD_READ);
                    
                    printf("Opening %s\n", full_filename);
                }
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
        return 0;
    }
    
    char full_filename[270] = {0};
    strcpy(full_filename, file_path);
    if (full_filename[strlen(file_path)-1] != '\\') {
        full_filename[strlen(file_path)] = '\\';
    }
    strcat(full_filename, file_name);
    
    printf("Errors were found while imaging the disk. A summary of these errors will be\n");
    printf("saved as a textfile in the specified path. Prepare disk and press any key\n");
    printf("once ready.\n");
    //_bios_keybrd(_NKEYBRD_READ);
    _bios_keybrd(_KEYBRD_READ);
    
    return DISKREAD::save_err_map(full_filename);
}


    