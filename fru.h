#ifndef __FRU_H__
#define __FRU_H__

#ifdef RECOVERY
#include <stdint.h>
#else
#include <common.h>
#endif
#include <stdbool.h>

#define MR_MAC_REC 0xC0
#define MR_UBOOT_REC 0xC1

#define FRU_ADDR 0xa6
#define FRU_PAGE_SIZE 32
#define FRU_ADDR_SIZE 2

#define FRU_STR_MAX 32
#define N_MULTIREC 8

#define FRU_STR(name, len) unsigned int len_##name; uint8_t val_##name[len]

struct multirec {
  uint8_t type;
  uint8_t format;
  bool end;
  unsigned int length;
  bool header_cs_ok;
  bool cs_ok;
  uint8_t *data;
};

struct fru {
  uint8_t mac[6];
  uint8_t mfg_date[3];
  uint8_t bootdevice[16];
  unsigned int board_area_offset;
  unsigned int product_area_offset;
  unsigned int mrec_area_offset;  
  FRU_STR(mfg_name, FRU_STR_MAX);
  FRU_STR(product_name, FRU_STR_MAX);
  FRU_STR(serial_number, FRU_STR_MAX);
  FRU_STR(part_number, FRU_STR_MAX);
  FRU_STR(fru_id, FRU_STR_MAX);
  
  FRU_STR(p_product_mfg, FRU_STR_MAX);
  FRU_STR(p_product_name, FRU_STR_MAX);
  FRU_STR(p_part_model_number, FRU_STR_MAX);
  FRU_STR(p_product_version, FRU_STR_MAX);
  FRU_STR(p_serial_number, FRU_STR_MAX);
  FRU_STR(p_fru_id, FRU_STR_MAX);
  struct multirec mrec[N_MULTIREC];
  unsigned int mrec_count;
};

extern struct fru fru;
int fru_open_parse(void);
int fru_update_mac(uint8_t *mac);
int fru_update_mrec_eeprom(void);
void print_board_area(struct fru *f);
void print_product_area(struct fru *f);

#endif/*__FRU_H__*/
