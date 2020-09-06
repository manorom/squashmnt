#include "squashmnt.h"
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void print_help() {
  puts("\nUsage:\n"
       " squashmnt [-h]\n"
       " squashmnt [OPTIONS] <mountpoint> <imagefile> <upperdir> <workdir>\n"
       " squashmnt -t [OPTIONS] <mountpoint> <imagefile>\n"
       " squashmnt -u [OPTIONS] <mountpoint>\n\n"
       " squashmnt -r [OPTIONS] <mountpoint>"
       "Mounts a squashfs image file read-write with overlayfs \n\n"

       "Options:\n"
       " -t, --tmp            creates upper and workdir in $TMP, $TEMP, or "
       "/tmp. These directories will be cleared on un- and remount"
       " -u, --unmount        unmounts the image\n"
       " -r, --remount        remounts the image\n"
       " -p, --update         recreates the image from the currently mounted "
       "filesystems before un- or remount\n");
  exit(0);
}

int main(int argc, char **argv) {
  int rc;
  int opt_unmount = 0;
  int opt_use_tmp = 0;
  int opt_remount = 0;
  char *mount_point = NULL;
  char *image_file = NULL;
  char *upper_dir = NULL;
  char *lower_dir = NULL;
  char *work_dir = NULL;

  struct option long_options[] = {{"unmount", no_argument, &opt_unmount, 1},
                                  {"remount", no_argument, &opt_remount, 1},
                                  {"tmp", no_argument, &opt_use_tmp, 1},
                                  {"help", no_argument, NULL, 'h'},
                                  {NULL, 0, NULL, 0}};

  char ch;
  while ((ch = getopt_long(argc, argv, "urth", long_options, NULL)) != -1) {
    switch (ch) {
    case 't':
      opt_use_tmp = 1;
      break;
    case 'u':
      opt_unmount = 1;
      break;
    case 'r':
      opt_remount = 1;
      break;
    case '?':
    case 'h':
      print_help();
    }
  }

  int num_args = argc - optind;

  // collect additional arguments
  if (num_args > 0) {
    if (num_args > 4) {
      fprintf(stderr, "Too many arguments");
      return 1;
    }
    if (num_args >= 1)
      mount_point = argv[optind];
    lower_dir = argv[optind];
    if (num_args >= 2)
      image_file = argv[optind + 1];
    if (num_args >= 3)
      upper_dir = argv[optind + 2];
    if (num_args >= 4)
      work_dir = argv[optind + 3];
  }

  if (opt_unmount && opt_remount) {
    fprintf(stderr, "Error: Cannot use -u and -r simultaneously");
    return 1;
  }

  if (opt_unmount) {
    if (num_args > 1) {
      fprintf(stderr, "Error: Cannot use options -u with more then one "
                      "argument. Use 'squashmnt -u <mountpoint>'");
      return 1;
    }

    char *loop_device;
    rc = sqmnt_lookup_mount_mountpoint(mount_point, &upper_dir, &lower_dir,
                                       &work_dir, &loop_device);
    if (rc < 0) {
      fprintf(stderr, "Error: could not find mount at %s\n", mount_point);
      return -1;
    }

    rc = sqmnt_unmount_image(mount_point, lower_dir, loop_device);
    if (rc < 0) {
      fprintf(stderr, "Error could not unmount mount at %s\n", mount_point);
    }
    free(upper_dir);
    free(lower_dir);
    free(work_dir);
    free(loop_device);
    return rc;
  }

  if (opt_remount) {
    if (num_args > 1) {
      fprintf(stderr, "Error: Cannot use options -r with more then one "
                      "argument. Use 'squashmnt -r <mountpoint>'");
      return 1;
    }

    char *loop_device;
    rc = sqmnt_lookup_mount_mountpoint(mount_point, &upper_dir, &lower_dir,
                                       &work_dir, &loop_device);
    if (rc < 0) {
      fprintf(stderr, "Error: Could not find mount at %s\n", mount_point);
      return 1;
    }

    rc = loop_get_backing(loop_device, image_file);
    if (rc < 0) {
      error(0, errno, "Error: Could not find backing file of loop device %s\n",
            loop_device);
      return 1;
    }

    rc = sqmnt_unmount_image(mount_point, lower_dir, loop_device);
    if (rc < 0) {
      fprintf(stderr, "Error: Could not unmount mount at %s\n", mount_point);
      return 1;
    }

    rc = sqmnt_mount_image(mount_point, image_file, upper_dir, lower_dir,
                           work_dir);
    if (rc < 0) {
      fprintf(stderr, "Error: Could not mount image at %s\n", mount_point);
      return 1;
    }

    free(upper_dir);
    free(lower_dir);
    free(work_dir);
    free(loop_device);
    free(image_file);
    return rc;
  }

  if (num_args == 0) {
    fprintf(stderr, "Error: Listing not implemented, yet");
    return 1;
  }

  if (num_args == 2) {
    if (opt_use_tmp) {
      rc = sqmnt_create_tmp_dirs(&upper_dir, &work_dir);
      if (rc < 0) {
        perror("Error: Could not create directories for overlayfs");
        return 1;
      }

      rc = sqmnt_mount_image(mount_point, image_file, upper_dir, lower_dir,
                             work_dir);
      if (rc < 0) {
        fprintf(stderr, "Error: Could not mount image at %s\n", mount_point);
        return 1;
      }

      free(upper_dir);
      free(work_dir);
      return rc;
    }
    fprintf(stderr, "Error: Not enough arguments (maybe use -t)");
    return 1;
  }

  if (num_args == 4) {
    if (opt_use_tmp) {
      fprintf(stderr, "Error: Too many arguments when using the -t flag");
      return 1;
    }

    rc = sqmnt_mount_image(mount_point, image_file, upper_dir, lower_dir,
                           work_dir);
    if (rc < 0) {
      fprintf(stderr, "Error: Could not mount image at %s\n", mount_point);
      return 1;
    }
    return 0;
  }

  fprintf(stderr, "Unsupported number of arguments: %i\n", num_args);
  return 1;
}
