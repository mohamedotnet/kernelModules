#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>

#define NB_SECTORS 20
#define SECTOR_SIZE 512
#define FIRST_MINOR 0
#define MINOR_COUNT 2

static int major = 0;
module_param(major, int, 0);

/* the internal structor of our ram device */
struct bDriver {
    int size; /* Device size in sectors */
    u8 *data;
    short users;
    short media_change;
    spinlock_t lock;  /* For mutual exclusion */
    struct request_queue *queue;  /* The device request queue */
    struct gendisk *gd;  /* The gendisk structure */
    struct timer_list timer;
};

static struct bDriver *rb_Driver = NULL;
static int _dev_open(struct block_device *bdev, fmode_t mode);
static void _dev_release(struct gendisk * gdisk, fmode_t mode);


// Driver Functions
struct block_device_operations bDriver_fops = {
    .owner = THIS_MODULE,
    .open = _dev_open,
    .release = _dev_release,
};

static int _dev_open(struct block_device *bdev, fmode_t mode){
    unsigned unit = iminor(bdev->bd_inode);
    printk(KERN_INFO "Ramdisk: Device is opened\n");

    return 0;
}

static void _dev_release(struct gendisk * gdisk, fmode_t mode){
	printk(KERN_INFO "Ramdisk: Device is closed\n");
}

static void ramDriver_request(struct request_queue *q)
{
    struct request *req;
    int ret;
    return;
}

int ramDriver_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
      return -ENOTTY; /* unknown command */
}

/*  the function that initialise our ram block driver */
int RamDriver_init(void)
{
    printk(KERN_INFO "Ram Block Driver is intialized\n");

    /*  register the ram driver in the kernel and get the major number */

    major =  register_blkdev(0, "RamDriver");
    if (major <= 0)
    {
        printk(KERN_ERR "RamBlock: Error in geting the major number\n");
        return -EBUSY;
    }


    rb_Driver = kmalloc(sizeof (struct bDriver), GFP_KERNEL);
    if (rb_Driver == NULL)
    {
         printk(KERN_INFO "err in rb driver instanciation\n");
         return -EINVAL;
    }
    /* intialize the bDriver structure and make availbale in the system  */
    memset (rb_Driver, 0, sizeof (struct bDriver));
    rb_Driver->size = NB_SECTORS*SECTOR_SIZE;
    rb_Driver->data = vmalloc(rb_Driver->size);
    if (rb_Driver->data == NULL) {
        printk (KERN_NOTICE "vmalloc failure.\n");
        return -ENOMEM;
    }
    /* allocate and initialize a spinlock*/
    spin_lock_init(&rb_Driver->lock);

    /* allocate the request queue */
    rb_Driver->queue = blk_init_queue(ramDriver_request, &rb_Driver->lock);
    //rb_Driver->queue = blk_alloc_queue (GFP_KERNEL);
    if (rb_Driver->queue == NULL)
    {
        printk(KERN_ERR "rb: blk_init_queue failure\n");
        return -ENOMEM;
    }

    /* allocate and initialize the gendisk structure*/
    printk(KERN_INFO "Allocating disk...\n");
    rb_Driver->gd = alloc_disk(MINOR_COUNT);
    if (! rb_Driver->gd) {
        printk (KERN_NOTICE "alloc_disk failure\n");
        if (rb_Driver->data)  { vfree(rb_Driver->data);}
    }
    rb_Driver->gd->major = major;
    rb_Driver->gd->first_minor = 0;
    rb_Driver->gd->fops = &bDriver_fops;
    rb_Driver->gd->queue = rb_Driver->queue;
    rb_Driver->gd->private_data = rb_Driver;
    snprintf (rb_Driver->gd->disk_name, 32, "ramDevice");

    printk(KERN_INFO "Setting the capacity...\n");
    set_capacity(rb_Driver->gd, NB_SECTORS);

    printk(KERN_INFO "Adding gendisk...\n");
    add_disk(rb_Driver->gd);

    return 0;
}

/*  the function that releases the driver */
void RamDriver_cleanup(void)
{
    del_gendisk(rb_Driver->gd);
    put_disk(rb_Driver->gd);
    blk_cleanup_queue(rb_Driver->queue);
    unregister_blkdev(major, "RamDriver");
    vfree(rb_Driver->data);
}


module_init(RamDriver_init);
module_exit(RamDriver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ines H <fi_harmali@esi.dz>");
MODULE_DESCRIPTION("Ram Block Driver");
