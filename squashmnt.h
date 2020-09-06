#include <stdio.h>

/*struct sqmnt_mount_record {
  char *mount_point;
  char *loop_device;
  char *overlay_upper_dir;
  char *overlay_lower_dir;
  char *overlay_work_dir;
};*/

int loop_setup(const char *backing_file, char *loop_device);
int loop_detach(const char *loop_device);
int loop_get_backing(const char *loop_device, char *backing_file);

int sqmnt_mount_image(const char *mount_point, const char *image_file,
                      const char *upper_dir, const char *lower_dir,
                      const char *work_dir);
int sqmnt_unmount_image(const char *mount_point, const char *lower_dir, const char *loop_device);

int sqmnt_lookup_mount_mountpoint(const char *mountpoint,
                                  char **overlay_upper_dir,
                                  char **overlay_lower_dir,
                                  char **overlay_work_dir,
                                  char **squashfs_device);
int sqmnt_lookup_all_mounts(FILE *ptr, char **mount_point,
                            char **overlay_upper_dir, char **overlay_lower_dir,
                            char **overlay_work_dir, char **loop_device);

int sqmnt_create_tmp_dirs(char **upper_dir, char **work_dir);
