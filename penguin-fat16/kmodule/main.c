#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>

#include "super.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("iv4n-t3a");
MODULE_DESCRIPTION("FAT16 filesystem driver ported from PenguinOS bootloader");

static struct dentry *penguin_fat16_mount(struct file_system_type *fs_type,
					   int flags,
					   const char *dev_name,
					   void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data,
			  penguin_fat16_fill_super);
}

static struct file_system_type penguin_fat16_fs_type = {
	.name     = "penguin_fat16",
	.fs_flags = FS_REQUIRES_DEV,
	.mount    = penguin_fat16_mount,
	.kill_sb  = kill_block_super,
	.owner    = THIS_MODULE,
};

static int __init penguin_fat16_init(void)
{
	int ret = register_filesystem(&penguin_fat16_fs_type);

	if (ret)
		pr_err("penguin_fat16: failed to register filesystem: %d\n", ret);
	else
		pr_info("penguin_fat16: registered\n");

	return ret;
}

static void __exit penguin_fat16_exit(void)
{
	unregister_filesystem(&penguin_fat16_fs_type);
	pr_info("penguin_fat16: unregistered\n");
}

module_init(penguin_fat16_init);
module_exit(penguin_fat16_exit);
