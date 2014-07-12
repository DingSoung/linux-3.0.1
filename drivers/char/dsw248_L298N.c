#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <mach/map.h>
#include <mach/gpio-bank-m.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <plat/regs-timer.h>
#include <linux/clk.h>
#define DEVICE_NAME "dsw248_L298N"
#define IO_ADDRESS  0x7F008810
#include <mach/map.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-bank-f.h>
typedef int ElemType;
static volatile unsigned long *GPIO_CON ;
static volatile unsigned long *GPIO_DAT ;
static struct class *GPIO_CLASS;
static ElemType DEVICE_MAJOR=248;
static ElemType DEVICE_MINOR=0;

static struct PWM_PORT
{
    int number;
    char *name;
};
static struct PWM_PORT PWM_PORT[] =
{
    {0,"PortClassA"},
    {1,"PortClassB"},
};
static ElemType IOPWM[]= {3,1};
static ElemType Level_Flag=0;
static unsigned long tcnt;
static irqreturn_t INTERRUPT_FUNCTION(int irq, void *dev_id)
{
    switch(IOPWM[0])
    {
    case 0:
        *GPIO_CON &= ~(0xFF<<20);
        *GPIO_CON |= (0x1<<20);
        break;
    case 1:
        *GPIO_CON &= ~(0xFF<<20);
        *GPIO_CON |= (0x10<<20);
        break;
    case 2:
        *GPIO_CON |= (0x11<<20);
        break;
    case 3:
        *GPIO_CON &= ~(0xFF<<20);
        break;
    default:
        *GPIO_CON &= ~(0xFF<<20);
        break;
    }
    switch(Level_Flag)
    {
    case 0:
        __raw_writel(IOPWM[1], S3C_TCNTB(1));
        *GPIO_DAT &= ~(0x3<<5);
        Level_Flag=1;
        break;

    case 1:
        __raw_writel(tcnt-IOPWM[1], S3C_TCNTB(1));
        *GPIO_DAT |= (0x3<<5);
        Level_Flag=0;
        break;

    default:
        __raw_writel(tcnt, S3C_TCNTB(1));
        *GPIO_DAT &= ~(0x3<<5);
        Level_Flag=0;
        break;
    }
    return IRQ_RETVAL(IRQ_HANDLED);
}
static void PWM_Start(unsigned long FREQ,unsigned long Duty)
{
    unsigned long tcfg0,tcfg1,tcon,cstat,pclk;
    struct clk *clk_p;
    tcfg1 = __raw_readl(S3C_TCFG1);
    tcfg0 = __raw_readl(S3C_TCFG0);
    tcon = __raw_readl(S3C_TCON);
    tcfg0 &= ~S3C_TCFG_PRESCALER0_MASK;
    tcfg0 |= (66 - 1);
    tcfg1 &= ~S3C_TCFG1_MUX1_MASK;
    tcfg1 |= S3C_TCFG1_MUX1_DIV4;
    __raw_writel(tcfg1, S3C_TCFG1);
    __raw_writel(tcfg0, S3C_TCFG0);
    clk_p = clk_get(NULL, "pclk");
    pclk = clk_get_rate(clk_p);
    tcnt = (pclk/66/4)/FREQ;
    __raw_writel(tcnt, S3C_TCNTB(1));
    __raw_writel(Duty, S3C_TCMPB(1));
    cstat = __raw_readl(S3C_TINT_CSTAT);
    cstat |=(0x1<<1);
    __raw_writel(cstat,S3C_TINT_CSTAT);
    tcon &= ~(0xF<<8);
    tcon |= (0xB<<8);
    __raw_writel(tcon, S3C_TCON);

    tcon &= ~(0x2<<8);
    __raw_writel(tcon, S3C_TCON);
}

static void PWM_Stop(void)
{
    unsigned long cstat,tcon;

    cstat = __raw_readl(S3C_TINT_CSTAT);
    cstat &=~(0x1<<1);
    __raw_writel(cstat,S3C_TINT_CSTAT);

    tcon = __raw_readl(S3C_TCON);
    tcon &=~(0x1<<8);
    __raw_writel(tcon, S3C_TCON);
}
static long dsw248_ioctl(
    struct file *file,
    unsigned int cmd,
    unsigned long arg)
{
    if(cmd>4||arg>tcnt)
    {
        return -1;
    }
    else
    {
        IOPWM[0]=cmd;
        IOPWM[1]=arg;
        return 0;
    }
}
static ElemType dsw248_open(
    struct inode *inode,
    struct file *filp)
{
    GPIO_CON = (volatile unsigned long *)ioremap (IO_ADDRESS, 16);
    GPIO_DAT = GPIO_CON + 2;
    IOPWM[0]=3;
    request_irq(IRQ_TIMER1,
                &INTERRUPT_FUNCTION,
                IRQF_DISABLED,
                PWM_PORT[0].name,
                &PWM_PORT[0]);
    PWM_Start(25000,10);
    return 0;
}
static ElemType dsw248_release(
    struct inode * inode,
    struct file * file)
{
    *GPIO_CON &= ~(0xFF<<20);
    PWM_Stop();
    free_irq(IRQ_TIMER1,&PWM_PORT[0]);
    iounmap(GPIO_CON);
    return 0;
}

static struct file_operations dsw248_fops=
{
    .owner=THIS_MODULE,
    .open=dsw248_open,
    .release=dsw248_release,
    .unlocked_ioctl = dsw248_ioctl,
};

static int __init dsw248_init(void)
{
    DEVICE_MAJOR = register_chrdev(0, DEVICE_NAME, &dsw248_fops);
    GPIO_CLASS = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(GPIO_CLASS, NULL, MKDEV(DEVICE_MAJOR, DEVICE_MINOR), NULL, DEVICE_NAME);
    return 0;
}

static void __exit dsw248_exit(void)
{
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
    device_destroy(GPIO_CLASS,MKDEV(DEVICE_MAJOR,DEVICE_MINOR));
    class_destroy(GPIO_CLASS);
}

module_init(dsw248_init);
module_exit(dsw248_exit);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("dsw248");
MODULE_DESCRIPTION("Use GPIO Output PWM");
MODULE_VERSION ("Beta");


