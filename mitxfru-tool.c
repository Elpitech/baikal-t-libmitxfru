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
  "  -d : multirecord data to set; for use with -s option\n";

bool qflag = 0;

int
main (int argc, char **argv) {
  bool hflag = 0;
  char *gvalue = NULL;
  char *svalue = NULL;
  char *dvalue = NULL;
  uint32_t val;
  int c;
  int ret;
  int i = 0;

  opterr = 0;

  while ((c = getopt (argc, argv, "qhg:s:d:")) != -1) {
    switch (c) {
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

  if (svalue != NULL) {
    val = strtoul(svalue, NULL, 16);
    switch (val) {
    case MR_MAC_REC:
      ret = sscanf(dvalue, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int*)&fru.mac[0], (unsigned int*)&fru.mac[1], (unsigned int*)&fru.mac[2], (unsigned int*)&fru.mac[3], (unsigned int*)&fru.mac[4], (unsigned int*)&fru.mac[5]);
      if (ret<6) {
        err("MAC format is not recognized; Example: 01:02:03:04:05:06\n");
        return -5;
      }
      break;
    case MR_SATADEV_REC:
      if (dvalue == NULL) {
        err("-d is not set, please bother yourself with reading some help\n");
        return -4;
      }
      memset(fru.bootdevice, 0, FRU_STR_MAX);
      memcpy(fru.bootdevice, dvalue, strlen(dvalue));
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
    default:
      err("Unknown multirecord id %i\n", val);
      return -2;
      break;
    }
  }
  
  return 0;
}
