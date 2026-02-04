#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "aes.h"

// ================= Configuration =================
// Define Flash layout constants
#define APROM_TOTAL_SIZE  65536       // 64 KB (0x10000)
#define SIGNATURE_SIZE    16          // AES Block Size
// Validation region = Total space - Signature space (Fixed at 65520 bytes)
#define DATA_REGION_SIZE  (APROM_TOTAL_SIZE - SIGNATURE_SIZE) 

// AES Key (16 bytes)
const uint8_t KEY[16] = {
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41
};
// =================================================

void print_hex(const char* label, const uint8_t* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    FILE* fp;
    uint8_t* file_buffer;
    uint8_t* full_image_buffer; // 64KB full image
    uint8_t* calc_buffer;       // Temporary buffer for AES calculation
    long file_len;
    struct AES_ctx ctx;
    uint8_t iv[16];

    // 1. Read original firmware
    fp = fopen("firmware.bin", "rb");
    if (!fp) {
        perror("Error: Cannot open firmware.bin");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    rewind(fp);

    // Check file size
    // Firmware can now occupy up to DATA_REGION_SIZE
    if (file_len > DATA_REGION_SIZE) {
        printf("Error: Firmware too large! Current: %ld, Max allowed: %d\n", file_len, DATA_REGION_SIZE);
        fclose(fp);
        return -1;
    }

    file_buffer = (uint8_t*)malloc(file_len);
    fread(file_buffer, 1, file_len, fp);
    fclose(fp);

    printf("Original firmware size: %ld bytes\n", file_len);

    // 2. Prepare 64KB full image
    // calloc initializes memory to 0, ensuring padding is 0x00
    full_image_buffer = (uint8_t*)calloc(1, APROM_TOTAL_SIZE);

    // Fill firmware data starting from offset 0
    // This ensures the Vector Table is located at 0x0000
    memcpy(full_image_buffer, file_buffer, file_len);

    // Remaining space (Padding) is already zeroed

    // 3. Prepare for AES-CBC calculation
    // Copy data to calc_buffer as tiny-AES-c performs in-place encryption
    calc_buffer = (uint8_t*)malloc(DATA_REGION_SIZE);
    memcpy(calc_buffer, full_image_buffer, DATA_REGION_SIZE);

    printf("Validation range: 0x0000 ~ 0x%X (Total %d bytes)\n", DATA_REGION_SIZE - 1, DATA_REGION_SIZE);

    // 4. Execution
    memset(iv, 0, 16); // IV = 0
    AES_init_ctx_iv(&ctx, KEY, iv);

    // Encrypt the first 65520 bytes
    AES_CBC_encrypt_buffer(&ctx, calc_buffer, DATA_REGION_SIZE);

    // 5. Extract Signature
    // The signature is the last 16 bytes of the encrypted result
    uint8_t* signature = calc_buffer + DATA_REGION_SIZE - 16;

    print_hex("Generated Signature", signature, 16);

    // Copy signature to the end of the full image (0xFFF0)
    memcpy(full_image_buffer + DATA_REGION_SIZE, signature, SIGNATURE_SIZE);

    // 6. Output file
    fp = fopen("firmware_signed_64k.bin", "wb");
    if (fp) {
        fwrite(full_image_buffer, 1, APROM_TOTAL_SIZE, fp);
        fclose(fp);
        printf("Success: Generated firmware_signed_64k.bin (Size: %d)\n", APROM_TOTAL_SIZE);
    }

    // Cleanup
    free(file_buffer);
    free(full_image_buffer);
    free(calc_buffer);

    return 0;
}