#include "tgfs.h"

static int chat_open(struct inode *inode, struct file *flip) {
    struct chat_dev *chat;
    chat = container_of(inode->i_cdev, struct chat_dev, cdev);
    flip->private_data = chat;
    pr_info("telegram: opened chat %s\n", chat->name);
    return 0;
}

static ssize_t chat_read(struct file *flip, char __user *ubuf, size_t count, loff_t *ppos) {
    struct chat_dev *chat = flip->private_data;
    ssize_t ret;

    mutex_lock(&chat->lock);

    while (*ppos >= chat->buf_len) {
        mutex_unlock(&chat->lock);

        if (flip->f_flags & O_NONBLOCK)
            return -EAGAIN;

        if (wait_event_interruptible(chat->waitq, *ppos < chat->buf_len))
            return -ERESTARTSYS;

        mutex_lock(&chat->lock);
    }

    count = min(count, (size_t)(chat->buf_len - *ppos));

    if (copy_to_user(ubuf, chat->buffer + *ppos, count)) {
        ret = -EFAULT;
        goto out;
    }

    *ppos += count;
    ret = count;

out:
    mutex_unlock(&chat->lock);
    return ret;
}

static ssize_t chat_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *ppos) {
    struct chat_dev *chat = flip->private_data;
    char tmp[MSG_BUFF_SIZE];
    ssize_t ret;
    int chat_idx;

    if (count >= sizeof(tmp)) {
        return -EINVAL;
    }

    if (copy_from_user(tmp, ubuf, count)) {
        return -EFAULT;
    }

    tmp[count] = '\0';

    mutex_lock(&chat->lock);

    ret = scnprintf(chat->buffer + chat->buf_len,
                    MSG_BUFF_SIZE - chat->buf_len,
                    "[User] %s\n", tmp);
    chat->buf_len += ret;

    mutex_unlock(&chat->lock);

    wake_up_interruptible(&chat->waitq);

    chat_idx = (int)(chat - chats);
    bus_enqueue_outgoing(chat_idx, tmp, count);

    pr_info("telegram: message sent to %s\n", chat->name);
    return count;
}

static int chat_release(struct inode *inode, struct file *flip)
{
    struct chat_dev *chat = flip->private_data;
    pr_info("telegram: closed chat %s\n", chat->name);
    return 0;
}

static __poll_t chat_poll(struct file *flip, poll_table *wait)
{
    struct chat_dev *chat = flip->private_data;
    __poll_t mask = EPOLLOUT | EPOLLWRNORM;

    poll_wait(flip, &chat->waitq, wait);

    mutex_lock(&chat->lock);
    if (flip->f_pos < chat->buf_len)
        mask |= EPOLLIN | EPOLLRDNORM;
    mutex_unlock(&chat->lock);

    return mask;
}

const struct file_operations chat_fops = {
    .owner   = THIS_MODULE,
    .open    = chat_open,
    .read    = chat_read,
    .write   = chat_write,
    .poll    = chat_poll,
    .release = chat_release,
};
