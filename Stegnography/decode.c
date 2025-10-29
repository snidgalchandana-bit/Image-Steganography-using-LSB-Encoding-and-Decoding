#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

Status read_and_validate_decode_args(int argc, char *argv[], DecodeInfo *decInfo)
{
    // Step 1: Validate argument count
    if (argc > 4)
    {
        printf("ERROR: Invalid number of arguments. Usage: <Program Name> -d <Stego Image> <Base Output Name>\n");
        return e_failure;
    }

    // Step 2: Check if stego image is a BMP file
    if (!strstr(argv[2], ".bmp"))
    {
        printf("ERROR: Stego image must be a BMP file.\n");
        return e_failure;
    }

    // Store the stego image file name
    decInfo->stego_image_fname = argv[2];

    // Step 3: Handle the output file name
    if (argv[3] == NULL)
    {
        // If output name is null, use "output" as the base name
        printf("No output file name provided, using default name: output\n");
        decInfo->output_fname = "output";
    }
    else if (strstr(argv[3], ".")) // Check if the provided name already has an extension
    {
        decInfo->output_fname = argv[3];  // Store as is
    }
    else
    {
        // If no extension is provided, store the base name and append the decoded extension later
        decInfo->output_fname = argv[3];
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    // Step 1: Open the stego image file
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    // Step 2: Decode the magic string and validate by prompting the user
    if (prompt_and_compare_magic_string(decInfo) == e_failure)
    {
        printf("ERROR: Magic string mismatch. Decoding aborted.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        return e_failure;
    }

    // Step 3: Decode the secret file extension size
    if (decode_secret_file_extn_size(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file extension size.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        return e_failure;
    }

    // Step 4: Decode the secret file extension
    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file extension.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        return e_failure;
    }

    // Step 5: Open the output file using concatenated base name and extension
    if (open_output_file(decInfo) == e_failure)
    {
        printf("ERROR: Failed to open output file.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        return e_failure;
    }

    // Step 6: Decode the secret file size
    if (decode_secret_file_size(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file size.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        fclose(decInfo->fptr_output); // Close the output file
        return e_failure;
    }

    // Step 7: Decode the secret file data
    if (decode_secret_file_data(decInfo) == e_failure)
    {
        printf("ERROR: Failed to decode secret file data.\n");
        fclose(decInfo->fptr_stego_image); // Close the stego image file
        fclose(decInfo->fptr_output); // Close the output file
        return e_failure;
    }

    // Close both files after successful decoding
    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_output);

    printf("Decoding successful. Secret file extracted to %s\n", decInfo->output_fname);
    return e_success;
}

Status prompt_and_compare_magic_string(DecodeInfo *decInfo)
{
    char user_magic_string[10]; // Buffer for user input (adjust size if needed)
    const char *magic_string = MAGIC_STRING;
    char image_buffer[8];
    char decoded_char;

    // Prompt the user to input the magic string
    printf("Enter the magic string to compare: ");
    scanf("%9s", user_magic_string);  // Limiting input to avoid buffer overflow

    // Skip the BMP header
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    // Step 1: Loop through each character in the magic string
    for (int i = 0; magic_string[i] != '\0'; i++)
    {
        // Read 8 bytes from the stego image
        if (fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to read 8 bytes from stego image.\n");
            return e_failure;
        }

        // Decode the character from LSBs
        if (decode_byte_from_lsb(&decoded_char, image_buffer) != e_success)
        {
            printf("ERROR: Failed to decode byte from LSB.\n");
            return e_failure;
        }

        // Compare the decoded character with the user-entered magic string
        if (decoded_char != user_magic_string[i])
        {
            printf("ERROR: Magic string mismatch at character %d.\n", i + 1);
            return e_failure;
        }
    }

    printf("Magic string successfully decoded and matched.\n");
    return e_success;
}

Status open_output_file(DecodeInfo *decInfo)
{
    char output_filename[100];  // Adjust size as needed, ensure it's large enough for the full file name

    // Step 1: Copy the base output file name
    strcpy(output_filename, decInfo->output_fname);

    // Step 2: Check if the base name already has an extension
    if (!strchr(decInfo->output_fname, '.'))
    {
        // If no extension, concatenate the decoded extension
        strcat(output_filename, decInfo->file_extn);  // Append the extension
    }

    // Step 3: Open the output file for writing the decoded data
    decInfo->fptr_output = fopen(output_filename, "w");
    if (decInfo->fptr_output == NULL)
    {
        perror("fopen");
        printf("ERROR: Unable to open file %s\n", output_filename);
        return e_failure;
    }

    // Step 4: Store the full output file name back in the structure
    strcpy(decInfo->output_fname, output_filename);  // Copy the final name back to decInfo->output_fname

    return e_success;
}


Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    char image_buffer[32];
    int extn_size;

    if (fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to read 32 bytes from stego image.\n");
        return e_failure;
    }

    if (decode_size_from_lsb(&extn_size, image_buffer) != e_success)
    {
        printf("ERROR: Failed to decode secret file extension size.\n");
        return e_failure;
    }

    decInfo->extn_size = extn_size;
    return e_success;
}

Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    char image_buffer[8];
    char decoded_char;

    for (int i = 0; i < decInfo->extn_size; i++)
    {
        if (fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to read 8 bytes from stego image.\n");
            return e_failure;
        }

        if (decode_byte_from_lsb(&decoded_char, image_buffer) != e_success)
        {
            printf("ERROR: Failed to decode byte from LSB.\n");
            return e_failure;
        }

        decInfo->file_extn[i] = decoded_char;
    }

    decInfo->file_extn[decInfo->extn_size] = '\0';  // Null-terminate the extension
    return e_success;
}

Status decode_secret_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];
    int file_size;

    if (fread(image_buffer, sizeof(char), 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to read 32 bytes from stego image.\n");
        return e_failure;
    }

    if (decode_size_from_lsb(&file_size, image_buffer) != e_success)
    {
        printf("ERROR: Failed to decode secret file size.\n");
        return e_failure;
    }

    decInfo->size_secret_file = file_size;
    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char image_buffer[8];
    char decoded_byte;

    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(image_buffer, sizeof(char), 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to read 8 bytes from stego image.\n");
            return e_failure;
        }

        if (decode_byte_from_lsb(&decoded_byte, image_buffer) != e_success)
        {
            printf("ERROR: Failed to decode byte from LSB.\n");
            return e_failure;
        }

        fputc(decoded_byte, decInfo->fptr_output);
    }

    return e_success;
}

Status decode_byte_from_lsb(char *data, char *image_buffer)
{
    *data = 0;
    for (int i = 0; i < 8; i++)
    {
        *data |= ((image_buffer[i] & 0x01) << (7 - i));
    }
    return e_success;
}

Status decode_size_from_lsb(int *data, char *image_buffer)
{
    *data = 0;
    for (int i = 0; i < 32; i++)
    {
        *data |= ((image_buffer[i] & 0x01) << (31 - i));
    }
    return e_success;
}
