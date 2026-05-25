#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/statfs.h>

#include "fat16.h"
#include "super.h"
#include "inode.h"

static void penguin_fat16_put_super(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	sb->s_fs_info = NULL;
}

static void penguin_fat16_evict_inode(struct inode *inode)
{
    kfree(inode->i_private);
    inode->i_private = NULL;
    clear_inode(inode);
}

const struct super_operations penguin_fat16_super_ops = {
  .evict_inode = penguin_fat16_evict_inode,
	.put_super = penguin_fat16_put_super,
};

int penguin_fat16_fill_super(struct super_block *sb, void *data, int silent)
{
	fat16_sb_info_t *sbi;
	struct buffer_head *bh;
	fat16_bpb_t *bpb;
	struct inode *root_inode;
	int ret = -EINVAL;

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	sb->s_fs_info = sbi;

	if (!sb_set_blocksize(sb, 512)) {
		pr_err("penguin_fat16: failed to set block size\n");
		ret = -EINVAL;
		goto err_free_sbi;
	}

	bh = sb_bread(sb, 0);
	if (!bh) {
		pr_err("penguin_fat16: cannot read VBR\n");
		ret = -EIO;
		goto err_free_sbi;
	}

	bpb = (fat16_bpb_t *)bh->b_data;

	sbi->bytes_per_sector    = le16_to_cpu(bpb->bytes_per_sector);
	sbi->sectors_per_cluster = bpb->sectors_per_cluster;
	sbi->bytes_per_cluster   = fat16_bytes_per_cluster(bpb);
	sbi->fat_count           = bpb->FAT_count;
	sbi->fat_start           = fat16_fat_sector(bpb);
	sbi->root_dir_start      = fat16_root_dir_sector(bpb);
	sbi->root_dir_entries    = le16_to_cpu(bpb->root_dir_entries);
	sbi->data_start          = fat16_data_sector(bpb);

	brelse(bh);

	sb->s_magic     = fat16_fs_magic;
	sb->s_op        = &penguin_fat16_super_ops;
	sb->s_flags    |= SB_RDONLY;
	sb->s_blocksize      = sbi->bytes_per_sector;
	sb->s_blocksize_bits = ilog2(sbi->bytes_per_sector);

	root_inode = penguin_fat16_get_root_inode(sb);
	if (!root_inode) {
		ret = -ENOMEM;
		goto err_free_sbi;
	}

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root) {
		ret = -ENOMEM;
		goto err_free_sbi;
	}

	return 0;

err_free_sbi:
	kfree(sbi);
	sb->s_fs_info = NULL;
	return ret;
}
