#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

/* Structure to store decoding information */
typedef struct _DecodeInfo
{
    /* Stego Image Info */
    char *stego_image_fname;
    FILE *fptr_stego_image;

    /* Output File Info */
    char *output_fname;       // Name with or without extension
    FILE *fptr_output;

    /* Secret File Info */
    char extn_secret_file[10];  // Extension of secret file
    long size_secret_file;                   // Size of secret file

    /* Additional Fields for Decoding */
    char magic_str[10];    // For magic string
    int extn_size;        // For extension size
    char file_extn[10];   // To store the decoded file extension

} DecodeInfo;

/* Function Prototypes */
Status read_and_validate_decode_args(int argc, char *argv[], DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);
Status decode_magic_string(DecodeInfo *decInfo);
Status prompt_and_compare_magic_string(DecodeInfo *decInfo);  // New function
Status decode_secret_file_extn_size(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);
Status decode_byte_from_lsb(char *data, char *image_buffer);
Status decode_size_from_lsb(int *data, char *image_buffer);
Status open_output_file(DecodeInfo *decInfo);

#endif
