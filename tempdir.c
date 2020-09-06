#define _GNU_SOURCE
#include <stdlib.h>
#include "squashmnt.h"

int sqmnt_create_tmp_dirs(char **upper_dir, char **work_dir) {
  const char *tmp_dir = getenv("TMP");
  if (!tmp_dir)
    tmp_dir = getenv("TEMP");
  if (!tmp_dir)
    tmp_dir = "/tmp";

  asprintf(upper_dir, "%s/squashmnt-upper-XXXXXXXX", tmp_dir);
  asprintf(work_dir, "%s/squashmnt-work-XXXXXXX", tmp_dir);

  *upper_dir = mkdtemp(*upper_dir);

  if (!*upper_dir) {
    fprintf(stderr, "Could not create overlayfs' upper dir");
    return -1;
  }

  *work_dir = mkdtemp(*work_dir);

  if (!*work_dir) {
    fprintf(stderr, "Could not create overlayfs work dir");
    return -1;
  }


  return 0;
}

