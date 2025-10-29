#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"  // Assuming common types like Status and OperationType are defined here

int main(int argc, char *argv[])
{
    // Step 1: Create a variable ret to store the operation type
    OperationType ret = check_operation_type(argc, argv);

    // Step 2: Declare EncodeInfo and DecodeInfo variables
    EncodeInfo encInfo;
    DecodeInfo decInfo;

    // Step 3: Check if operation is encoding
    if (ret == e_encode)
    {
        printf("Encoding operation selected.\n");

        // Step 4: Validate encoding arguments
        Status val_ret = read_and_validate_encode_args(argc, argv, &encInfo);
        if (val_ret == e_success)
        {
            // Step 5: Perform encoding
            Status ret_enc = do_encoding(&encInfo);
            if (ret_enc == e_success)
            {
                printf("Encoding is done successfully!\n");
            }
            else
            {
                printf("ERROR: Encoding failed.\n");
            }
        }
        else
        {
            // If validation fails
            printf("ERROR: Validation of encoding arguments failed.\n");
        }
    }
    // Step 6: Check if operation is decoding
    else if (ret == e_decode)
    {
        printf("Decoding operation selected.\n");

        // Step 7: Validate decoding arguments
        Status val_ret = read_and_validate_decode_args(argc, argv, &decInfo);
        if (val_ret == e_success)
        {
            // Step 8: Perform decoding
            Status ret_dec = do_decoding(&decInfo);
            if (ret_dec == e_success)
            {
                printf("Decoding is done successfully!\n");
            }
            else
            {
                printf("ERROR: Decoding failed.\n");
            }
        }
        else
        {
            // If validation fails
            printf("ERROR: Validation of decoding arguments failed.\n");
        }
    }
    // Step 9: Handle invalid operation type
    else
    {
        printf("ERROR: Invalid operation type. Please choose either encode or decode.\n");
    }

    return 0;
}

OperationType check_operation_type(int argc, char *argv[])
{
    // Step 1: Check if the number of arguments is greater than 1 (must have at least one argument besides the program name)
    if (argc > 1)
    {
        // Step 2: Check if the operation is encoding ("-e")
        if (strcmp(argv[1], "-e") == 0)
        {
            return e_encode;
        }
        // Step 3: Check if the operation is decoding ("-d")
        else if (strcmp(argv[1], "-d") == 0)
        {
            return e_decode;
        }
        // Step 4: If neither, return unsupported
        else
        {
            return e_unsupported;
        }
    }
    else
    {
        // Step 5: If there are not enough arguments, return unsupported
        printf("ERROR: No operation type provided. Use -e for encoding or -d for decoding.\n");
        return e_unsupported;
    }
}



