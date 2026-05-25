#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/buffer_head.h>

#include "fat16.h"
#include "super.h"
#include "inode.h"
#include "dir.h"
#include "file.h"

unsigned long fat16_make_ino(struct super_block *sb,
			      u32 sector, u32 entry_index)
{
	return (unsigned long)sector * FAT16_SBI(sb)->bytes_per_sector
	       + entry_index * sizeof(fat16_direntry_t);
}

struct inode *penguin_fat16_iget(struct super_block *sb,
				  unsigned long ino,
				  const fat16_direntry_t *de)
{
	struct inode *inode;
	fat16_inode_info_t *fi;

	inode = new_inode(sb);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	fi = kzalloc(sizeof(*fi), GFP_KERNEL);
	if (!fi) {
		iput(inode);
		return ERR_PTR(-ENOMEM);
	}

	fi->start_cluster = le16_to_cpu(de->first_cluster_low);
	fi->file_size     = le32_to_cpu(de->file_size);
	inode->i_private  = fi;

	inode->i_ino   = ino;
	inode->i_size  = fi->file_size;
	inode->i_sb    = sb;
	set_nlink(inode, 1);

	if (fat16_de_is_dir(de)) {
		inode->i_mode = S_IFDIR | 0555;
		inode->i_op   = &penguin_fat16_dir_inode_ops;
		inode->i_fop  = &penguin_fat16_dir_ops;
	} else {
		inode->i_mode = S_IFREG | 0444;
		inode->i_op   = &penguin_fat16_inode_ops;
		inode->i_fop  = &penguin_fat16_file_ops;
	}

	insert_inode_hash(inode);
	return inode;
}

struct inode *penguin_fat16_get_root_inode(struct super_block *sb)
{
	fat16_direntry_t fake = {};

	fake.attr              = FAT16_ATTR_DIRECTORY;
	fake.first_cluster_low = 0;
	fake.file_size         = 0;

	return penguin_fat16_iget(sb, 1, &fake);
}

struct lookup_cbdata {
	struct super_block *sb;
	const char *name;
	unsigned int namelen;
	struct dentry *dentry;
	struct dentry *result;
};

static int lookup_cb(fat16_direntry_t *de, u32 entry_sector, u32 entry_idx,
		     u32 abs_idx, void *data)
{
	struct lookup_cbdata *lc = data;
	char namebuf[FAT16_FILENAME_LEN + 2];

	if (fat16_de_is_skip(de))
		return 0;

	fat16_83_to_str(de->filename, namebuf, sizeof(namebuf));
	if (strlen(namebuf) != lc->namelen ||
	    strncasecmp(namebuf, lc->name, lc->namelen) != 0)
		return 0;

	unsigned long ino = fat16_make_ino(lc->sb, entry_sector, entry_idx);
	struct inode *inode = penguin_fat16_iget(lc->sb, ino, de);
	lc->result = IS_ERR(inode) ? ERR_CAST(inode) : d_splice_alias(inode, lc->dentry);
	return 1;
}

static struct dentry *penguin_fat16_lookup(struct inode *inode,
					    struct dentry *dentry,
					    unsigned int flags)
{
	fat16_inode_info_t *fi = FAT16_INO(inode);
	struct lookup_cbdata cbdata = {
		.sb      = inode->i_sb,
		.name    = dentry->d_name.name,
		.namelen = dentry->d_name.len,
		.dentry  = dentry,
		.result  = NULL,
	};

	int ret = fat16_dir_walk(inode->i_sb, fi, lookup_cb, &cbdata);
	if (ret < 0)
		return ERR_PTR(ret);
	if (cbdata.result)
		return cbdata.result;

	d_add(dentry, NULL);
	return NULL;
}

static int penguin_fat16_getattr(struct mnt_idmap *idmap,
				  const struct path *path,
				  struct kstat *stat,
				  u32 request_mask,
				  unsigned int query_flags)
{
	generic_fillattr(idmap, request_mask, d_inode(path->dentry), stat);
	return 0;
}

const struct inode_operations penguin_fat16_inode_ops = {
	.getattr = penguin_fat16_getattr,
};

const struct inode_operations penguin_fat16_dir_inode_ops = {
	.lookup  = penguin_fat16_lookup,
	.getattr = penguin_fat16_getattr,
};
