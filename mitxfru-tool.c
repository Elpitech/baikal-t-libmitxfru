#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "fru.h"
#include "common.h"

#define TAG "MITXFRUTOOL"

static const char usage[] = "  -q : quite; dont print anything unrelated to what you asked\n"
  "  -h : help; you are reading it already though\n"
  "  -g : get multirecord by hex id\n"
  "  -s : set multirecord by hex id, requires -d option to be filled with some data\n"
  "  -d : multirecord data to set; for use with -s option\n"
  "  -r : display FRU information\n";

bool qflag = false;

int
main (int argc, char **argv) {
  bool hflag = false;
  bool rflag = false;
  char *gvalue = NULL;
  char *svalue = NULL;
  char *dvalue = NULL;
  uint32_t val;
  uint8_t mac[6];
  uint8_t test_ok;
  uint8_t power_policy;
  int c;
  int ret;
  int i = 0;

  opterr = 0;

  while ((c = getopt (argc, argv, "rqhg:s:d:")) != -1) {
    switch (c) {
    case 'r':
      rflag = true;
      break;
    case 'q':
      qflag = true;
      break;
    case 'h':
      hflag = true;
      break;
    case 'g':
      gvalue = optarg;
      break;
    case 's':
      svalue = optarg;
      break;
    case 'd':
      dvalue = optarg;
      break;
    case '?':
      if (optopt == 'g') {
        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      } else if (optopt == 's') {
        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      } else if (optopt == 'd') {
        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      } else if (isprint (optopt)) {
        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      } else {
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
      }
      return 1;
    default:
      abort ();
    }
  }

  if (hflag) {
    printf(usage);
    return 0;
  }

  log("Started\n");
  ret = fru_open_parse();
  if (ret != 0) {
    err("Failed to load data from EEPROM\n");
    return -1;
  }

  if (rflag) {
    printf("b_mfg_name, %s\n", fru.val_mfg_name);
    printf("b_product_name, %s\n", fru.val_product_name);
    printf("b_serial_number, %s\n", fru.val_serial_number);
    printf("b_part_number, %s\n", fru.val_part_number);
    printf("b_fru_id, %s\n", fru.val_fru_id);

    printf("p_product_mfg, %s\n", fru.val_p_product_mfg);
    printf("p_product_name, %s\n", fru.val_p_product_name);
    printf("p_part_model_number, %s\n", fru.val_p_part_model_number);
    printf("p_product_version, %s\n", fru.val_p_product_version);
    printf("p_serial_number, %s\n", fru.val_p_serial_number);
    printf("p_fru_id, %s\n", fru.val_p_fru_id);
    return 0;
  }

  if (svalue != NULL) {
    val = strtoul(svalue, NULL, 16);
    if (dvalue == NULL) {
      err("-d is not set, please bother yourself with reading some help\n");
      return -4;
    }
    switch (val) {
    case MR_MAC_REC:
      ret = sscanf(dvalue, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int*)&mac[0], (unsigned int*)&mac[1], (unsigned int*)&mac[2], (unsigned int*)&mac[3], (unsigned int*)&mac[4], (unsigned int*)&mac[5]);
      if (ret<6) {
        err("MAC format is not recognized; Example: 01:02:03:04:05:06\n");
        return -5;
      }
      fru_mrec_update_mac(&fru, mac);
      break;
    case MR_SATADEV_REC:
      fru_mrec_update_bootdevice(&fru, (uint8_t*)dvalue);
      break;
    case MR_PASSWD_REC:
      fru_mrec_update_passwd_line(&fru, (uint8_t*)dvalue);
      break;
    case MR_TESTOK_REC:
      ret = sscanf(dvalue, "%i", (unsigned int *)&test_ok);
      if (ret != 1) {
        err("Test state format not recognized\n");
        return -6;
      }
      fru_mrec_update_test_ok(&fru, test_ok);
      break;
    case MR_POWER_POLICY_REC:
      ret = sscanf(dvalue, "%i", (unsigned int *)&power_policy);
      if (ret != 1) {
        err("Power policy format not recognized\n");
        return -6;
      }
      fru_mrec_update_power_policy(&fru, power_policy);
      break;

    default:
      err("Unknown multirecord id %i\n", val);
      return -3;
      break;
    }
    log("Updating multirecord\n");
    fru_update_mrec_eeprom();
    log("Saving data to EEPROM\n");
    for (i=0;i<10;i++) {
      sleep(1);
      msg(".");
      fflush(stdout);
    }
    sync();
  } else if (gvalue != NULL) {
    uint32_t val;
    val = strtoul(gvalue, NULL, 16);
    switch (val) {
    case MR_MAC_REC:
      printf("%02x:%02x:%02x:%02x:%02x:%02x\n", fru.mac[0], fru.mac[1], fru.mac[2], fru.mac[3], fru.mac[4], fru.mac[5]);
      break;
    case MR_SATADEV_REC:
      printf("%s\n", fru.bootdevice);
      break;
    case MR_PASSWD_REC:
      printf("%s\n", fru.passwd_line);
      break;
    case MR_TESTOK_REC:
      printf("%i\n", fru.test_ok);
      break;
    case MR_POWER_POLICY_REC:
      printf("%i\n", fru.power_policy);
      break;
    default:
      err("Unknown multirecord id %i\n", val);
      return -2;
      break;
    }
  }
  
  return 0;
}
