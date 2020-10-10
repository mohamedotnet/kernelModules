#include <linux/module.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/init.h>
#include <linux/types.h>

MODULE_LICENSE("Dual BSD/GPL");

static int ramdisk_major = 0;

static int nsectors = 10;
//module_param(nsectors, int, 0);


#define SECTOR_SIZE                  512
#define BBLOCK_MINORS                  1
#define MINORS                        16

static int _dev_open(struct block_device *bdev, fmode_t mode);
static void _dev_release(struct gendisk * gdisk, fmode_t mode);

struct ramdisk_dev{
    unsigned int size;
    u8 *data;
    spinlock_t lock;
    struct request_queue *rb_queue;
    struct gendisk *rb_disk;
}_dev;

static struct block_device_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = _dev_open,
    .release = _dev_release,
};

static int _dev_open(struct block_device *bdev, fmode_t mode){
    unsigned unit = iminor(bdev->bd_inode);

    printk(KERN_INFO "Ramdisk: Device is opened\n");
    printk(KERN_INFO "Ramdisk: Inode number is %d\n", unit);

    if (unit > MINORS)
        return -ENODEV;
    return 0;
}

static void _dev_release(struct gendisk * gdisk, fmode_t mode){
	printk(KERN_INFO "Ramdisk: Device is closed\n");
}
//static struct ramdisk_dev *dev = NULL;


/*
    transfering function
*/
static void ramdisk_transfer(struct request *req){
    int dir = rq_data_dir(req);
    int sector = blk_rq_pos(req);
    int nsectors = blk_rq_sectors(req);

    int nsec;
    // TODO: verify that we don't exceed device size
    char *buffer, *ddata_location;

    struct bio_vec bvec;
    struct req_iterator iter;

    ddata_location = _dev.data;

    rq_for_each_segment(bvec, req, iter){
        //a pointer to where the desired data resides
        buffer = page_address(bvec.bv_page) + bvec.bv_offset;
        // TODO: check for the length of the buffer
        nsec = bvec.bv_len / SECTOR_SIZE;

        if (dir){ //it's a write
            printk(KERN_NOTICE "writing data to device\n");
            memcpy(ddata_location, buffer, bvec.bv_len);
        }else{
            printk(KERN_NOTICE "reading data from device\n");
            memcpy(buffer, ddata_location, bvec.bv_len);
        }
        ddata_location += bvec.bv_len;
    }
}


/*
    request handling function
*/
static void ramdisk_request(struct request_queue *q){
    struct request *req;
    int nsect = 0;


    while((req = blk_fetch_request(q)) != NULL){
        ramdisk_transfer(req);
        __blk_end_request_all(req, 0);
    }
}


/*
    setup our ramdisk device
*/
static void setup_ramdisk_device(void){
    printk(KERN_NOTICE "Setting Up The Device\n");

    memset(&_dev, 0, sizeof(struct ramdisk_dev));
    _dev.size = nsectors * SECTOR_SIZE;
    _dev.data = vmalloc(_dev.size);
    if (_dev.data == NULL){
        printk(KERN_ERR "Cannot create data for the device\n");
        return;
    }

    spin_lock_init(&_dev.lock);

    //init request queue
    printk(KERN_NOTICE "Request queue initialization ... \n");
    _dev.rb_queue = blk_init_queue(ramdisk_request,
                                        &_dev.lock);
    if (_dev.rb_queue == NULL){
        printk(KERN_ERR "Request initialization failed\n");
        goto err_out;
    }

    //allocate gendisk in our device
    printk(KERN_INFO "Allocating disk...\n");
    _dev.rb_disk = alloc_disk(MINORS);

    if (_dev.rb_disk == NULL){
        printk(KERN_ERR "alloc_disk failed\n");
        goto err_out;
    }
    printk(KERN_NOTICE "Initializing gendisk...\n");
    //gendisk init
    _dev.rb_disk->major = ramdisk_major;
    _dev.rb_disk->first_minor = 0;
    snprintf (_dev.rb_disk->disk_name, 32, "ramdisk");
    _dev.rb_disk->fops = &dev_fops;
    _dev.rb_disk->queue = _dev.rb_queue;
    _dev.rb_disk->private_data = &_dev;

    //set the capacity of the generic disk
    printk(KERN_INFO "Setting up the capacity of the disk...\n");
    set_capacity(_dev.rb_disk, _dev.size);

    //add disk
    printk(KERN_INFO "Adding disk...\n");
    //device_add_disk(NULL, _dev.rb_disk);

    add_disk(_dev.rb_disk);
    //log: the device is created
    printk(KERN_INFO "ramdisk: device %s is created\n",
                        _dev.rb_disk->disk_name);

    //success return
    return;

err_out:
    vfree(_dev.data);
    return;
}


/*
    init module
*/
static int ramdisk_driver_init(void){
    //register ramdisk driver
    ramdisk_major = register_blkdev(0, "ramdisk");
    if (ramdisk_major <= 0){
        printk(KERN_ERR "registering the device failed\n");
        return EBUSY;
    }
    //dev = kmalloc(sizeof(struct ramdisk_dev), GFP_KERNEL);
    setup_ramdisk_device();

    printk("HELLO %d\n", ramdisk_major);
    return 0;
}

/*
    module exit
*/
static void ramdisk_driver_exit(void){
    if (_dev.rb_disk){
      del_gendisk(_dev.rb_disk);
      put_disk(_dev.rb_disk);
    }
    if (_dev.rb_queue){
      blk_cleanup_queue(_dev.rb_queue);
    }
    unregister_blkdev(ramdisk_major, "ramdisk");
}

module_init(ramdisk_driver_init);
module_exit(ramdisk_driver_exit);
