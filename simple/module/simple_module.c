#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

dev_t id;
struct cdev cdev;
struct class *class;
struct device *dev;

char temp[100];

#define DEVICE_NAME "simple"

int simple_open(struct inode *inode, struct file *filp)
{
	printk( "open\n" );
	memset( temp, 0, 0 );

	return 0;
}

int simple_close(struct inode *inode, struct file *filp)
{
	printk( "close\n" );
	return 0;
}

ssize_t simple_read(struct file *filp, char *buf, size_t size, loff_t *offset)
{
	int ret;

	printk( "simple_read\n" );
	printk( "DEV : write something\n" );
	printk( "%s %dbytes\n", temp, (int)strlen(temp) );
	ret = copy_to_user( buf, temp, strlen(temp)+1 );

	return strlen(temp);
}

ssize_t simple_write(struct file *filp, const char *buf,
						size_t size, loff_t *offset)
{
	int ret;
	printk( "simple_write\n" );
	printk( "DEV : read something\n");

	ret = copy_from_user( temp, buf, size );
	printk( "%s %dbytes\n", temp, (int)size );

	return size;
}

long simple_ioctl ( struct file *filp, unsigned int cmd, unsigned long arg)
{

	printk( "ioctl\n" );
	return 0;
}


struct file_operations simple_fops =
{
	.owner           = THIS_MODULE,
	.read            = simple_read,
	.write           = simple_write,
	.unlocked_ioctl  = simple_ioctl,
	.open            = simple_open,
	.release         = simple_close,
};

int simple_init(void)
{
	int ret;

	ret = alloc_chrdev_region( &id, 0, 1, DEVICE_NAME );
	if ( ret ){
		printk( "alloc_chrdev_region error %d\n", ret );
		return ret;
	}

	cdev_init( &cdev, &simple_fops );
	cdev.owner = THIS_MODULE;

	ret = cdev_add( &cdev, id, 1 );
	if (ret){
		printk( "cdev_add error %d\n", ret );
		unregister_chrdev_region( id, 1 );
		return ret;
	}

	class = class_create( THIS_MODULE, DEVICE_NAME );
	if (IS_ERR(class)){
		ret = PTR_ERR( class );
		printk( "class_create error %d\n", ret );

		cdev_del( &cdev );
		unregister_chrdev_region( id, 1 );
		return ret;
	}

	dev = device_create( class, NULL, id, NULL, DEVICE_NAME );
	if ( IS_ERR(dev) ){
		ret = PTR_ERR(dev);
		printk( "device_create error %d\n", ret );

		class_destroy(class);
		cdev_del( &cdev );
		unregister_chrdev_region( id, 1 );
		return ret;
	}

	printk("%s done\n", __func__);

	return 0;
}

void simple_exit(void)
{
	printk("%s call\n", __func__);

	device_destroy(class, id );
	class_destroy(class);
	cdev_del( &cdev );
	unregister_chrdev_region( id, 1 );
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("Dual BSD/GPL");
