#ifndef PENGUIN_FAT16_INODE_H
#define PENGUIN_FAT16_INODE_H

#include <linux/fs.h>

#include "fat16.h"

typedef struct {
	u32 start_cluster;
	u32 file_size;
} fat16_inode_info_t;

static inline fat16_inode_info_t *FAT16_INO(struct inode *inode)
{
	return inode->i_private;
}

extern const struct inode_operations penguin_fat16_inode_ops;
extern const struct inode_operations penguin_fat16_dir_inode_ops;

unsigned long fat16_make_ino(struct super_block *sb, u32 sector, u32 entry_index);

struct inode *penguin_fat16_iget(struct super_block *sb,
				  unsigned long ino,
				  const fat16_direntry_t *de);

struct inode *penguin_fat16_get_root_inode(struct super_block *sb);

#endif  // #ifndef PENGUIN_FAT16_INODE_H
