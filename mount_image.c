#include "squashmnt.h"
#include <errno.h>
#include <error.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>

#define OPTION_TEMPLATE "lowerdir=%s,upperdir=%s,workdir=%s,index=off"
#define SIZE_OF_OPTION_TEMPLATE (strlen(OPTION_TEMPLATE) + 1)

static int mount_overlay(const char *mount_point, const char *upper_dir,
                         const char *lower_dir, const char *work_dir) {
  int ret = 0;
  char *overlay_options = malloc(strlen(lower_dir) + strlen(upper_dir) +
                                 strlen(work_dir) + SIZE_OF_OPTION_TEMPLATE);
  sprintf(overlay_options, OPTION_TEMPLATE, lower_dir, upper_dir, work_dir);
  if (mount("overlay", mount_point, "overlay", 0, overlay_options) < 0) {
    ret = -1;
  }
  free(overlay_options);
  return ret;
}

static int mount_squashfs(const char *mount_point, const char *device) {
  if (mount(device, mount_point, "squashfs", MS_RDONLY, "") < 0)
    return -1;
  else
    return 0;
}

int sqmnt_mount_image(const char *mount_point, const char *image_file,
                      const char *upper_dir, const char *lower_dir,
                      const char *work_dir) {
  char loop_device[14];

  if (loop_setup(image_file, loop_device) < 0) {
    perror("Could not set up loop device");
    return -1;
  }

  if (mount_squashfs(lower_dir, loop_device) < 0) {
    perror("Could not mount squashfs image");
    goto fail_detach_loop;
  }

  if (mount_overlay(mount_point, upper_dir, lower_dir, work_dir) < 0) {
    perror("Could not mount overlay");
    goto fail_unmount_squash;
  }
  return 0;

fail_unmount_squash:
  umount(lower_dir);
fail_detach_loop:
  loop_detach(loop_device);
  return -1;
}

int sqmnt_unmount_image(const char *mount_point, const char *lower_dir,
                        const char *loop_device) {

  if (umount(mount_point) != 0) {
    error(0, errno, "Could not unmount %s", mount_point);
    return -1;
  }

  if (umount(lower_dir) != 0) {
    error(0, errno, "Could not unmount %s (overlayfs lower dir)", lower_dir);
    return -1;
  }

  if (loop_detach(loop_device) != 0) {
    error(0, errno, "Could not detach loop device %s", loop_device);
    return -1;
  }

  return 0;
}
