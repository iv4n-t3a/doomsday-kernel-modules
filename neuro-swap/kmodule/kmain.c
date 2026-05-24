#if !__KERNEL__
#error "You're trying to compile module implementation for user-space"
#endif

#include "config.h"

#include "inference.h"

#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

static unsigned long nswap_size_mb = DEFAULT_SWAP_SIZE_MB;
module_param(nswap_size_mb, ulong, 0444);
MODULE_PARM_DESC(nswap_size_mb, "Size of nswap device in MiB (default 32)");

static struct nswap_block_dev {
  struct gendisk *gd;
  size_t nr_sectors;
  spinlock_t lock;
  struct blk_mq_tag_set tag_set;
  struct request_queue *queue;

  // Sure, this is a good idea!
  LSTMState *lstm;
} dev;

static blk_status_t nswap_queue_rq(struct blk_mq_hw_ctx *hctx,
                                   const struct blk_mq_queue_data *bd) {
  struct request *rq = bd->rq;
  struct nswap_block_dev *dev = rq->q->queuedata;
  sector_t sector = blk_rq_pos(rq);
  size_t offset = sector << SECTOR_SHIFT;
  size_t len = blk_rq_bytes(rq);
  struct bio_vec bvec;
  struct req_iterator iter;

  if (offset + len > dev->nr_sectors << SECTOR_SHIFT) {
    blk_mq_end_request(rq, BLK_STS_IOERR);
    return BLK_STS_IOERR;
  }

  rq_for_each_segment(bvec, rq, iter) {
    size_t seg_len = bvec.bv_len;

    if (rq_data_dir(rq) == WRITE) {
      void *src;
      src = bvec_kmap_local(&bvec);
      lstm_memorize(dev->lstm, (uint8_t *)src, seg_len);
    } else {
      void *dst;
      dst = bvec_kmap_local(&bvec);
      lstm_recall(dev->lstm, (uint8_t *)dst, seg_len);
    }
  }

  blk_mq_end_request(rq, BLK_STS_OK);
  return BLK_STS_OK;
}

static const struct blk_mq_ops nswap_mq_ops = {.queue_rq = nswap_queue_rq};

static const struct block_device_operations nswap_ops = {
    .owner = THIS_MODULE,
};

static int __init nswap_init(void) {
  int err;

  if (nswap_size_mb == 0) {
    printk(KERN_ERR "nswap: size must be at least 1 MiB\n");
    return -EINVAL;
  }
  dev.nr_sectors = nswap_size_mb << 11;

  int status = register_blkdev(NEUROSWAP_BLOCK_MAJOR, NEUROSWAP_BLOCKDEV_NAME);
  if (status < 0) {
    printk(KERN_ERR "nswap: unable to register nswap device\n");
    return -EBUSY;
  }

  dev.lstm = (LSTMState *)vmalloc(sizeof(LSTMState));
  if (!dev.lstm) {
    printk(KERN_ERR "nswap: failed to allocate lstm state\n");
    err = -ENOMEM;
    goto err_out_unregister;
  }
  lstm_state_reset(dev.lstm);

  spin_lock_init(&dev.lock);

  dev.tag_set.ops = &nswap_mq_ops;
  dev.tag_set.nr_hw_queues = 1;
  dev.tag_set.queue_depth = BLKDEV_QUEUE_DEPTH;
  dev.tag_set.numa_node = NUMA_NO_NODE;
  dev.tag_set.cmd_size = 0;
  dev.tag_set.flags = 0;
  err = blk_mq_alloc_tag_set(&dev.tag_set);
  if (err) {
    printk(KERN_ERR "nswap: failed to alloc tag set, err=%d\n", err);
    goto err_out_free_buffer;
  }

  struct queue_limits limits = {
      .logical_block_size = LOGICAL_BLOCK_SIZE,
      .physical_block_size = PHYSICAL_BLOCK_SIZE,
      .max_hw_sectors = MAX_HW_SECTORS,
  };

  dev.gd = blk_mq_alloc_disk(&dev.tag_set, &limits, &dev);
  if (IS_ERR(dev.gd)) {
    err = PTR_ERR(dev.gd);
    pr_err("nswap: failed to allocate disk, err=%d\n", err);
    goto err_out_free_tag_set;
  }
  dev.queue = dev.gd->queue;

  if (!dev.gd) {
    printk(KERN_ERR "nswap: failed to alloc disk");
    err = -ENOMEM;
    goto err_out_destroy_queue;
  }

  dev.gd->major = NEUROSWAP_BLOCK_MAJOR;
  dev.gd->first_minor = 0;
  dev.gd->minors = 1;
  snprintf(dev.gd->disk_name, DISK_NAME_LEN, "%s", NEUROSWAP_BLOCKDEV_NAME);
  dev.gd->fops = &nswap_ops;
  dev.gd->private_data = &dev;
  dev.gd->queue = dev.queue;
  set_capacity(dev.gd, dev.nr_sectors);

  err = add_disk(dev.gd);
  if (err) {
    printk(KERN_ERR "nswap: failed to add disk, err=%d\n", err);
    goto err_out_put_disk;
  }

  printk(KERN_INFO "nswap: device successfully registered!");
  return 0;

err_out_put_disk:
  put_disk(dev.gd);

err_out_destroy_queue:
  blk_mq_destroy_queue(dev.queue);

err_out_free_tag_set:
  blk_mq_free_tag_set(&dev.tag_set);

err_out_free_buffer:
  vfree((void *)dev.lstm);

err_out_unregister:
  unregister_blkdev(NEUROSWAP_BLOCK_MAJOR, NEUROSWAP_BLOCKDEV_NAME);
  return err;
}

static void __exit nswap_exit(void) {
  if (dev.gd) {
    del_gendisk(dev.gd);
    put_disk(dev.gd);
  }

  blk_mq_free_tag_set(&dev.tag_set);
  vfree((void *)dev.lstm);
  unregister_blkdev(NEUROSWAP_BLOCK_MAJOR, NEUROSWAP_BLOCKDEV_NAME);
  printk(KERN_INFO "nswap: device unregistered\n");
}

module_init(nswap_init);
module_exit(nswap_exit);
MODULE_LICENSE("GPL");
