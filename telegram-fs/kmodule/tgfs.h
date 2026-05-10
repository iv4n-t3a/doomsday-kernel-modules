#ifndef TGFS_H
#define TGFS_H

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "tgfs"
#define MAX_CHATS   10
#define BUS_QUEUE_SIZE 32
#define BUS_MESSAGE_SIZE 128
#define MSG_BUFF_SIZE 4096
#define CHAT_NAME_SIZE 32
#define BUS_BUFF_SIZE 4096
#define CHAT_ID_BUFF_SIZE 64

struct chat_dev {
    struct cdev           cdev;
    struct mutex          lock;
    wait_queue_head_t     waitq;
    char                  name[CHAT_NAME_SIZE];
    char                  buffer[MSG_BUFF_SIZE];
    size_t                buf_len;
    char                  chat_id[CHAT_ID_BUFF_SIZE];
};

struct bus_message {
    char payload[BUS_MESSAGE_SIZE];
    size_t len;
};

struct tg_bus_dev {
    struct cdev        cdev;
    struct mutex       lock;
    struct bus_message queue[BUS_QUEUE_SIZE];
    wait_queue_head_t  waitq;
    size_t             count;
    size_t             head;
    size_t             tail;
    size_t             write_pos;
};

extern struct chat_dev   chats[MAX_CHATS];
extern struct class     *telegram_class;
extern dev_t             chats_dev;

extern struct tg_bus_dev tg_bus;
extern dev_t             bus_dev;

extern const struct file_operations bus_fops;
extern const struct file_operations chat_fops;

void bus_enqueue_outgoing(int chat_idx, const char *msg, size_t msg_len);

#endif
