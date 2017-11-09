#include "fru.h"
#include <stdbool.h>

#define FRU_VERSION 1
#define BOARD_AREA_VERSION 1
#define PRODUCT_AREA_VERSION 1
#define FRU_SIZE 4096
#define TAG "FRU"

#ifdef RECOVERY
#include <string.h>
#include <stdio.h>
#include "common.h"

#ifdef FRU_DEBUG
#define dbg(...) {fprintf (logfile, __VA_ARGS__); fflush(logfile); }
#else
#define dbg(...)
#endif

#else
#include <common.h>
#include <i2c.h>
#define msg(...) {printf (__VA_ARGS__); }
#define log(...) {printf ("L["TAG"]: "__VA_ARGS__); }
#define warn(...) {printf ("W["TAG"]: "__VA_ARGS__); }
#define err(...) {printf ("E["TAG"]: "__VA_ARGS__); }

#ifdef FRU_DEBUG
#define dbg(...) {printf (__VA_ARGS__); }
#else
#define dbg(...)
#endif

#endif

static uint8_t fru_buf[FRU_SIZE];
static uint8_t fru_buf2[FRU_SIZE];
struct fru fru;

#define MR_MAC_REC 0xC0
#define MR_UBOOT_REC 0xC1

uint8_t
calc_cs(uint8_t *buf, uint8_t size) {
  uint8_t cs = 0;
  int i = 0;
  dbg("CS:\n");
  for (;i<size; i++) {
    cs += buf[i];
#ifdef FRU_DEBUG
    if (i%8==0) {
      dbg("\n");
    }
    dbg("0x%02x ", cs);
#endif
  }
  dbg("\n");
  return cs;
}

int
fru_mk_multirecord(uint8_t *buf, unsigned int buf_size, uint8_t record_type, bool end, uint8_t *record, uint8_t record_size) {
  int i = 0;
  int size = 5+record_size;
  int remainder = buf_size-size;
  log("packing multirecord");

  if (remainder<=0) {
    return -1;
  }
  buf[0] = record_type;
  buf[1] = (end ? (1<<7) : 0) | 0x02;
  buf[2] = record_size;
  memcpy((buf+5), record, record_size);
  buf[3] = 256-calc_cs(buf+5, record_size);
  buf[4] = 256-calc_cs(buf, 4);
  for (;i<size;i++) {
    if (i%8==0) {
      dbg("\r\n");
    }
    dbg("%02x[%c] ", buf[i], (buf[i]>=0x20 ? buf[i] : ' '));
  }
  dbg("\r\n");
  return size;
}

int
read_fru_str(uint8_t *buf, uint8_t *str, unsigned int *len, unsigned int offt) {
  *len = ((buf[offt]>(FRU_STR_MAX-1))?(FRU_STR_MAX-1):buf[offt]);
  memset(str, 0, FRU_STR_MAX);
  memcpy(str, &buf[offt+1], *len);
  offt += buf[offt]+1;
  return offt;
}

int
parse_board_area(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  uint8_t cs;
  int offt = 0;
  
  if (buf[0] != BOARD_AREA_VERSION) {
    warn("FRU: Board area version is not valid\n");
    return -1;
  }
  if ((buf[1]*8)>buf_len) {
    warn("FRU: Board area size mismatch\n");
    return -2;
  }
#ifdef FRU_DEBUG
  {
    int i = 0;
    log("FRU Board area bin:\n");
    for (;i<buf[1]*8;i++) {
      if ((i%8)==0) {
        dbg("\n");
      }
      dbg("0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
    }
    dbg("\n");
  }
#endif

  cs = calc_cs(buf, buf[1]*8);
  if (cs != 0) {
    warn("FRU: Bad board area checksum [0-%i]: %i\n", buf[1]*8, cs);
    return -3;
  }
  f->mfg_date[0] = buf[3];
  f->mfg_date[1] = buf[4];
  f->mfg_date[2] = buf[5];

  offt = 5;
  offt = read_fru_str(buf, f->val_mfg_name, &f->len_mfg_name, offt);
  offt = read_fru_str(buf, f->val_product_name, &f->len_product_name, offt);
  offt = read_fru_str(buf, f->val_serial_number, &f->len_serial_number, offt);
  offt = read_fru_str(buf, f->val_part_number, &f->len_part_number, offt);
  offt = read_fru_str(buf, f->val_fru_id, &f->len_fru_id, offt);
#ifdef FRU_DEBUG
  print_board_area(&fru);
#endif
  return 0;
}

void
print_board_area(struct fru *f) {
  log("FRU Board area:\n");
  log("Board mfg:          \t%s\n", f->val_mfg_name);
  log("Board name:         \t%s\n", f->val_product_name);
  log("Board serial number:\t%s\n", f->val_serial_number);
  log("Board part number:  \t%s\n", f->val_part_number);
  log("Board fru id:       \t%s\n", f->val_fru_id);
}

int
parse_product_area(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  int offt = 0;
  if (buf[0] != PRODUCT_AREA_VERSION) {
    warn("FRU: Product area version is not valid\n");
    return -1;
  }
  if ((buf[1]*8)>buf_len) {
    warn("FRU: Product area size mismatch\n");
    return -2;
  }
#ifdef FRU_DEBUG
  {
    int i = 0;
    log("FRU Product area bin:\n");
    for (i=0; i<buf[1]*8; i++) {
      if ((i%8)==0) {
        dbg("\n");
      }
      dbg("0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
    }
    dbg("\n");
  }
#endif

  if (calc_cs(buf, buf[1]*8) != 0) {
    warn("FRU: Bad product area checksum\n");
    return -3;
  }
  offt = 3;
  offt = read_fru_str(buf, f->val_p_product_mfg, &f->len_p_product_mfg, offt);
  offt = read_fru_str(buf, f->val_p_product_name, &f->len_p_product_name, offt);
  offt = read_fru_str(buf, f->val_p_part_model_number, &f->len_p_part_model_number, offt);
  offt = read_fru_str(buf, f->val_p_product_version, &f->len_p_product_version, offt);
  offt = read_fru_str(buf, f->val_p_serial_number, &f->len_p_serial_number, offt);
  offt = read_fru_str(buf, f->val_p_fru_id, &f->len_p_fru_id, offt);
#ifdef FRU_DEBUG
  print_product_area(&fru);
#endif
  return 0;
}

void
print_product_area(struct fru *f) {
  log("FRU Product area:\n");
  log("Product mfg:          \t%s\n", f->val_p_product_mfg);
  log("Product name:         \t%s\n", f->val_p_product_name);
  log("Product model number: \t%s\n", f->val_p_part_model_number);
  log("Product version:      \t%s\n", f->val_p_product_version);
  log("Product serial number:\t%s\n", f->val_p_serial_number);
  log("Product fru id:       \t%s\n", f->val_p_fru_id);
}

int
fru_parse_multirecord(struct multirec *m, uint8_t *buf, unsigned int buf_len) {
  uint8_t data_cs;
  if (buf_len<5) {
    warn("FRU: no space in multirecord buffer, failed to parse header\n");
    return -4;
  }
#ifdef FRU_DEBUG
  {
    int i = 0;
    log("FRU mrec header bin:\n");
    for (;i<5;i++) {
      dbg("0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
    }
    dbg("\n");
  }
#endif

  if (calc_cs(buf, 5) != 0) {
    warn("FRU: multirecord header checksum is invalid\n");
    m->header_cs_ok = false;
    return -1;
  } else {
    m->header_cs_ok = true;
  }
  m->type = buf[0];
  m->format = buf[1]&0x7;
  if (m->format != 0x2) {
    warn("FRU: multirecord format is unknown [%i]\n", m->format);
    return -2;
  }
  m->end = (buf[1]&0x80?true:false);
  if (m->end) {
    dbg("FRU: last multirecord\n");
  }
  m->length = buf[2];
  if ((buf_len-5)<(m->length)) {
    warn("FRU: no space in multirecord buffer, failed check data\n");
    return -5;
  }
#ifdef FRU_DEBUG
  {
    int i = 0;
    log("FRU mrec bin:\n");
    for (;i<(m->length+5);i++) {
      if ((i%8)==0) {
        dbg("\n");
      }
      dbg("0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
    }
    dbg("\n");
  }
#endif
  
  data_cs = calc_cs(&buf[5], m->length)+buf[3];
  if (data_cs != 0) {
    warn("FRU: multirecord data checksum is invalid [0x%02x]\n", data_cs);
    return -3;
  }
  m->data = &buf[5];
  return m->length+5;
}

int
parse_fru(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  int ret = 0;
  int mrec_n = 0;
  int offt = 0;
  if (buf_len<8) {
    warn("FRU buffer is too short\n");
    return -1;
  }
  dbg("FRU: Checking header [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
  if (buf[0] == 0xff) {
    warn("FRU: Empty EEPROM detected\n");
    return -2;
  } else if (buf[0] != FRU_VERSION) {
    warn("FRU: Header version is not valid\n");
    return -3;
  } else if (calc_cs(buf, 8) != 0) {
    warn("FRU: Bad header checksum: %i\n", calc_cs(buf, 8));
    return -4;
  }
  f->board_area_offset = buf[3]*8;
  f->product_area_offset = buf[4]*8;
  f->mrec_area_offset = buf[5]*8;
  if (parse_board_area(f, &buf[f->board_area_offset], buf_len-f->board_area_offset)) {
    return -5;
  }
  if (parse_product_area(f, &buf[f->product_area_offset], buf_len-f->product_area_offset)) {
    return -6;
  }
  f->mrec_count = 0;
  offt = f->mrec_area_offset;
  while (ret >= 0 && (mrec_n < N_MULTIREC)) {
    dbg("FRU: parsing multirecord %i\n", f->mrec_count);
    ret = fru_parse_multirecord(&f->mrec[mrec_n], &buf[offt], buf_len-offt);
    if (ret > 0) {
      f->mrec_count ++;
    } else {
      warn("FRU: Failed to parse multirecord\n");
      return -7;
    }
    if (f->mrec[mrec_n].end) {
      break;
    } else {
      offt += ret;
    }
    mrec_n ++;
  }
  return 0;
}

int
fru_mk_multirecords_area(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  int i = 0;
  int ret = 0;
  unsigned int offt = 0;
  log("Packing multirecord area\n");
  for (;i<f->mrec_count; i++) {
    ret = fru_mk_multirecord(buf+offt, buf_len-offt, f->mrec[i].type, (f->mrec_count-i>1?false:true), f->mrec[i].data, f->mrec[i].length);
    if (ret < 0) {
      warn("FRU: Failed to pack multirecord\n");
      return -1;
    }
    offt+=ret;
  }
  return 0;
}

int
fru_mrec_update_mac(struct fru *f, uint8_t *mac) {
  int i = 0;
  memcpy(f->mac, mac, 6);
  for (; i<f->mrec_count; i++) {
    if (f->mrec[i].type == MR_MAC_REC) {
      f->mrec[i].data = f->mac;
      return 0;
    }
  }
  return -1;
}

int
fru_mrec_update_bootdevice(struct fru *f, uint8_t *bootdevice) {
  int i = 0;
  int len = strlen((char *)bootdevice);
  len = (len>FRU_STR_MAX?FRU_STR_MAX:len);
  memset(f->bootdevice, 0, FRU_STR_MAX);
  memcpy(f->bootdevice, bootdevice, (len>FRU_STR_MAX?FRU_STR_MAX:len));
  log("Checking if bootdev mrec already exists\n");
  for (; i<f->mrec_count; i++) {
    if (f->mrec[i].type == MR_SATADEV_REC) {
      log("Found bootdev mrec, updating\n");
      f->mrec[i].data = f->bootdevice;
      f->mrec[i].length = len;
      return 0;
    }
  }
  log("Bootdev mrec not found, creating mrec %i\n", f->mrec_count);
  //no mrec for sata found, creating one
  struct multirec *m = &(f->mrec[f->mrec_count]);
  m->type = MR_SATADEV_REC;
  m->format = 2;
  m->end = true;
  m->length = len;
  m->data = f->bootdevice;
  f->mrec_count ++;
  return -1;
}

int
fru_mrec_update_passwd_line(struct fru *f, uint8_t *passwd_line) {
  int i = 0;
  int len = strlen((char *)passwd_line);
  len = (len>FRU_PWD_MAX?FRU_PWD_MAX:len);
  memset(f->passwd_line, 0, FRU_PWD_MAX);
  memcpy(f->passwd_line, passwd_line, (len>FRU_PWD_MAX?FRU_PWD_MAX:len));
  log("Checking if passwd line mrec already exists\n");
  for (; i<f->mrec_count; i++) {
    if (f->mrec[i].type == MR_PASSWD_REC) {
      log("Found passwd line mrec, updating\n");
      f->mrec[i].data = f->passwd_line;
      f->mrec[i].length = len;
      return 0;
    }
  }
  log("Passwd line mrec not found, creating mrec %i\n", f->mrec_count);
  struct multirec *m = &(f->mrec[f->mrec_count]);
  m->type = MR_PASSWD_REC;
  m->format = 2;
  m->end = true;
  m->length = len;
  m->data = f->passwd_line;
  f->mrec_count ++;
  return -1;
}

int
fru_mrec_update_test_ok(struct fru *f, uint8_t test_ok) {
  int i = 0;
  f->test_ok = test_ok;
  log("Checking if test ok mrec already exists\n");
  for (; i<f->mrec_count; i++) {
    if (f->mrec[i].type == MR_TESTOK_REC) {
      log("Found test ok mrec, updating\n");
      f->mrec[i].data = &f->test_ok;
      f->mrec[i].length = 1;
      return 0;
    }
  }
  log("Test ok mrec not found, creating mrec %i\n", f->mrec_count);
  struct multirec *m = &(f->mrec[f->mrec_count]);
  m->type = MR_TESTOK_REC;
  m->format = 2;
  m->end = true;
  m->length = 1;
  m->data = &f->test_ok;
  f->mrec_count ++;
  return -1;
}

int
fru_mrec_update_power_policy(struct fru *f, enum POWER_POLICY pp) {
  int i = 0;
  f->power_policy = pp;
  log("Checking if power policy mrec already exists\n");
  for (; i<f->mrec_count; i++) {
    if (f->mrec[i].type == MR_POWER_POLICY_REC) {
      log("Found power policy mrec, updating\n");
      f->mrec[i].data = &f->power_policy;
      f->mrec[i].length = 1;
      return 0;
    }
  }
  log("Power policy mrec not found, creating mrec %i\n", f->mrec_count);
  struct multirec *m = &(f->mrec[f->mrec_count]);
  m->type = MR_POWER_POLICY_REC;
  m->format = 2;
  m->end = true;
  m->length = 1;
  m->data = &f->power_policy;
  f->mrec_count ++;
  return -1;
}

#ifdef RECOVERY
int
fru_open_parse(void) {
  FILE *f = fopen("/sys/bus/i2c/devices/1-0053/eeprom", "r");
  int i = 0;
  int ret = 0;
  if (f == NULL) {
    err("FRU: failed to open eeprom\n");
    return -1;
  }
  dbg("Reading eeprom\n");
  ret = fread(fru_buf, sizeof(uint8_t), FRU_SIZE, f);
  dbg("Read %i bytes\n", ret);
  if (parse_fru(&fru, fru_buf, FRU_SIZE) != 0) {
    return -2;
  }
  for (i=0; i<fru.mrec_count; i++) {
    if (fru.mrec[i].type == MR_MAC_REC) {
      memcpy(fru.mac, fru.mrec[i].data, 6);
      dbg("FRU: found MAC mrec [%02x %02x %02x %02x %02x %02x]\n", fru.mac[0], fru.mac[1], fru.mac[2], fru.mac[3], fru.mac[4], fru.mac[5]);
    } else if (fru.mrec[i].type == MR_SATADEV_REC) {
      memset(fru.bootdevice, 0, FRU_STR_MAX);
      memcpy(fru.bootdevice, fru.mrec[i].data, (fru.mrec[i].length>FRU_STR_MAX?FRU_STR_MAX:fru.mrec[i].length));
      dbg("FRU: found SATA boot device [%s]\n", fru.bootdevice);
    } else if (fru.mrec[i].type == MR_PASSWD_REC) {
      memset(fru.passwd_line, 0, FRU_PWD_MAX);
      memcpy(fru.passwd_line, fru.mrec[i].data, (fru.mrec[i].length>FRU_PWD_MAX?FRU_PWD_MAX:fru.mrec[i].length));
      dbg("FRU: found passwd line [%s]\n", fru.passwd_line);
    } else if (fru.mrec[i].type == MR_TESTOK_REC) {
      fru.test_ok = 0;
      memcpy(&fru.test_ok, fru.mrec[i].data, 1);
      dbg("FRU: found test ok record [0x%02x]\n", fru.test_ok);
    } else if (fru.mrec[i].type == MR_POWER_POLICY_REC) {
      fru.power_policy = 0;
      memcpy(&fru.power_policy, fru.mrec[i].data, 1);
      dbg("FRU: found power policy record [0x%02x]\n", fru.power_policy);
    }
  }
  fclose(f);
  return 0;
}

int
fru_update_mrec_eeprom(void) {
  int ret = 0;
  FILE *f = NULL;
  memcpy(fru_buf2, fru_buf, fru.mrec_area_offset);
  dbg("Put multirecord area at %i\n", fru.mrec_area_offset);
  ret = fru_mk_multirecords_area(&fru, fru_buf2+fru.mrec_area_offset, FRU_SIZE-fru.mrec_area_offset);
  if (ret < 0) {
    return -1;
  }
  log("Writing eeprom ");
  f = fopen("/sys/bus/i2c/devices/1-0053/eeprom", "w");
  if (f == NULL) {
    err("FRU: failed to open eeprom\n");
    return -1;
  }
  ret = fwrite(fru_buf2, sizeof(uint8_t), FRU_SIZE, f);
  dbg("Wrote %i bytes\n", ret);
  fclose(f);
  return 0;
}


#else
int
fru_open_parse(void) {
  int ret = 0;
  int i;
  dbg("Reading eeprom\n");
  if (i2c_set_bus_num(CONFIG_SYS_OEM_BUS_NUM)) {
		return -1;
  }

  for (i=0;i<FRU_SIZE;i+=32) {
    ret = i2c_read(CONFIG_SYS_OEM_I2C_ADDR | 1, i, 2, fru_buf+i, 32);
    if (ret != 0) {
      err("FRU: failed to read eeprom [%i]\n", ret);
      return -1;
    }

#ifdef FRU_DEBUG
    int j;
    for (j=i;j<32+i;j++) {
      if ((j%8)==0) {
        dbg("\n");
      }
      dbg("%02x[%c] ", fru_buf[j], (fru_buf[j]>' '?fru_buf[j]:' '));
    }
    dbg("\n");
#endif

  }

  if (parse_fru(&fru, fru_buf, FRU_SIZE) != 0) {
    return -2;
  }
  for (i=0; i<fru.mrec_count; i++) {
    if (fru.mrec[i].type == MR_MAC_REC) {
      memcpy(fru.mac, fru.mrec[i].data, 6);
      dbg("FRU: found MAC mrec [%02x %02x %02x %02x %02x %02x]\n", fru.mac[0], fru.mac[1], fru.mac[2], fru.mac[3], fru.mac[4], fru.mac[5]);
    } else if (fru.mrec[i].type == MR_SATADEV_REC) {
      memset(fru.bootdevice, 0, FRU_STR_MAX);
      memcpy(fru.bootdevice, fru.mrec[i].data, (fru.mrec[i].length>FRU_STR_MAX?FRU_STR_MAX:fru.mrec[i].length));
      dbg("FRU: found SATA boot device [%s]\n", fru.bootdevice);
    } else if (fru.mrec[i].type == MR_PASSWD_REC) {
      memset(fru.passwd_line, 0, FRU_PWD_MAX);
      memcpy(fru.passwd_line, fru.mrec[i].data, (fru.mrec[i].length>FRU_PWD_MAX?FRU_PWD_MAX:fru.mrec[i].length));
      dbg("FRU: found passwd line [%s]\n", fru.passwd_line);
    } else if (fru.mrec[i].type == MR_TESTOK_REC) {
      fru.test_ok = 0;
      memcpy(&fru.test_ok, fru.mrec[i].data, 1);
      dbg("FRU: found test ok record [0x%02x]\n", fru.test_ok);
    } else if (fru.mrec[i].type == MR_POWER_POLICY_REC) {
      fru.power_policy = 0;
      memcpy(&fru.power_policy, fru.mrec[i].data, 1);
      dbg("FRU: found power policy record [0x%02x]\n", fru.power_policy);
    }
  }
  return 0;
}

int
fru_update_mrec_eeprom(void) {
  int i = 0;
  int ret = 0;
  memcpy(fru_buf2, fru_buf, fru.mrec_area_offset);
  printf("Put multirecord area at %i\n", fru.mrec_area_offset);
  ret = fru_mk_multirecords_area(&fru, fru_buf2+fru.mrec_area_offset, FRU_SIZE-fru.mrec_area_offset);
  if (ret < 0) {
    return -1;
  }
  log("Writing eeprom ");
  if (i2c_set_bus_num(CONFIG_SYS_OEM_BUS_NUM)) {
		return -2;
  }

  for (i=0;i<FRU_SIZE;i+=32) {
    ret = i2c_write(CONFIG_SYS_OEM_I2C_ADDR, i, 2, fru_buf2+i, 32);
    if (ret != 0) {
      err("FRU: failed to write eeprom [%i]\n", ret);
      return -3;
    }

#ifdef FRU_DEBUG
    int j;
    for (j=i;j<32+i;j++) {
      if ((j%8)==0) {
        dbg("\n");
      }
      dbg("0x%02x[%c] ", fru_buf2[j], (fru_buf2[j]>' '?fru_buf2[j]:' '));
    }
    dbg("\n");
#else
    msg(".");
#endif
    udelay(5000);

  }
  msg("\n");
  return 0;
}
#endif
