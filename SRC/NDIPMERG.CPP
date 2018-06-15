/**
 * NDIPMERG - NDIP Merge
 *  - Merges the split files from NDIPFLPY into a single image file
 * 2018 By @marssaturn91
 */

#include <stdio.h>
#include <string.h>
#include <bios.h>

#define CHUNK_BUFF_SIZE     8192

char src_file_path[256] = {0};
char src_file_name[12] = {0};
char dest_file_name[270] = {0};

unsigned char chunk_buffer[CHUNK_BUFF_SIZE] = {0};

int read_chunk(unsigned char *chunk_buff, unsigned long chunk_offset, char *file_name, unsigned int chunk_num);
int write_full_image(unsigned char *chunk_buff, unsigned int chunk_buff_len, char *file_name);

int main(int argc, char *argv[])
{

    printf("NDIPMERG - NDIP (Network Disk Imaging Protocl) Merge from Floppy\n");
    printf(" - Combines the saved disk image chunks into a single usable disk image\n");
    printf("2018 By @marssaturn91\n");
    printf("https://github.com/marssaturn91/Network-Disk-Imaging-Protocol\n\n");

    printf("Enter drive and path for the disk image chunks.\n");
    printf("Note: Only Drive A or B will prompt you for disk changes.\n");
    printf(">");

    do {
        if (fgets(src_file_path, 256, stdin) == NULL) {
            fprintf(stderr, "Error processing filepath.\n");
            return 1;
        }

        if (src_file_path[1] != ':') {
            printf("Invalid path. You must include the drive letter. Try again.\n");
            printf(">");
        }

        if (strcmp(src_file_path, "\n") == 0) {
            printf("You need to enter a file path. Try again.\n");
            printf(">");
        }
    } while (strcmp(src_file_path, "\n") == 0 || src_file_path[1] != ':');

    strtok(src_file_path, "\n");

    if (src_file_path[2] == '\0') {
        src_file_path[2] = '\\';
    }

    printf("Enter the first eight letters of the image file chunks. Extension will be\n");
    printf("000, 001, 002, etc.\n");
    printf(">");

    do {
        if (fgets(src_file_name, 9, stdin) == NULL) {
            fprintf(stderr, "Error processing filename.\n");
            return 1;
        }

        if (strchr(src_file_name, '.') != NULL) {
            printf("You can't include the extension in the file. Try again.\n");
            printf(">");
        }

        if (strcmp(src_file_name, "\n") == 0) {
            printf("You need to enter a filename. Try again.\n");
            printf(">");
        }
    } while (strcmp(src_file_name, "\n") == 0 || strchr(src_file_name, '.') != NULL);

    strtok(src_file_name, "\n");
    fseek(stdin, 0, SEEK_END);

    printf("Enter the full path and filename of the newly merged disk image.\n");
    printf("Make sure your destination will have enough space for the entire image.\n");
    printf("If you don't specify an extension, .IMG will be added to the end.\n");
    printf(">");

    do {
        if (fgets(dest_file_name, 256, stdin) == NULL) {
            fprintf(stderr, "Error processing filename.\n");
            return 1;
        }

        if (strcmp(dest_file_name, "\n") == 0) {
            printf("You need to enter a filename. Try again.\n");
            printf(">");
        }
    } while (strcmp(dest_file_name, "\n") == 0);

    strtok(dest_file_name, "\n");
    if (strchr(dest_file_name, '.') == NULL) {
        strcat(dest_file_name, ".IMG");
    }
    fseek(stdin, 0, SEEK_END);

    char src_full_filename[270] = {0};
    strcpy(src_full_filename, src_file_path);
    if (src_full_filename[strlen(src_file_path)-1] != '\\') {
        src_full_filename[strlen(src_file_path)] = '\\';
    }
    strcat(src_full_filename, src_file_name);
    
    unsigned int chunk_num = 0;
    unsigned long chunk_offset = 0;
    printf("Reading Chunk %d\n", chunk_num);

    while (true) {
        int status = read_chunk(chunk_buffer, chunk_offset, src_full_filename, chunk_num);
        if (status == -2) {
            printf("Disk Image merger complete.\n");
            return 0;
        } else if (status == -1) {
            return 1;
        } else if (status == 0) {
            chunk_num++;
            chunk_offset = 0;
            if (src_file_path[0] == 'a' || src_file_path[0] == 'A' || src_file_path[0] == 'b' || src_file_path[0] == 'B') {
                printf("Please insert the disk with Chunk %d and press any key to continue.\n");
                _bios_keybrd(_NKEYBRD_READ);
            }
            printf("Reading Chunk %d\n", chunk_num);
            continue;
        } else if (status > 0) {
            chunk_offset += status;
        }

        if (write_full_image(chunk_buffer, status, dest_file_name) != 0) {
            return 1;
        }
    }

    return 0;
}

int read_chunk(unsigned char *chunk_buff, unsigned long chunk_offset, char *file_name, unsigned int chunk_num)
{
    if (chunk_buff == NULL) {
        fprintf(stderr, "read_chunk() called with null-pointer to chunk_buff.\n");
        return -1;
    }
    if (file_name == NULL || strlen(file_name) == 0) {
        fprintf(stderr, "read_chunk() called without specifying file_name.\n");
        return -1;
    }

    char full_filename[270] = {0};
    int ext_loc;

    strcpy(full_filename, file_name);
    ext_loc = strlen(full_filename);

    sprintf(full_filename + ext_loc, ".%03d", chunk_num);

    FILE *chunk_file_ptr = NULL;

    chunk_file_ptr = fopen(full_filename, "rb");
    if (chunk_file_ptr == NULL) {
        fprintf(stderr, "End of chunk files or couldn't open chunk file for reading.\n");
        fprintf(stderr, "If there are more, make sure you have the right disk.\n");
        fprintf(stderr, "%s\n", full_filename);
        return -2;
    }

    if (fseek(chunk_file_ptr, chunk_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Couldn't get to offset 0x%08X in file.\n", chunk_offset);
        fclose(chunk_file_ptr);
        chunk_file_ptr = NULL;
        return -1;
    }

    if (feof(chunk_file_ptr)) {
        fclose(chunk_file_ptr);
        chunk_file_ptr = NULL;
        return 0;
    }

    int ret_val = fread(chunk_buff, sizeof(unsigned char), CHUNK_BUFF_SIZE, chunk_file_ptr);

    fclose(chunk_file_ptr);
    chunk_file_ptr = NULL;
    return ret_val;
}

int write_full_image(unsigned char *chunk_buff, unsigned int chunk_buff_len, char *file_name)
{
    if (chunk_buff == NULL) {
        fprintf(stderr, "write_full_image() called with null-pointer to chunk_buff.\n");
        return -1;
    }
    if (file_name == NULL || strlen(file_name) == 0) {
        fprintf(stderr, "write_full_image() called with null-pointer to file_name.\n");
        return -1;
    }

    FILE *full_image_ptr = NULL;

    full_image_ptr = fopen(file_name, "ab");
    if (full_image_ptr == NULL) {
        fprintf(stderr, "Couldn't open full image file for writing. Make sure your destination has\n");
        fprintf(stderr, "enough space: %s\n", file_name);
        return -1;
    }

    if (fwrite(chunk_buff, sizeof(unsigned char), chunk_buff_len, full_image_ptr) < chunk_buff_len) {
        fprintf(stderr, "Couldn't write all of the data to image file. Make sure your destination has\n");
        fprintf(stderr, "enough space.\n");
        fclose(full_image_ptr);
        full_image_ptr = NULL;
        return -1;
    }

    fclose(full_image_ptr);
    return 0;
}