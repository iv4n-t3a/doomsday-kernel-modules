#ifndef PENGUIN_FAT16_SUPER_H
#define PENGUIN_FAT16_SUPER_H

#include <linux/fs.h>

#include "fat16.h"

typedef struct {
	u32 fat_start;
	u32 root_dir_start;
	u32 data_start;
	u32 sectors_per_cluster;
	u32 bytes_per_cluster;
	u32 root_dir_entries;
	u16 bytes_per_sector;
	u8  fat_count;
} fat16_sb_info_t;

static inline fat16_sb_info_t *FAT16_SBI(struct super_block *sb)
{
	return sb->s_fs_info;
}

extern const struct super_operations penguin_fat16_super_ops;

int penguin_fat16_fill_super(struct super_block *sb, void *data, int silent);

#endif // #ifndef PENGUIN_FAT16_SUPER_H
