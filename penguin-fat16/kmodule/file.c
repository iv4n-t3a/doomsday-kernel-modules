#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/slab.h>

#include "fat16.h"
#include "inode.h"
#include "super.h"
#include "file.h"

static ssize_t penguin_fat16_read_iter(struct kiocb *iocb,
				        struct iov_iter *to)
{
  struct super_block *sb = iocb->ki_filp->f_inode->i_sb;
  fat16_sb_info_t* sbi = FAT16_SBI(sb);
  fat16_inode_info_t* ino = FAT16_INO(iocb->ki_filp->f_inode);

  void *buf = kmalloc(sbi->bytes_per_cluster, GFP_KERNEL);
  u16 cluster = ino->start_cluster;

  size_t bytes_requested = iov_iter_count(to);
  size_t bytes_read = 0;

  if (bytes_requested > ino->file_size - iocb->ki_pos)
    bytes_requested = ino->file_size - iocb->ki_pos;

  if (!buf)
    return -ENOMEM;

  while (bytes_read < bytes_requested && cluster < FAT16_FINAL_CLUSTER) {
    if (fat16_read_cluster(sb, cluster, buf)) {
      kfree(buf);
      return -EIO;
    }

    size_t bytes_requested_from_cluster = sbi->bytes_per_cluster;

    if (bytes_requested - bytes_read < bytes_requested_from_cluster)
      bytes_requested_from_cluster = bytes_requested - bytes_read;

    size_t bytes_read_from_cluster = copy_to_iter(buf, bytes_requested_from_cluster, to);

    if (bytes_read_from_cluster != bytes_requested_from_cluster) {
      kfree(buf);
      return -EIO;
    }

    bytes_read += bytes_read_from_cluster;

    if (fat16_next_cluster(sb, &cluster)) {
      kfree(buf);
      return -EIO;
    }
  }
  iocb->ki_pos += bytes_read;
  kfree(buf);
  return bytes_read;
}

static loff_t penguin_fat16_llseek(struct file *file, loff_t offset, int whence)
{
	return generic_file_llseek(file, offset, whence);
}

const struct file_operations penguin_fat16_file_ops = {
	.llseek    = penguin_fat16_llseek,
	.read_iter = penguin_fat16_read_iter,
};
