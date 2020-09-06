#include <errno.h>
#include <fcntl.h>
#include <linux/loop.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "squashmnt.h"

int loop_setup(const char *backing_file, char *loop_device) {
  int control_fd, devnr, backing_file_fd, loop_device_fd;
  int err = 0;
  if ((control_fd = open("/dev/loop-control", O_RDONLY)) < 0) {
    return -1;
  }

  devnr = ioctl(control_fd, LOOP_CTL_GET_FREE);
  if (devnr < 0 || devnr > 999) {
    err = -2;
    printf("\nErr: LOOP_CTL_GET_FREE %i, %i\n", devnr, errno);
    goto fail_controlfd;
  }

  sprintf(loop_device, "/dev/loop%i", devnr);
  if ((backing_file_fd = open(backing_file, O_RDONLY)) < 0) {
    err = -3;
    goto fail_controlfd;
  }

  if ((loop_device_fd = open(loop_device, O_RDWR)) < 0) {
    err = -4;
    goto fail_backingfd;
  }

  err = ioctl(loop_device_fd, LOOP_SET_FD, backing_file_fd);
  if (err < 0) {
    puts("\nErr: LOOP_SET_FD");
    errno = -err;
    err = -5;
  }

  /* insert set_status code here */

  close(loop_device_fd);
fail_backingfd:
  close(backing_file_fd);
fail_controlfd:
  close(control_fd);
  return err;
}

int loop_detach(const char *loop_device) {
  int loop_device_fd;
  int err = 0;
  if ((loop_device_fd = open(loop_device, O_RDONLY)) < 0) {
    err = -1;
    goto fail_open;
  }
  if (ioctl(loop_device_fd, LOOP_CLR_FD, 0) < 0) {
    err = -2;
  }
  close(loop_device_fd);
fail_open:
  return err;
}

int loop_get_backing(const char *loop_device, char *backing_file) {
  int loop_device_fd;
  int rc;

  if ((loop_device_fd = open(loop_device, O_RDONLY)) < 0) {
    return -1;
  }

  struct loop_info64 loop_info;
  rc = ioctl(loop_device_fd, LOOP_GET_STATUS64, &loop_info);
  if (rc < 0) {
    errno = -rc;
    close(loop_device_fd);
    return -1;
  }

  strcpy(backing_file, (char *)loop_info.lo_file_name);
  return 0;
}
