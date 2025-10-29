#include <stdio.h>
#include<string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18, 
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);//from 18th byte of an header file having width and heigth of the file

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}

Status read_and_validate_encode_args(int argc, char *argv[], EncodeInfo *encInfo)
{
    // Step 1: Check if the total number of arguments is NOT 5
    if (argc >= 6)
    {
        // Invalid number of arguments
        printf("ERROR: Invalid number of arguments. Usage: <Program Name> -e <Source Image> <Secret File> <Stego Image>\n");
        return e_failure;
    }

    // Step 2: Check if the source image file is not a BMP file
    if (!strstr(argv[2], ".bmp"))
    { 
        printf("ERROR: Source image must be a BMP file.\n");
        return e_failure;
    }
    // Store source image filename
    encInfo->src_image_fname = argv[2];

    // Step 3: Check if the secret file is not a file
    if (!strstr(argv[3], "."))
    {
        printf("ERROR: Secret file must be a file.\n");
        return e_failure;
    }
    // Store secret file filename
    encInfo->secret_fname = argv[3];

    // Step 4: Handle stego image filename
    if (argv[4] == NULL || strlen(argv[4]) == 0)
    {
        // If argv[4] is NULL or empty, assign default filename "stego.bmp"
        strcpy(encInfo->stego_image_fname, "stego.bmp");
        printf("INFO: No stego image filename provided, using default: stego.bmp\n");
    }
    else if (!strstr(argv[4], ".bmp"))
    {
        // Check if the stego image file is not a BMP file
        printf("ERROR: Stego image must be a BMP file.\n");
        return e_failure;
    }
    else
    {
        // Store the provided stego image filename
        strcpy(encInfo->stego_image_fname, argv[4]);
    }

    // All checks passed
    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    // Step 1: Open the files (source image, secret file, stego image)
    Status open_status = open_files(encInfo);
    if (open_status == e_failure)
    {
        // If opening files failed, return failure
        printf("ERROR: Unable to open the required files.\n");
        return e_failure;
    }

    // Step 2: Check if the source image has enough capacity to store the secret
    Status capacity_status = check_capacity(encInfo);
    if (capacity_status == e_failure)
    {
        // If capacity is insufficient, return failure
        printf("ERROR: Source image does not have enough capacity to hold the secret data.\n");
        return e_failure;
    }

    // Step 3: Copy the BMP header from the source image to the stego image
    Status copy_bmp_status = copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    if (copy_bmp_status == e_failure)
    {
        // If the BMP header is not copied
        printf("ERROR: Failed to copy the BMP header to stego image.\n");
        return e_failure;
    }

    // Step 4: Encode the magic string
    Status magic_string_status = encode_magic_string("#*", encInfo);
    if (magic_string_status == e_failure)
    {
        // If the magic string is not copied to the stego image
        printf("ERROR: Failed to encode the magic string to the stego image.\n");
        return e_failure;
    }

    // Step 5: Encode the secret file extension size
    Status extn_size_status = encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo);
    if (extn_size_status == e_failure)
    {
        // If the extension size is not encoded properly
        printf("ERROR: Failed to encode the secret file extension size.\n");
        return e_failure;
    }

    // Step 6: Encode the secret file extension
    Status extn_data_status = encode_secret_file_extn(encInfo->extn_secret_file, encInfo);
    if (extn_data_status == e_failure)
    {
        // If the extension data is not encoded properly
        printf("ERROR: Failed to encode the secret file extension.\n");
        return e_failure;
    }

    // Step 7: Encode the size of the secret file
    Status secret_size_status = encode_secret_file_size(get_file_size(encInfo->fptr_secret), encInfo);
    if (secret_size_status == e_failure)
    {
        // If the secret file size is not encoded properly
        printf("ERROR: Failed to encode the secret file size.\n");
        return e_failure;
    }

    // Step 8: Encode the secret file data
    Status secret_data_status = encode_secret_file_data(encInfo);
    if (secret_data_status == e_failure)
    {
        // If the secret file data is not encoded properly
        printf("ERROR: Failed to encode the secret file data.\n");
        return e_failure;
    }

    // Step 9: Copy remaining data from source image to stego image
    Status remaining_data_status = copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    if (remaining_data_status == e_failure)
    {
        // If the remaining data is not copied properly
        printf("ERROR: Failed to copy the remaining data from source to stego image.\n");
        return e_failure;
    }

    // Return success if all encoding steps are completed successfully
    return e_success;
}

uint get_file_size(FILE *fptr) 
{
    // Step 1: Move the file pointer to the end of the file
    fseek(fptr, 0L, SEEK_END);

    // Step 2: Get the current file pointer position, which is the file size
    uint file_size = ftell(fptr);

    // Step 3: Move the file pointer back to the start of the file
    fseek(fptr, 0L, SEEK_SET);

    // Step 4: Return the file size
    return file_size;
}

Status check_capacity(EncodeInfo *encInfo)
{
    // Step 1: Calculate the estimated size required for encoding
    
    // Get the length of the magic string and multiply by 8 (1 byte = 8 bits)
    int estimated_size = strlen(MAGIC_STRING) * 8; // Magic string size in bits (e.g., "#x")
    printf("Size of magic string (in bits): %d\n", estimated_size);

    // Step 2: Add the size required for the file extension (e.g., ".txt")
    // Extract the file extension from the secret file
    strcpy(encInfo->extn_secret_file, strstr(encInfo->secret_fname, "."));
    int extension_size = (strlen(encInfo->extn_secret_file) * 8) + (sizeof(int) * 8); // File extension size in bits + 4 bytes for length
    estimated_size += extension_size;
    printf("Size of file extension (in bits): %d\n", extension_size);

    // Step 3: Add the size required to store the secret file size (32 bits)
    int secret_file_size_bits = sizeof(encInfo->size_secret_file) * 8; // File size stored in 4 bytes (32 bits)
    estimated_size += secret_file_size_bits;
    printf("Size to store secret file size (in bits): %d\n", secret_file_size_bits);

    // Step 4: Add the size of the secret file data (in bits)
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret); // Get the secret file size in bytes
    int secret_data_size = encInfo->size_secret_file * 8; // Convert file size to bits
    estimated_size += secret_data_size;
    printf("Size of secret file data (in bits): %d\n", secret_data_size);
    printf("Size of estimated size (in bits): %d\n", estimated_size);

    // Step 5: Get the size of the BMP image minus the 54-byte header
    encInfo->image_capacity = (get_image_size_for_bmp(encInfo->fptr_src_image) - 54) * 8; // BMP image capacity excluding header in bits
    printf("Available image capacity (in bits): %u\n", encInfo->image_capacity);

    // Step 6: Compare the estimated size with the image capacity
    if (estimated_size > encInfo->image_capacity)
    {
        printf("ERROR: The source image does not have enough capacity to hold the secret data.\n");
        return e_failure;
    }

    // If the image has enough capacity, return success
    return e_success;
}

Status copy_bmp_header(FILE *src_image, FILE *stego_image)
{
    // Step 1: Move file pointers to the start (position 0) of both files
    fseek(src_image, 0L, SEEK_SET);  // Move to the beginning of the source image
    fseek(stego_image, 0L, SEEK_SET);  // Move to the beginning of the stego image

    // Step 2: Create a buffer to hold the BMP header (54 bytes)
    char header[54];

    // Step 3: Read 54 bytes (BMP header) from the source BMP file
    if (fread(header, 54, 1, src_image) != 1)
    {
        printf("ERROR: Failed to read BMP header from the source image.\n");
        return e_failure;
    }

    // Step 4: Write the BMP header into the stego BMP file
    if (fwrite(header, 54, 1, stego_image) != 1)
    {
        printf("ERROR: Failed to write BMP header to the stego image.\n");
        return e_failure;
    }

    // Step 5: Successful header copy
    return e_success;
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    printf("Encoding byte: '%c' (0x%02X)\n", data, data); // Log the character and its hex value

    // Step 1: Loop over each bit of the byte to encode (8 bits)
    for (int i = 0; i < 8; i++)
    {
        // Step 2: Clear the LSB of the current image buffer byte
        image_buffer[i] &= ~(1 << 0);  // Clear the LSB (set it to 0)

        // Step 3: Get the i-th bit of the data (MSB to LSB) and shift it to LSB position
        char bit_to_encode = (data >> (7 - i)) & 1;  // Extract the (7 - i)-th bit of data

        // Step 4: Set the LSB of the current image buffer byte with the extracted bit
        image_buffer[i] |= (bit_to_encode << 0);  // Set the LSB with the extracted bit

        // Log the bit being encoded
        printf("Bit %d encoded into byte %d of image buffer: %d\n", bit_to_encode, i, image_buffer[i] & 1);
    }

    // Step 5: Return success after encoding the byte
    return e_success;
}


Status encode_size_to_lsb(int data, char *image_buffer) {

    printf("Encoding byte: '%d' (0x%02X)\n", data, data); // Log the character and its hex value

    // Step 1: Loop over each bit of the integer (32 bits)
    for (int i = 0; i < 32; i++) {
        // Step 2: Clear the LSB of the current image buffer byte
        image_buffer[i] &= ~(1 << 0);  // Clear the LSB (set it to 0)

        // Step 3: Extract the i-th bit from the integer data (starting from the MSB)
        int secret_bit = (data >> (31 - i)) & 1;  // Extract the (31 - i)-th bit of data

        // Step 4: Set the LSB of the current image buffer byte with the extracted secret bit
        image_buffer[i] |= (secret_bit << 0);  // Set the LSB with the extracted secret bit
        
        // Log the bit being encoded
        printf("Bit %d encoded into byte %d of image buffer: %d\n",secret_bit, i, image_buffer[i] & 1);
    }

    // Step 5: Return success after encoding the size
    return e_success;
}


Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    char image_buffer[8];

    // Step 1: Loop through each character in the magic string
    for (int i = 0; magic_string[i] != '\0'; i++)
    {
        // Step 2: Read 8 bytes from the source image
        if (fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)
        {
            printf("ERROR: Failed to read 8 bytes from source image during encoding.\n");
            return e_failure;
        }

        // Step 3: Encode the current byte of the magic string into the LSB of the image buffer
        if (encode_byte_to_lsb(magic_string[i], image_buffer) != e_success)
        {
            printf("ERROR: Failed to encode byte to LSB during encoding.\n");
            return e_failure;
        }

        // Step 4: Write the modified 8 bytes to the stego image file
        if (fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to write 8 bytes to stego image during encoding.\n");
            return e_failure;
        }

        // Log the encoded character
        printf("Encoded character '%c' into LSBs.\n", magic_string[i]);
    }

    // Return success after encoding the entire magic string
    return e_success;
}


Status encode_secret_file_extn_size(int file_size, EncodeInfo *encInfo)
{
    // Step 1: Create a buffer to hold 8 bytes of data from the source image
    char image_buffer[32];  // Allocate 32 bytes to hold 32 bits

    // Step 2: Find the file extension from the secret file name
    strcpy(encInfo->extn_secret_file, strstr(encInfo->secret_fname, "."));
    
    // Step 3: Calculate the size of the file extension
    int extn_size = strlen(encInfo->extn_secret_file);

    // Step 4: Read 32 bytes from the source image
    if (fread(image_buffer, sizeof(char), 32, encInfo->fptr_src_image) != 32)
    {
        printf("ERROR: Failed to read 32 bytes from source image.\n");
        return e_failure;
    }

    // Step 5: Encode the file extension size (int) into the 32 least significant bits
    if (encode_size_to_lsb(extn_size, image_buffer) != e_success)
    {
        printf("ERROR: Failed to encode extension size to LSB.\n");
        return e_failure;
    }

    // Step 6: Write the modified 32 bytes back to the stego image
    if (fwrite(image_buffer, sizeof(char), 32, encInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to write 32 bytes to stego image.\n");
        return e_failure;
    }

    // Step 7: Return success after encoding the file extension size
    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo) {
     // Step 1: Create a buffer to hold 8 bytes of data from the source image
     char image_buffer[8];

     // Step 2: Loop through each character in the magic string
     for (int i = 0; file_extn[i] != '\0'; i++)
     {
         // Step 3: Read 8 bytes from the source image
         if (fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)
         {
             printf("ERROR: Failed to read 8 bytes from source image.\n");
             return e_failure;
         }
 
         // Step 4: Encode the current byte of the magic string into the LSB of the image buffer
         if (encode_byte_to_lsb(file_extn[i], image_buffer) != e_success)
         {
             printf("ERROR: Failed to encode byte to LSB.\n");
             return e_failure;
         }
 
         // Step 5: Write the modified 8 bytes to the stego image file
         if (fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)
         {
             printf("ERROR: Failed to write 8 bytes to stego image.\n");
             return e_failure;
         }
     }
 
     // Step 6: Return success after encoding the entire magic string
     return e_success;
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    // Step 1: Create a buffer to hold 32 bytes of data from the source image
    char image_buffer[32];

    // Convert the file size from long to int (typically 4 bytes)
    int file_size_as_int = (int) file_size;  // Assuming file size fits into 32 bits

    // Step 2: Read 32 bytes from the source image (enough for 32 bits encoding)
    if (fread(image_buffer, sizeof(char), 32, encInfo->fptr_src_image) != 32)
    {
        printf("ERROR: Failed to read 32 bytes from source image.\n");
        return e_failure;
    }

    // Step 3: Encode the file size (32 bits) into the least significant bits (LSBs) of the image buffer
    if (encode_size_to_lsb(file_size_as_int, image_buffer) != e_success)
    {
        printf("ERROR: Failed to encode file size to LSB.\n");
        return e_failure;
    }

    // Step 4: Write the modified 32 bytes back to the stego image
    if (fwrite(image_buffer, sizeof(char), 32, encInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Failed to write 32 bytes to stego image.\n");
        return e_failure;
    }

    // Step 5: Return success after encoding the file size
    return e_success;
}


Status encode_secret_file_data(EncodeInfo *encInfo)
{
    // Step 1: Create buffers for reading the image and storing the secret file data
    char image_buffer[8];  // To store 8 bytes of image data at a time
    char secret_byte;      // To store one byte of secret file data

    // Step 2: Get the size of the secret file using the helper function
    uint secret_file_size = get_file_size(encInfo->fptr_secret);  // Get the size of the secret file in bytes

    // Step 3: Loop over each byte of the secret file
    for (uint i = 0; i < secret_file_size; i++)
    {
        // Step 4: Read a byte from the secret file
        if (fread(&secret_byte, sizeof(char), 1, encInfo->fptr_secret) != 1)
        {
            printf("ERROR: Failed to read a byte from secret file.\n");
            return e_failure;
        }

        // Step 5: Read 8 bytes from the source image
        if (fread(image_buffer, sizeof(char), 8, encInfo->fptr_src_image) != 8)
        {
            printf("ERROR: Failed to read 8 bytes from source image.\n");
            return e_failure;
        }

        // Step 6: Encode the secret file byte into the LSBs of the 8 bytes from the image
        if (encode_byte_to_lsb(secret_byte, image_buffer) != e_success)
        {
            printf("ERROR: Failed to encode byte to LSB.\n");
            return e_failure;
        }

        // Step 7: Write the modified 8 bytes to the stego image file
        if (fwrite(image_buffer, sizeof(char), 8, encInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Failed to write 8 bytes to stego image.\n");
            return e_failure;
        }
    }

    // Step 8: Return success after encoding all the secret file data
    return e_success;
}

// Function to copy the remaining data from src_image to stego_image
Status copy_remaining_img_data(FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char buffer[1024];
    size_t bytes_read;

    // Loop to copy data from the source image to the stego image
    while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), fptr_src_image)) > 0)
    {
        if (fwrite(buffer, sizeof(char), bytes_read, fptr_stego_image) != bytes_read)
        {
            printf("ERROR: Failed to write remaining data to stego image.\n");
            return e_failure;
        }
    }

    return e_success;
}
