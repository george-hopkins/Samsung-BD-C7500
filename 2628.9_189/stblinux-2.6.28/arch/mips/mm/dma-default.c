/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000  Ani Joshi <ajoshi@unixbox.com>
 * Copyright (C) 2000, 2001, 06  Ralf Baechle <ralf@linux-mips.org>
 * swiped from i386, and cloned for MIPS by Geert, polished by Ralf.
 */

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/string.h>

#include <asm/cache.h>
#include <asm/io.h>

#include <dma-coherence.h>

static inline unsigned long dma_addr_to_virt(dma_addr_t dma_addr)
{
	unsigned long addr = plat_dma_addr_to_phys(dma_addr);

	return (unsigned long)phys_to_virt(addr);
}

/*
 * Warning on the terminology - Linux calls an uncached area coherent;
 * MIPS terminology calls memory areas with hardware maintained coherency
 * coherent.
 */

static inline int cpu_is_noncoherent_r10000(struct device *dev)
{
	return !plat_device_is_coherent(dev) &&
	       (current_cpu_type() == CPU_R10000 ||
	       current_cpu_type() == CPU_R12000);
}

static gfp_t massage_gfp_flags(const struct device *dev, gfp_t gfp)
{
	/* ignore region specifiers */
	gfp &= ~(__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM);

#ifdef CONFIG_ZONE_DMA
	if (dev == NULL)
		gfp |= __GFP_DMA;
	else if (dev->coherent_dma_mask < DMA_BIT_MASK(24))
		gfp |= __GFP_DMA;
	else
#endif
#ifdef CONFIG_ZONE_DMA32
	     if (dev->coherent_dma_mask < DMA_BIT_MASK(32))
		gfp |= __GFP_DMA32;
	else
#endif
		;

	/* Don't invoke OOM killer */
	gfp |= __GFP_NORETRY;

	return gfp;
}

void *dma_alloc_noncoherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, gfp_t gfp)
{
	void *ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = plat_map_dma_mem(dev, ret, size);
	}

	return ret;
}

EXPORT_SYMBOL(dma_alloc_noncoherent);

void *dma_alloc_coherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, gfp_t gfp)
{
	void *ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret) {
		memset(ret, 0, size);
		*dma_handle = plat_map_dma_mem(dev, ret, size);

		if (!plat_device_is_coherent(dev)) {
			dma_cache_wback_inv((unsigned long) ret, size);
			ret = UNCAC_ADDR(ret);
		}
	}

	return ret;
}

EXPORT_SYMBOL(dma_alloc_coherent);

void dma_free_noncoherent(struct device *dev, size_t size, void *vaddr,
	dma_addr_t dma_handle)
{
	plat_unmap_dma_mem(dma_handle);
	free_pages((unsigned long) vaddr, get_order(size));
}

EXPORT_SYMBOL(dma_free_noncoherent);

void dma_free_coherent(struct device *dev, size_t size, void *vaddr,
	dma_addr_t dma_handle)
{
	unsigned long addr = (unsigned long) vaddr;

	plat_unmap_dma_mem(dma_handle);

	if (!plat_device_is_coherent(dev))
		addr = CAC_ADDR(addr);

	free_pages(addr, get_order(size));
}

EXPORT_SYMBOL(dma_free_coherent);

static inline void __dma_sync(unsigned long addr, size_t size,
	enum dma_data_direction direction)
{

	BUG_ON(addr < KSEG0);

	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr, size);
		break;

	default:
		BUG();
	}
}

dma_addr_t dma_map_single(struct device *dev, void *ptr, size_t size,
	enum dma_data_direction direction)
{
	unsigned long addr = (unsigned long) ptr;

	if (!plat_device_is_coherent(dev))
		__dma_sync(addr, size, direction);

	return plat_map_dma_mem(dev, ptr, size);
}

EXPORT_SYMBOL(dma_map_single);

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
	enum dma_data_direction direction)
{
	if (cpu_is_noncoherent_r10000(dev))
		__dma_sync(dma_addr_to_virt(dma_addr), size,
		           direction);

	plat_unmap_dma_mem(dma_addr);
}

EXPORT_SYMBOL(dma_unmap_single);

#ifdef CONFIG_MIPS_BCM_NDVD
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nents; i++, sg++) {

	        BUG_ON(!sg_page(sg));

		sg->dma_address = sg_phys(sg);

		if (!plat_device_is_coherent(dev)) {
			unsigned char *addr;
			unsigned long size, resid, offset;
			struct page *pg;

			pg = sg_page(sg);

			/* 
			 * check if target is entirely in low memory
			 */

			if (!PageHighMem(pg)) {
				addr = lowmem_page_address(pg) + sg->offset;
				if (!PageHighMem( virt_to_page(addr + sg->length)) ) {
					__dma_sync((unsigned long)addr, sg->length, direction);
					continue;
				}
			}

			/*
			 * when pages are dynamically mapped, we have to 
			 * check each page in the range.
			 * Because of the way kmap_high leaves a page mapped
			 * until it is resused, it's possible for the first page
			 * to be mapped, but not the next.
			 */
			resid = sg->length;
			offset = sg->offset;
			while (resid) {
				size = (PAGE_SIZE - offset);
				size = (size < resid) ? size: resid;

				if ((addr = page_address(pg))) {
					addr += offset;
					__dma_sync((unsigned long)addr, size, direction);
				}  

				resid -= size;
				offset = 0;
				pg++;
			}
		}
	}

	return nents;
}
#else
int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nents; i++, sg++) {
		unsigned long addr;

	        BUG_ON(!sg_page(sg));

		addr = (unsigned long) sg_virt(sg);
		if (!plat_device_is_coherent(dev) && (addr >= KSEG0))
			__dma_sync(addr, sg->length, direction);

		sg->dma_address = sg_phys(sg);
	}

	return nents;
}
#endif

EXPORT_SYMBOL(dma_map_sg);

dma_addr_t dma_map_page(struct device *dev, struct page *page,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (!plat_device_is_coherent(dev)) {
		unsigned long addr;

		addr = (unsigned long) page_address(page) + offset;
		if (addr >= KSEG0)
		    dma_cache_wback_inv(addr, size);
	}

	return plat_map_dma_mem_page(dev, page) + offset;
}

EXPORT_SYMBOL(dma_map_page);

void dma_unmap_page(struct device *dev, dma_addr_t dma_address, size_t size,
	enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (!plat_device_is_coherent(dev) && direction != DMA_TO_DEVICE) {
		unsigned long addr;
		unsigned int offset;

		offset = dma_address & ~PAGE_MASK;
		addr = (unsigned long)page_address(pfn_to_page(dma_address >> PAGE_SHIFT)) + offset;
		if (addr >= KSEG0)
		    dma_cache_wback_inv(addr, size);
	}

	plat_unmap_dma_mem(dma_address);
}

EXPORT_SYMBOL(dma_unmap_page);

#ifdef CONFIG_MIPS_BCM_NDVD
void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nhwentries; i++, sg++) {

		if (!plat_device_is_coherent(dev) &&
		    direction != DMA_TO_DEVICE) {
			unsigned char *addr;
			unsigned long size, resid, offset;
			struct page *pg;

			pg = sg_page(sg);

			/* 
			 * check if target is entirely in low memory
			 */

			if (!PageHighMem(pg)) {
				addr = lowmem_page_address(pg) + sg->offset;
				if (!PageHighMem( virt_to_page(addr + sg->length)) ) {
					__dma_sync((unsigned long)addr, sg->length, direction);
					continue;
				}
			}

			/*
			 * when pages are dynamically mapped, we have to 
			 * check each page in the range.
			 * Because of the way kmap_high leaves a page mapped
			 * until it is resused, it's possible for the first page
			 * to be mapped, but not the next.
			 */
			resid = sg->length;
			offset = sg->offset;
			while (resid) {
				size = (PAGE_SIZE - offset);
				size = (size < resid) ? size: resid;

				if ((addr = page_address(pg))) {
					addr += offset;
					__dma_sync((unsigned long)addr, size, direction);
				}  

				resid -= size;
				offset = 0;
				pg++;
			}
		}
		plat_unmap_dma_mem(sg->dma_address);
	}
}
#else
void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
	enum dma_data_direction direction)
{
	unsigned long addr;
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nhwentries; i++, sg++) {
		if (!plat_device_is_coherent(dev) &&
		    direction != DMA_TO_DEVICE) {
			addr = (unsigned long) sg_virt(sg);
			if (addr >= KSEG0)
				__dma_sync(addr, sg->length, direction);
		}
		plat_unmap_dma_mem(sg->dma_address);
	}
}
#endif

EXPORT_SYMBOL(dma_unmap_sg);

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (cpu_is_noncoherent_r10000(dev)) {
		unsigned long addr;

		addr = dma_addr_to_virt(dma_handle);
		__dma_sync(addr, size, direction);
	}
}

EXPORT_SYMBOL(dma_sync_single_for_cpu);

void dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (!plat_device_is_coherent(dev)) {
		unsigned long addr;

		addr = dma_addr_to_virt(dma_handle);
		__dma_sync(addr, size, direction);
	}
}

EXPORT_SYMBOL(dma_sync_single_for_device);

void dma_sync_single_range_for_cpu(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (cpu_is_noncoherent_r10000(dev)) {
		unsigned long addr;

		addr = dma_addr_to_virt(dma_handle);
		__dma_sync(addr + offset, size, direction);
	}
}

EXPORT_SYMBOL(dma_sync_single_range_for_cpu);

void dma_sync_single_range_for_device(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (!plat_device_is_coherent(dev)) {
		unsigned long addr;

		addr = dma_addr_to_virt(dma_handle);
		__dma_sync(addr + offset, size, direction);
	}
}

EXPORT_SYMBOL(dma_sync_single_range_for_device);

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		if (cpu_is_noncoherent_r10000(dev))
			__dma_sync((unsigned long)page_address(sg_page(sg)),
			           sg->length, direction);
	}
}

EXPORT_SYMBOL(dma_sync_sg_for_cpu);

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		if (!plat_device_is_coherent(dev))
			__dma_sync((unsigned long)page_address(sg_page(sg)),
			           sg->length, direction);
	}
}

EXPORT_SYMBOL(dma_sync_sg_for_device);

int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

EXPORT_SYMBOL(dma_mapping_error);

int dma_supported(struct device *dev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
	if (mask < DMA_BIT_MASK(24))
		return 0;

	return 1;
}

EXPORT_SYMBOL(dma_supported);

int dma_is_consistent(struct device *dev, dma_addr_t dma_addr)
{
	return plat_device_is_coherent(dev);
}

EXPORT_SYMBOL(dma_is_consistent);

void dma_cache_sync(struct device *dev, void *vaddr, size_t size,
	       enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);
	BUG_ON(vaddr < (void *)KSEG0);

	if (!plat_device_is_coherent(dev))
		__dma_sync((unsigned long)vaddr, size, direction);
}

EXPORT_SYMBOL(dma_cache_sync);
