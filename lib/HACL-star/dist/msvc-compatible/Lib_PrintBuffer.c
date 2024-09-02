#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <Lib_PrintBuffer.h>

void Lib_PrintBuffer_print_bytes(uint32_t len, uint8_t* buffer) {
  uint32_t i;
  for (i = 0; i < len; i++){
    printf("%02x ", buffer[i]);
  }
  printf("\n");
}

void Lib_PrintBuffer_print_compare(uint32_t len, uint8_t* buffer1, uint8_t* buffer2) {
  uint32_t i;
  for (i = 0; i < len; i++){
    printf("%02x ", buffer1[i]);
  }
  printf("\n");
  for (i = 0; i < len; i++){
    printf("%02x ", buffer2[i]);
  }
  printf("\n");
}

void Lib_PrintBuffer_print_compare_display(uint32_t len, const uint8_t* buffer1, const uint8_t* buffer2) {
  uint8_t res = 0;
  uint32_t i;
  Lib_PrintBuffer_print_compare(len, (uint8_t*)buffer1, (uint8_t*)buffer2);
  for (i = 0; i < len; i++) {
    res |= buffer1[i] ^ buffer2[i];
  }
  if (res == 0) {
    printf("Success !\n");
  } else {
    printf("Failure !\n");
  }
  printf("\n");
}

bool Lib_PrintBuffer_result_compare_display(uint32_t len, const uint8_t* buffer1, const uint8_t* buffer2) {
  uint8_t res = 0;
  uint32_t i;
  Lib_PrintBuffer_print_compare(len, (uint8_t*)buffer1, (uint8_t*)buffer2);
  for (i = 0; i < len; i++) {
    res |= buffer1[i] ^ buffer2[i];
  }
  if (res == 0) {
    printf("Success !\n\n");
    return true;
  } else {
    printf("Failure !\n\n");
    return false;
  }
}

