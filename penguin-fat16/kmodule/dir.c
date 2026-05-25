#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "fat16.h"
#include "super.h"
#include "inode.h"
#include "dir.h"

int fat16_dir_walk(struct super_block *sb, const fat16_inode_info_t *fi,
		   fat16_dirent_cb_t cb, void *cbdata)
{
	fat16_sb_info_t *sbi = FAT16_SBI(sb);
	u32 abs_idx = 0;

	if (fi->start_cluster == 0) {
		u32 eps          = fat16_entries_per_sector(sbi->bytes_per_sector);
		u32 root_sectors = fat16_root_sectors(sbi->root_dir_entries, sbi->bytes_per_sector);

		for (u32 s = 0; s < root_sectors; s++) {
			struct buffer_head *bh = sb_bread(sb, sbi->root_dir_start + s);
			if (!bh)
				return -EIO;

			fat16_direntry_t *entries = (fat16_direntry_t *)bh->b_data;
			for (u32 i = 0; i < eps; i++, abs_idx++) {
				fat16_direntry_t *de = &entries[i];

				if (fat16_de_is_end(de)) {
					brelse(bh);
					return 0;
				}

				int ret = cb(de, sbi->root_dir_start + s, i, abs_idx, cbdata);
				if (ret) {
					brelse(bh);
					return ret;
				}
			}
			brelse(bh);
		}
	} else {
		u32 eps  = fat16_entries_per_sector(sbi->bytes_per_sector);
		u32 epc  = fat16_entries_per_cluster(sbi->bytes_per_cluster);
		u16 cluster = (u16)fi->start_cluster;
		void *buf = kmalloc(sbi->bytes_per_cluster, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		while (cluster < FAT16_FINAL_CLUSTER) {
			if (fat16_read_cluster(sb, cluster, buf)) {
				kfree(buf);
				return -EIO;
			}

			u32 cs = fat16_cluster_sector(sbi->data_start, cluster,
						      sbi->sectors_per_cluster);
			fat16_direntry_t *entries = (fat16_direntry_t *)buf;

			for (u32 i = 0; i < epc; i++, abs_idx++) {
				fat16_direntry_t *de = &entries[i];

				if (fat16_de_is_end(de)) {
					kfree(buf);
					return 0;
				}

				int ret = cb(de, fat16_entry_sector(cs, i, eps),
					     fat16_entry_idx(i, eps), abs_idx, cbdata);
				if (ret) {
					kfree(buf);
					return ret;
				}
			}

			if (fat16_next_cluster(sb, &cluster)) {
				kfree(buf);
				return -EIO;
			}
		}
		kfree(buf);
	}

	return 0;
}

struct iterate_cbdata {
	struct dir_context *ctx;
	struct super_block *sb;
	loff_t start_idx;
};

static int iterate_cb(fat16_direntry_t *de, u32 entry_sector, u32 entry_idx,
		      u32 abs_idx, void *data)
{
	struct iterate_cbdata *ic = data;
	char namebuf[FAT16_FILENAME_LEN + 2];

	if ((loff_t)abs_idx < ic->start_idx)
		return 0;

	if (fat16_de_is_skip(de)) {
		ic->ctx->pos++;
		return 0;
	}

	fat16_83_to_str(de->filename, namebuf, sizeof(namebuf));
	unsigned long ino = fat16_make_ino(ic->sb, entry_sector, entry_idx);
	unsigned char d_type = fat16_de_is_dir(de) ? DT_DIR : DT_REG;

	if (!dir_emit(ic->ctx, namebuf, strlen(namebuf), ino, d_type))
		return 1;
	ic->ctx->pos++;
	return 0;
}

static int penguin_fat16_iterate_shared(struct file *file,
					 struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	fat16_inode_info_t *fi = FAT16_INO(inode);

	if (!dir_emit_dots(file, ctx))
		return 0;

	struct iterate_cbdata cbdata = {
		.ctx       = ctx,
		.sb        = inode->i_sb,
		.start_idx = ctx->pos - 2,
	};

	int ret = fat16_dir_walk(inode->i_sb, fi, iterate_cb, &cbdata);
	return ret < 0 ? ret : 0;
}

const struct file_operations penguin_fat16_dir_ops = {
	.llseek         = generic_file_llseek,
	.iterate_shared = penguin_fat16_iterate_shared,
};
