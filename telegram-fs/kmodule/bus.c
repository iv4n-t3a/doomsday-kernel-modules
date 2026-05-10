#include "tgfs.h"

void bus_enqueue_outgoing(int chat_idx, const char *msg, size_t msg_len)
{
    struct bus_message *bm;

    mutex_lock(&tg_bus.lock);

    if (tg_bus.count >= BUS_QUEUE_SIZE) {
        pr_warn("telegram: bus queue full, dropping message\n");
        goto out;
    }

    bm = &tg_bus.queue[tg_bus.tail];
    bm->len = scnprintf(bm->payload, BUS_MESSAGE_SIZE,
                        "OUT|%d|%.*s\n", chat_idx, (int)msg_len, msg);

    tg_bus.tail = (tg_bus.tail + 1) % BUS_QUEUE_SIZE;
    tg_bus.count++;

    wake_up_interruptible(&tg_bus.waitq);
out:
    mutex_unlock(&tg_bus.lock);
}

static int bus_open(struct inode *inode, struct file *flip)
{
    flip->private_data = &tg_bus;
    pr_info("telegram: bus device opened by daemon pid=%d\n", current->pid);
    return 0;
}

static ssize_t bus_read(struct file *flip, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    struct tg_bus_dev *bus = flip->private_data;
    struct bus_message *bm;
    ssize_t ret;

    mutex_lock(&bus->lock);

    while (bus->count == 0) {
        mutex_unlock(&bus->lock);

        if (flip->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        if (wait_event_interruptible(bus->waitq, bus->count > 0)) {
            return -ERESTARTSYS;
        }

        mutex_lock(&bus->lock);
    }

    bm = &bus->queue[bus->head];

    if (count < bm->len) {
        mutex_unlock(&bus->lock);
        return -EMSGSIZE;
    }

    if (copy_to_user(ubuf, bm->payload, bm->len)) {
        mutex_unlock(&bus->lock);
        return -EFAULT;
    }

    ret = bm->len;
    bus->head = (bus->head + 1) % BUS_QUEUE_SIZE;
    bus->count--;

    mutex_unlock(&bus->lock);
    return ret;
}

static ssize_t bus_write(struct file *flip, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char tmp[BUS_MESSAGE_SIZE];
    char *p, *cmd, *idx_str, *arg1, *arg2;
    int chat_idx;
    struct chat_dev *chat;

    if (count >= sizeof(tmp)) {
        return -EINVAL;
    }

    if (copy_from_user(tmp, ubuf, count)) {
        return -EFAULT;
    }

    tmp[count] = '\0';

    p       = tmp;
    cmd     = strsep(&p, "|");
    idx_str = strsep(&p, "|");

    if (!cmd || !idx_str) {
        return -EINVAL;
    }

    if (kstrtoint(idx_str, 10, &chat_idx) ||
        chat_idx < 0 || chat_idx >= MAX_CHATS) {
        return -EINVAL;
    }

    chat = &chats[chat_idx];

    if (strcmp(cmd, "IN") == 0) {
        /* IN|<idx>|<sender>|<message> */
        arg1 = strsep(&p, "|"); /* sender */
        arg2 = p;               /* message (rest of string) */

        if (!arg1 || !arg2)
            return -EINVAL;

        mutex_lock(&chat->lock);
        chat->buf_len += scnprintf(chat->buffer + chat->buf_len,
                                   MSG_BUFF_SIZE - chat->buf_len,
                                   "[%s] %s\n", arg1, arg2);
        mutex_unlock(&chat->lock);

        wake_up_interruptible(&chat->waitq);

    } else if (strcmp(cmd, "SET") == 0) {
        /* SET|<idx>|<telegram_chat_id> */
        arg1 = p; /* telegram chat id */
        if (!arg1) {
            return -EINVAL;
        }

        mutex_lock(&chat->lock);
        strscpy(chat->chat_id, arg1, CHAT_ID_BUFF_SIZE);
        mutex_unlock(&chat->lock);

        pr_info("telegram: chat_%d bound to Telegram ID %s\n",
                chat_idx + 1, arg1);
    } else {
        return -EINVAL;
    }

    return count;
}

static __poll_t bus_poll(struct file *flip, poll_table *wait)
{
    struct tg_bus_dev *bus = flip->private_data;
    __poll_t mask = EPOLLOUT | EPOLLWRNORM;

    poll_wait(flip, &bus->waitq, wait);

    mutex_lock(&bus->lock);
    if (bus->count > 0) {
        mask |= EPOLLIN | EPOLLRDNORM;
    }
    mutex_unlock(&bus->lock);

    return mask;
}

const struct file_operations bus_fops = {
    .owner   = THIS_MODULE,
    .open    = bus_open,
    .read    = bus_read,
    .write   = bus_write,
    .poll    = bus_poll,
};
