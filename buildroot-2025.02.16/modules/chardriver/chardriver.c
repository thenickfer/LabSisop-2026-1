#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

#define DEVICE_NAME "chardrv"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Estudante");
MODULE_DESCRIPTION("Character Device Driver");
MODULE_VERSION("0.0.1");

static ssize_t chardrv_read   (struct  file *,       char __user *, size_t, loff_t *);
static ssize_t chardrv_write  (struct  file *, const char __user *, size_t, loff_t *);
static int     chardrv_open   (struct inode *, struct file *);
static int     chardrv_release(struct inode *, struct file *);

static struct file_operations fops =
{
    .read    = chardrv_read,
    .write   = chardrv_write,
    .open    = chardrv_open,
    .release = chardrv_release
};

static int major;


static struct class *cls = NULL;
static struct device *dev = NULL;

static int chardrv_init(void)
{
    pr_info("Inserting the Character Device\n");

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("Character Device failed to register a major number\n");
        return major;
    }
    pr_info("Character Device registered major %d\n", major);

    cls = class_create(DEVICE_NAME);
    if (IS_ERR(cls)) {
        pr_err("Character Device failed to register device class\n");
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(cls);
    }
    pr_info("Character Device registered class\n");

    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev)) {
        pr_err("Character Device failed to create device\n");
        class_destroy(cls);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(dev);
    }
    pr_info("Character Device registered device\n");

    return 0;
}

static void chardrv_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Removed the Character Device\n");
}

static int number_opens = 0;

static int chardrv_open(struct inode *inodep, struct file *filep)
{
    pr_info("Device has been opened %d time(s)\n", ++number_opens);
    return 0;
}

static int chardrv_release(struct inode *inodep, struct file *filep)
{
    pr_info("Device successfully closed\n");
    return 0;
}

static char *message = NULL;
static size_t size_of_message = 0;

static unsigned int max_size = 256;
module_param(max_size, uint, 0444);
MODULE_PARM_DESC(max_size, "Tamanho maximo de cada mensagem, em bytes");

static ssize_t chardrv_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    if (len > max_size) {
        pr_alert("Message too long (%zu > %u)\n", len, max_size);
        return -EINVAL;
    }

    kfree(message);
    size_of_message = 0;

    message = kmalloc(len + 1, GFP_KERNEL);
    if (message == NULL) {
        pr_alert("Failed to allocate memory for the message\n");
        return -ENOMEM;
    }

    if (copy_from_user(message, buffer, len) != 0) {
        pr_alert("Failed to receive the message from the user\n");
        kfree(message);
        message = NULL;
        return -EFAULT;
    }

    message[len] = '\0';
    size_of_message = len;

    pr_info("Allocated %zu bytes for a message with %zu characters\n", len + 1, len);
    return len;
}

static ssize_t chardrv_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    pr_info("Sending up to %zu characters to the user\n", len);
    return simple_read_from_buffer(buffer, len, offset, message, size_of_message);
}

static void chardrv_exit(void)
{
    kfree(message);

    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Removed the Character Device\n");
}

module_init(chardrv_init);
module_exit(chardrv_exit);
