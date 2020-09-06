#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_overlay_mount_opt(struct mntent *cur_mount,
                                    const char *opt_name) {
  char *opt;
  if ((opt = hasmntopt(cur_mount, opt_name))) {
    if ((opt = strchr(opt, '='))) {
      if (opt[0] == '.') {
        // We cannot work with relative paths
        return NULL;
      }
      char *opt_end = strchr(opt, ',');
      if (opt_end) {
        return strndup(opt + 1, opt_end - opt - 1);
      }
      return strdup(opt + 1);
    }
  }
  return NULL;
}

static int match_overlay_mount_opts(struct mntent *cur_mount,
                                    const char *opt_name, const char *opt_val) {
  char *opt;
  if ((opt = hasmntopt(cur_mount, opt_name))) {
    if ((opt = strchr(opt, '='))) {
      char *opt_end = strchr(opt, ',');
      size_t opt_len;

      if (opt_end) {
        opt_len = opt_end - opt - 1;
      } else {
        opt_len = strlen(opt + 1);
      }

      if (strncmp(opt + 1, opt_val, opt_len) == 0) {
        return 1;
      }
    }
  }
  return 0;
}

int sqmnt_lookup_mount_mountpoint(const char *mountpoint,
                                  char **overlay_upper_dir,
                                  char **overlay_lower_dir,
                                  char **overlay_work_dir,
                                  char **squashfs_device) {

  struct mntent *cur_mount;
  FILE *proc_mounts_fp = setmntent("/proc/mounts", "r");

  *overlay_upper_dir = NULL;
  *overlay_lower_dir = NULL;
  *overlay_work_dir = NULL;
  *squashfs_device = NULL;

  // First: find the overlayfs entry
  while ((cur_mount = getmntent(proc_mounts_fp))) {
    if (strcmp(cur_mount->mnt_dir, mountpoint) == 0 &&
        strcmp(cur_mount->mnt_fsname, "overlay") == 0) {
      *overlay_lower_dir = read_overlay_mount_opt(cur_mount, "lowerdir");
      *overlay_upper_dir = read_overlay_mount_opt(cur_mount, "upperdir");
      *overlay_work_dir = read_overlay_mount_opt(cur_mount, "workdir");
      // TODO handle failure of read_overlay_opt() here
      break;
    }
  }

  if (!(*overlay_upper_dir && *overlay_lower_dir && *overlay_work_dir)) {
    goto fail;
  }

  // find the corresponding squashfs mount
  rewind(proc_mounts_fp);
  while ((cur_mount = getmntent(proc_mounts_fp))) {
    if (strcmp(cur_mount->mnt_dir, *overlay_lower_dir) == 0 &&
        strcmp(cur_mount->mnt_type, "squashfs") == 0) {
      *squashfs_device = strdup(cur_mount->mnt_fsname);
    }
  }

  if (*squashfs_device) {
    endmntent(proc_mounts_fp);
    return 0;
  }

  return 0;

fail:
  endmntent(proc_mounts_fp);
  free(*overlay_upper_dir);
  free(*overlay_lower_dir);
  free(*overlay_work_dir);
  return -1;
}

int sqmnt_lookup_all_mounts(FILE *ptr, char **mount_point,
                            char **overlay_upper_dir, char **overlay_lower_dir,
                            char **overlay_work_dir, char **loop_device) {
  *mount_point = NULL;
  *overlay_upper_dir = NULL;
  *overlay_lower_dir = NULL;
  *overlay_work_dir = NULL;
  *loop_device = NULL;
  fpos_t last_squashfs_pos;
  struct mntent *squashfs_mount;
  // iterate over all squashfs mounts
  while ((squashfs_mount = getmntent(ptr))) {
    if (strcmp(squashfs_mount->mnt_type, "squashfs") == 0) {
      *overlay_lower_dir = strdup(squashfs_mount->mnt_dir);
      *loop_device = strdup(squashfs_mount->mnt_fsname);
      // iterate over all other mounts looking for an overlay
      fgetpos(ptr, &last_squashfs_pos);
      struct mntent *overlay_mount;
      while ((overlay_mount = getmntent(ptr))) {
        if (strcmp(overlay_mount->mnt_type, "overlay") == 0 &&
            match_overlay_mount_opts(overlay_mount, "lowerdir",
                                     *overlay_lower_dir)) {
          *mount_point = strdup(overlay_mount->mnt_dir);
          *overlay_upper_dir =
              read_overlay_mount_opt(overlay_mount, "upperdir");
          *overlay_work_dir = read_overlay_mount_opt(overlay_mount, "workdir");

          break;
        }
      }

      if (*mount_point && *overlay_upper_dir && *overlay_lower_dir &&
          *overlay_work_dir) {
        return 1;
      } else {
        fsetpos(ptr, &last_squashfs_pos);
        free(*mount_point);
        free(*overlay_upper_dir);
        free(*overlay_lower_dir);
        free(*overlay_work_dir);
        free(*loop_device);

        *mount_point = NULL;
        *overlay_upper_dir = NULL;
        *overlay_lower_dir = NULL;
        *overlay_work_dir = NULL;
        *loop_device = NULL;
      }
    }
  }
  return 0;
}
