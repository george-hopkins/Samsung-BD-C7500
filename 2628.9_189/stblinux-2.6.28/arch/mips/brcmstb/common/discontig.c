/*
 * linux/arch/mips/brcmstb/common/discontig.c
 *
 * Discontiguous memory support.
 *
 * Copyright (C) 2006 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * when   who what
 * 060501 tht Initial codings for 97438 NUMA
 */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/mmzone.h>

int numnodes = 1;

/*
 * Our node_data structure for discontiguous memory.
 */

static bootmem_data_t node_bootmem_data[NR_NODES];

pg_data_t discontig_node_data[NR_NODES] = {
  { bdata: &node_bootmem_data[0] },
  { bdata: &node_bootmem_data[1] },
};

struct page  *node_mem_maps[NR_NODES];

int node_sizes[NR_NODES];

EXPORT_SYMBOL(numnodes);
EXPORT_SYMBOL(node_mem_maps);
EXPORT_SYMBOL(node_sizes);
EXPORT_SYMBOL(discontig_node_data);

