#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include  <asm/delay.h>
#include <linux/time.h>
#define DEVICE_NAME "dsw248_HC_SR04"
#define IO_ADDRESS  0x7F008810
typedef int ElemType;
static char ker_buf[5];
static volatile unsigned long *GPIO_CON0;
static volatile unsigned long *GPIO_CON1;
static volatile unsigned long *GPIO_DAT;
static struct class *GPIO_CLASS;
static ElemType DEVICE_MAJOR=248;
static ElemType DEVICE_MINOR=0;
static DECLARE_WAIT_QUEUE_HEAD(INTERRUPT_WAIT);
static struct GPIO_DESC
{
    ElemType pin;
    char *name;
    ElemType val;
};
static struct GPIO_DESC GPIO_DESC[] =
{
    {0, "HC_SR04",1},
};
static struct timeval TRIGGER;
static irqreturn_t INTERRUPT_FUNCTION(int irq, void *dev_id)
{
    struct GPIO_DESC *pindesc = (struct GPIO_DESC *)dev_id;
    if (*GPIO_DAT & (1<<(pindesc->pin+8)))
    {
        do_gettimeofday(&TRIGGER);
        ker_buf[0]=1;
    }
    else
    {
        do_gettimeofday(&TRIGGER);
        ker_buf[0]=0;
    }
    wake_up_interruptible(&INTERRUPT_WAIT);
    return IRQ_HANDLED;
}
static ElemType dsw248_open(
    struct inode *inode,
    struct file *file)
{
    *GPIO_CON0 &= ~(0xFFF);
    *GPIO_CON0 |= 0x111;
    return 0;
}
static ElemType dsw248_release(
    struct inode * inode,
    struct file * file)
{
    return 0;
}
static ElemType dsw248_write(
    struct file *file,
    const char __user *buf,
    size_t count ,
    loff_t *ppos)
{
    copy_from_user(ker_buf, buf, count);
    *GPIO_DAT = (*GPIO_DAT&~(0x7))|ker_buf[0];
    udelay(10);
    *GPIO_DAT |= 0x7;
    return 1;
}
static ElemType dsw248_read(
    struct file *file,
    char __user *buf,
    size_t count ,
    loff_t *f_pos)
{
    wait_event_interruptible(INTERRUPT_WAIT,TRIGGER.tv_usec);
    ker_buf[1] =(int)TRIGGER.tv_sec;
    ker_buf[2] =(int)TRIGGER.tv_usec>>16;
    ker_buf[3] =(int)TRIGGER.tv_usec>>8;
    ker_buf[4] =(int)TRIGGER.tv_usec;
    copy_to_user(buf, ker_buf, count);
    TRIGGER.tv_sec = 0;
    TRIGGER.tv_usec = 0;
    return count;
}
static struct file_operations dsw248_fops =
{
    . owner  = THIS_MODULE ,
    . open   = dsw248_open ,
    .release=dsw248_release,
    . read   = dsw248_read ,
    . write  = dsw248_write ,
};
static ElemType __init dsw248_init(void)
{
    ElemType  ret;
    DEVICE_MAJOR = register_chrdev(0, DEVICE_NAME, &dsw248_fops);
    GPIO_CLASS = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(GPIO_CLASS, NULL, MKDEV(DEVICE_MAJOR, DEVICE_MINOR), NULL, DEVICE_NAME);
    GPIO_CON0 = (volatile unsigned long *)ioremap(IO_ADDRESS, 16);
    GPIO_CON1 =GPIO_CON0 + 1;
    GPIO_DAT = GPIO_CON0 + 2;
    ret = request_irq(IRQ_EINT(16),
                      INTERRUPT_FUNCTION,
                      IRQF_SHARED|IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
                      GPIO_DESC[0].name,
                      &GPIO_DESC[0]);
    return ret;
}
static void __exit dsw248_exit (void)
{
    free_irq(IRQ_EINT(16), &GPIO_DESC);
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
    device_destroy(GPIO_CLASS,MKDEV(DEVICE_MAJOR, DEVICE_MINOR));
    class_destroy(GPIO_CLASS);
    iounmap(GPIO_CON0);
}
module_init (dsw248_init);
module_exit (dsw248_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("dsw248");
MODULE_DESCRIPTION("Interrupt and HC_SR04 Driver,GPL0 Trig,GPL8 Echo");
MODULE_VERSION ("Beta");



