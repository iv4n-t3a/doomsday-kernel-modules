#include "tgfs.h"

struct chat_dev   chats[MAX_CHATS];
struct class     *telegram_class;
dev_t             chats_dev;

struct tg_bus_dev tg_bus;
dev_t             bus_dev;

static char *telegram_devnode(const struct device *dev, umode_t *mode)
{
    if (mode) {
        *mode = 0666;
    }
    return NULL;
}

static int __init telegram_init(void)
{
    int i, ret;

    ret = alloc_chrdev_region(&chats_dev, 0, MAX_CHATS, DEVICE_NAME);
    if (ret < 0) {
        return ret;
    }

    telegram_class = class_create(DEVICE_NAME);
    if (IS_ERR(telegram_class)) {
        unregister_chrdev_region(chats_dev, MAX_CHATS);
        return PTR_ERR(telegram_class);
    }

    telegram_class->devnode = telegram_devnode;

    for (i = 0; i < MAX_CHATS; i++) {
        struct chat_dev *chat = &chats[i];

        snprintf(chat->name, sizeof(chat->name), "chat_%d", i + 1);
        mutex_init(&chat->lock);
        init_waitqueue_head(&chat->waitq);

        chat->buf_len = 0;

        cdev_init(&chat->cdev, &chat_fops);
        chat->cdev.owner = THIS_MODULE;
        cdev_add(&chat->cdev, MKDEV(MAJOR(chats_dev), i), 1);

        device_create(telegram_class, NULL,
                      MKDEV(MAJOR(chats_dev), i),
                      NULL, "telegram/chat_%d", i + 1);
    }
    pr_info("telegram: device initialized\n");

    mutex_init(&tg_bus.lock);
    init_waitqueue_head(&tg_bus.waitq);

    ret = alloc_chrdev_region(&bus_dev, 0, 1, "telegram_bus");
    if (ret < 0) {
        goto err_bus_region;
    }

    cdev_init(&tg_bus.cdev, &bus_fops);
    tg_bus.cdev.owner = THIS_MODULE;
    ret = cdev_add(&tg_bus.cdev, bus_dev, 1);

    if (ret < 0) {
        goto err_bus_cdev;
    }

    device_create(telegram_class, NULL, bus_dev, NULL, "telegram/bus");

    pr_info("telegram: module loaded\n");
    return 0;

err_bus_cdev:
    unregister_chrdev_region(bus_dev, 1);

err_bus_region:
    for (i = 0; i < MAX_CHATS; i++) {
        device_destroy(telegram_class, MKDEV(MAJOR(chats_dev), i));
        cdev_del(&chats[i].cdev);
    }
    class_destroy(telegram_class);
    unregister_chrdev_region(chats_dev, MAX_CHATS);
    return ret;
}

static void __exit telegram_exit(void)
{
    int i;

    device_destroy(telegram_class, bus_dev);
    cdev_del(&tg_bus.cdev);
    unregister_chrdev_region(bus_dev, 1);

    for (i = 0; i < MAX_CHATS; i++) {
        device_destroy(telegram_class, MKDEV(MAJOR(chats_dev), i));
        cdev_del(&chats[i].cdev);
    }

    class_destroy(telegram_class);
    unregister_chrdev_region(chats_dev, MAX_CHATS);
    pr_info("telegram: module unloaded\n");
}

module_init(telegram_init);
module_exit(telegram_exit);
MODULE_LICENSE("GPL");
