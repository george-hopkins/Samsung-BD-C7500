/*
 *  Readonly Block Device Layer Over MTD
 *
 *  (C) 2006 Baydarov Konstantin <kbaidarov at dev.rtsoft.ru>
 *           Pantelis Antoniou <panto at intracom.gr>
 *           David Woodhouse <dwmw2 at infradead.org>
 *
 *  It allows to use any filesystem on this device in
 *  RO mode and thus gain faster mount times and better
 *  throughput rates.
 *  It allows to put one RO filesystem into NAND using a
 *  bad-block aware program initially, and mount it.  But be aware,
 *  it doesn't handle run-time bit flips or errors (even if this event
 *  seems pretty rare).
 *
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

struct romblock_map {
	struct mtd_blktrans_dev dev;
	/* block map for RO */
	int32_t *block_map;
	int32_t block_top;
	int32_t block_scantop;
};

static loff_t map_over_bad_blocks(struct mtd_blktrans_dev *dev, loff_t from)
{
	int i, block;
	struct mtd_info *mtd = dev->mtd;
	struct romblock_map *dev_cont =
	    container_of(dev, struct romblock_map, dev);
	int32_t *block_map = dev_cont->block_map;
	int32_t block_top = dev_cont->block_top;
	int32_t block_scantop = dev_cont->block_scantop;

	/* if no bad block checking is possible bail out */
	if (mtd->block_isbad == NULL)
		return from;

	/* first time in */
	if (block_map == NULL) {
		block_top = mtd->size / mtd->erasesize;
		//block_map = kmalloc(sizeof(*block_map) * block_top, GFP_KERNEL);
		block_map = vmalloc(sizeof(*block_map) * block_top);
		if (block_map == NULL) {
			printk(KERN_ERR
			       "map_over_bad_blocks(): unable to allocate block map\n");
			return -ENOMEM;
		}

		/* THT/Sidc: Update global struct */
		dev_cont->block_map = block_map;
		dev_cont->block_top = block_top;
		
		for (i = 0; i < block_top; i++)
			block_map[i] = -1;

		for (i = 0; i < block_top; i++)
			if ((*mtd->block_isbad) (mtd, i * mtd->erasesize) == 0)
				break;

		if (i >= block_top) {
			printk(KERN_WARNING
			       "map_over_bad_blocks(): all blocks bad!\n");
			return -EIO;
		}
		block_scantop = 0;
		block_map[0] = i;

		DEBUG(MTD_DEBUG_LEVEL0, "mtd: map %d -> %d\n", block_scantop,
		      block_map[block_scantop]);
	}

	block = ((int)from / mtd->erasesize);
	if (block >= block_top)
		return (loff_t)-1;

	/* scan for bad block up to where we want */
	while (block >= block_scantop) {
		/* find a non bad block */
		for (i = block_map[block_scantop] + 1; i < block_top; i++)
			if ((*mtd->block_isbad) (mtd, i * mtd->erasesize) == 0)
				break;

		/* exhausted ? */
		if (i > block_top) {
		  	DEBUG(MTD_DEBUG_LEVEL0, "mtd:  no more good blocks, scanned %d of %d blocks, scanned 0 to %d, current block num %d \n", 
				i, block_top, block_scantop, block);

			return (loff_t)-1;
		}

		if (i == block_top) {
		  	DEBUG(MTD_DEBUG_LEVEL0, "mtd:  using last good block, scanned %d of %d blocks, scanned 0 to %d, current block num %d \n", 
				i, block_top, block_scantop, block);
			
			/* we are now past the last available block  */
			++block_scantop; 
		}
		else {

			block_map[++block_scantop] = i;
			DEBUG(MTD_DEBUG_LEVEL0, "mtd: map %d -> %d\n", block_scantop,
	      		block_map[block_scantop]);

			/* THT/Sidc: Update global struct */
			dev_cont->block_scantop = block_scantop;
		}	
	}

	block = block_map[(int)from / mtd->erasesize];
	from = (block * mtd->erasesize) | ((int)from & (mtd->erasesize - 1));
	return from;
}

static int romblock_readsect(struct mtd_blktrans_dev *dev,
			     unsigned long block, char *buf)
{
	size_t retlen;
	unsigned long from;
    int returncode;

	from = map_over_bad_blocks(dev, block << 9);
    returncode = dev->mtd->read(dev->mtd, from, 512, &retlen, buf);
    switch (returncode)
    {
    case 0:           //No Error or Correctable error, continue!
        return 0;
        break;

    case -EBADMSG:    // Uncorrectable ECC Error... 
        return 1;     // Give it to upper layer.
        break;

    default:          // This is an unknown type of error... 
        return 1;     // Give it back to upper layer.
        break;
    }
}

static int romblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	size_t retlen;

	if (dev->mtd->write(dev->mtd, (block * 512), 512, &retlen, buf))
		return 1;
	return 0;
}

static void romblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct romblock_map *dev_cont = kzalloc(sizeof(*dev_cont), GFP_KERNEL);

	if (!dev_cont)
		return;

	dev_cont->dev.mtd = mtd;
	dev_cont->dev.devnum = mtd->index;
	dev_cont->dev.size = mtd->size >> 9;
	dev_cont->dev.tr = tr;
	dev_cont->dev.readonly = 1;

	add_mtd_blktrans_dev(&(dev_cont->dev));
}

static void romblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	struct romblock_map* dev_cont = container_of(dev, struct romblock_map, dev);
	
	del_mtd_blktrans_dev(dev);
	/* THT: Free the map */
	if (dev_cont->block_map) {
		kfree(dev_cont->block_map);
		dev_cont->block_map = NULL;
	}
	vfree(dev_cont);
}

static struct mtd_blktrans_ops romblock_tr = {
	.name = "romblock",
	.major = 253,
	.part_bits = 0,
	.blksize = 512,
	.readsect = romblock_readsect,
	.writesect = romblock_writesect,
	.add_mtd = romblock_add_mtd,
	.remove_dev = romblock_remove_dev,
	.owner = THIS_MODULE,
};

static int __init romblock_init(void)
{
	return register_mtd_blktrans(&romblock_tr);
}

static void __exit romblock_exit(void)
{
	deregister_mtd_blktrans(&romblock_tr);
}

module_init(romblock_init);
module_exit(romblock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Baydarov Konstantin <kbaidarov at dev.rtsoft.ru>");
MODULE_DESCRIPTION("Readonly Block Device Layer Over MTD");
