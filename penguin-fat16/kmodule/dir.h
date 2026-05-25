#ifndef PENGUIN_FAT16_DIR_H
#define PENGUIN_FAT16_DIR_H

#include <linux/fs.h>

#include "fat16.h"
#include "inode.h"

typedef int (*fat16_dirent_cb_t)(fat16_direntry_t *de,
				  u32 entry_sector, u32 entry_idx,
				  u32 abs_idx, void *cbdata);

int fat16_dir_walk(struct super_block *sb, const fat16_inode_info_t *fi,
		   fat16_dirent_cb_t cb, void *cbdata);

extern const struct file_operations penguin_fat16_dir_ops;

#endif  // #ifndef PENGUIN_FAT16_DIR_H
