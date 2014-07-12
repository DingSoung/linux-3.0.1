#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>

#include <linux/device.h>
#include <linux/io.h>

#define DEVICE_NAME "dsw248_GPIO"
#define IO_ADDRESS  0x7F008810

typedef int ElemType;

static volatile unsigned long *GPIO_CON;
static volatile unsigned long *GPIO_DAT;

static struct class *GPIO_CLASS;
static ElemType DEVICE_MAJOR=248;
static ElemType DEVICE_MINOR=0;

static ElemType dsw248_open(
    struct inode *inode,
    struct file *file)
{
    *GPIO_CON &= ~(0xffffffff);
    *GPIO_CON |= 0x11111111;
    return 0;
}
static ElemType dsw248_release(
    struct inode *inode,
    struct file *file)
{
    return 0;
}
static ElemType dsw248_read(
    struct file *file,
    char __user *buf,
    size_t count ,
    loff_t *f_pos )
{
    return count ;
}
static ElemType dsw248_write(
    struct file *file,
    const char __user *buf,
    size_t count ,
    loff_t *ppos )
{
    ElemType ker_buf;
    copy_from_user(&ker_buf, buf, 1);
    *GPIO_DAT &= ~(0xff);
    *GPIO_DAT |= ker_buf;
    return 0;
}

static struct file_operations dsw248_fops =
{
    . owner  = THIS_MODULE ,
    . open   = dsw248_open ,
    . release    = dsw248_release,
    . read   = dsw248_read ,
    . write  = dsw248_write ,
};

static ElemType __init dsw248_init (void)
{
    DEVICE_MAJOR = register_chrdev(0, DEVICE_NAME, &dsw248_fops);
    GPIO_CLASS = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(GPIO_CLASS, NULL, MKDEV(DEVICE_MAJOR, DEVICE_MINOR), NULL, DEVICE_NAME);

    GPIO_CON = (volatile unsigned long *)ioremap(IO_ADDRESS, 16);
    GPIO_DAT = GPIO_CON + 2;

    return 0;
}

static void __exit dsw248_exit (void)
{
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
    device_destroy(GPIO_CLASS,MKDEV(DEVICE_MAJOR,DEVICE_MINOR));
    class_destroy(GPIO_CLASS);
    iounmap(GPIO_CON);
}

module_init (dsw248_init);
module_exit (dsw248_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("dsw248");
MODULE_DESCRIPTION("contral GPL LOW 8 bit  Register");
MODULE_VERSION ("Beta");
