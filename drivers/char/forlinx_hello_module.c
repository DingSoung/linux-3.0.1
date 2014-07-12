
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");
static int dsw248_init(void)
{
	printk(KERN_ALERT "Hello, world\n");
	return 0;
}
static void dsw248_exit(void)
{
	printk(KERN_ALERT "Goodbye, cruel world\n");
}
module_init(dsw248_init);
module_exit(dsw248_exit);

