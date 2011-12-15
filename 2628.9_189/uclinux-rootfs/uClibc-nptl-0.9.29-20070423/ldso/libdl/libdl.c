/* vi: set sw=4 ts=4: */
/*
 * Program to load an ELF binary on a linux system, and run it
 * after resolving ELF shared library symbols
 *
 * Copyright (C) 2000-2006 by Erik Andersen <andersen@uclibc.org>
 * Copyright (c) 1994-2000 Eric Youngdale, Peter MacDonald,
 *				David Engel, Hongjiu Lu and Mitch D'Souza
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the above contributors may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <ldso.h>
#include <stdio.h>

#ifndef SHARED
/* TLS currently not supported with static dlopen */
#undef USE_TLS
#endif

#if USE_TLS
# include <ldsodefs.h>
#endif


#ifdef SHARED

#if USE_TLS
# include <dl-tls.h>

extern void _dl_add_to_slotinfo(struct link_map  *l);
extern struct link_map *_dl_update_slotinfo(unsigned long int req_modid);
extern struct elf_resolve *_dl_tls_reloc_tpnt;
extern ElfW(Sym) *_dl_tls_reloc_sym;
#endif

/* When libdl is loaded as a shared library, we need to load in
 * and use a pile of symbols from ldso... */

extern char *_dl_find_hash(const char *, struct dyn_elf *, struct elf_resolve *, int);
extern struct elf_resolve * _dl_load_shared_library(int, struct dyn_elf **,
	struct elf_resolve *, char *, int);
extern int _dl_fixup(struct dyn_elf *rpnt, int lazy);
extern void _dl_protect_relro(struct elf_resolve * tpnt);
extern int _dl_errno;
extern struct dyn_elf *_dl_symbol_tables;
extern struct dyn_elf *_dl_handles;
extern struct elf_resolve *_dl_loaded_modules;
extern struct r_debug *_dl_debug_addr;
extern unsigned long _dl_error_number;
extern void *(*_dl_malloc_function)(size_t);
extern void _dl_run_init_array(struct elf_resolve *);
extern void _dl_run_fini_array(struct elf_resolve *);
#ifdef __LDSO_CACHE_SUPPORT__
int _dl_map_cache(void);
int _dl_unmap_cache(void);
#endif
#ifdef __mips__
extern void _dl_perform_mips_global_got_relocations(struct elf_resolve *tpnt, int lazy);
#endif
#ifdef __SUPPORT_LD_DEBUG__
extern char *_dl_debug;
#endif


#else /* SHARED */

/* When libdl is linked as a static library, we need to replace all
 * the symbols that otherwise would have been loaded in from ldso... */

#ifdef __SUPPORT_LD_DEBUG__
char *_dl_debug  = 0;
#endif
const char *_dl_progname       = "";        /* Program name */
char *_dl_library_path         = 0;         /* Where we look for libraries */
char *_dl_ldsopath             = 0;         /* Location of the shared lib loader */
int _dl_errno                  = 0;         /* We can't use the real errno in ldso */
size_t _dl_pagesize            = PAGE_SIZE; /* Store the page size for use later */
/* This global variable is also to communicate with debuggers such as gdb. */
struct r_debug *_dl_debug_addr = NULL;
#define _dl_malloc malloc
#include "../ldso/dl-debug.c"
#if USE_TLS
#include "../ldso/dl-tls.c"

/*
 * Giving this initialized value preallocates some surplus bytes in the
 * static TLS area, see __libc_setup_tls (libc-tls.c).
 */
size_t _dl_tls_static_size = 2048;
#endif
#include LDSO_ELFINTERP
#include "../ldso/dl-hash.c"
#define _dl_trace_loaded_objects    0
#include "../ldso/dl-elf.c"
#endif /* SHARED */

#ifdef __SUPPORT_LD_DEBUG__
# define _dl_if_debug_print(fmt, args...) \
	do { \
	if (_dl_debug) \
		fprintf(stderr, "%s():%i: " fmt, __FUNCTION__, __LINE__, ## args); \
	} while (0)
#else
# define _dl_if_debug_print(fmt, args...)
#endif

static int do_dlclose(void *, int need_fini);


static const char *dl_error_names[] = {
	"",
	"File not found",
	"Unable to open /dev/zero",
	"Not an ELF file",
#if defined (__i386__)
	"Not i386 binary",
#elif defined (__sparc__)
	"Not sparc binary",
#elif defined (__mc68000__)
	"Not m68k binary",
#else
	"Unrecognized binary type",
#endif
	"Not an ELF shared library",
	"Unable to mmap file",
	"No dynamic section",
	"Library contains unsupported TLS",
#ifdef ELF_USES_RELOCA
	"Unable to process REL relocs",
#else
	"Unable to process RELA relocs",
#endif
	"Bad handle",
	"Unable to resolve symbol"
};


#if USE_TLS
#ifdef SHARED
/*
 * Systems which do not have tls_index also probably have to define
 * DONT_USE_TLS_INDEX.
 */

# ifndef __TLS_GET_ADDR
#  define __TLS_GET_ADDR __tls_get_addr
# endif

/*
 * Return the symbol address given the map of the module it is in and
 *  the symbol record.  This is used in dl-sym.c.
 */
static void *
internal_function
_dl_tls_symaddr(struct link_map *map, const ElfW(Sym) *ref)
{
# ifndef DONT_USE_TLS_INDEX
	tls_index tmp =
	{
		.ti_module = map->l_tls_modid,
		.ti_offset = ref->st_value
	};

	return __TLS_GET_ADDR (&tmp);
# else
	return __TLS_GET_ADDR (map->l_tls_modid, ref->st_value);
# endif
}
#endif

/* Returns true we an non-empty was found.  */
static bool
remove_slotinfo(size_t idx, struct dtv_slotinfo_list *listp, size_t disp,
	 bool should_be_there)
{
	if(idx - disp >= listp->len)
	{
		if(listp->next == NULL)
		{
			/*
			 * The index is not actually valid in the slotinfo list,
			 * because this object was closed before it was fully set
			 * up due to some error.
			 */
			_dl_assert(!should_be_there);
		}
		else
		{
			if(remove_slotinfo(idx, listp->next, disp + listp->len,
					should_be_there))
				return true;

			/*
			 * No non-empty entry. Search from the end of this element's
			 * slotinfo array.
			 */
			idx = disp + listp->len;
		}
	}
	else
	{
		struct link_map *old_map = listp->slotinfo[idx - disp].map;

		/*
		 * The entry might still be in its unused state if we are
		 * closing an object that wasn't fully set up.
		 */
		if(__builtin_expect(old_map != NULL, 1))
		{
			_dl_assert(old_map->l_tls_modid == idx);

			/* Mark the entry as unused. */
			listp->slotinfo[idx - disp].gen = _dl_tls_generation + 1;
			listp->slotinfo[idx - disp].map = NULL;
		}

		/*
		 * If this is not the last currently used entry no need to
		 * look further.
		 */
		if (idx != _dl_tls_max_dtv_idx)
			return true;
	}

	while(idx - disp > (disp == 0 ? 1 + _dl_tls_static_nelem : 0))
	{
		--idx;

		if(listp->slotinfo[idx - disp].map != NULL)
		{
			/* Found a new last used index.  */
			_dl_tls_max_dtv_idx = idx;
			return true;
		}
	}

	/* No non-entry in this list element.  */
	return false;
}
#endif

void dl_cleanup(void) __attribute__ ((destructor));
void dl_cleanup(void)
{
	struct dyn_elf *d;
	for (d = _dl_handles; d; d = d->next_handle) {
		do_dlclose(d, 1);
	}
}

void *dlopen(const char *libname, int flag)
{
	struct elf_resolve *tpnt, *tfrom;
	struct dyn_elf *dyn_chain, *rpnt = NULL, *dyn_ptr, *relro_ptr, *handle;
	ElfW(Addr) from;
	struct elf_resolve *tpnt1;
	void (*dl_brk) (void);
	int now_flag;
	struct init_fini_list *tmp, *runp, *runp2, *dep_list;
	unsigned int nlist, i;
	struct elf_resolve **init_fini_list;
#ifdef USE_TLS
	bool any_tls = false;
#endif

	/* A bit of sanity checking... */
	if (!(flag & (RTLD_LAZY|RTLD_NOW))) {
		_dl_error_number = LD_BAD_HANDLE;
		return NULL;
	}

	from = (ElfW(Addr)) __builtin_return_address(0);

	/* Cover the trivial case first */
	if (!libname)
		return _dl_symbol_tables;

	_dl_map_cache();

	/*
	 * Try and locate the module we were called from - we
	 * need this so that we get the correct RPATH/RUNPATH.  Note that
	 * this is the current behavior under Solaris, but the
	 * ABI+ specifies that we should only use the RPATH from
	 * the application.  Thus this may go away at some time
	 * in the future.
	 */
	{
		struct dyn_elf *dpnt;
		tfrom = NULL;
		for (dpnt = _dl_symbol_tables; dpnt; dpnt = dpnt->next) {
			tpnt = dpnt->dyn;
			if (tpnt->loadaddr < from
					&& (tfrom == NULL || tfrom->loadaddr < tpnt->loadaddr))
				tfrom = tpnt;
		}
	}
	for(rpnt = _dl_symbol_tables; rpnt && rpnt->next; rpnt=rpnt->next);

	relro_ptr = rpnt;
	now_flag = (flag & RTLD_NOW) ? RTLD_NOW : 0;
	if (getenv("LD_BIND_NOW"))
		now_flag = RTLD_NOW;

	/* Try to load the specified library */
	_dl_if_debug_print("Trying to dlopen '%s', RTLD_GLOBAL:%d RTLD_NOW:%d\n",
			(char*)libname, (flag & RTLD_GLOBAL ? 1:0), (now_flag & RTLD_NOW ? 1:0));
	tpnt = _dl_load_shared_library(0, &rpnt, tfrom, (char*)libname, 0);

	if (tpnt == NULL) {
		_dl_unmap_cache();
		return NULL;
	}
	dyn_chain = (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
	_dl_memset(dyn_chain, 0, sizeof(struct dyn_elf));
	dyn_chain->dyn = tpnt;
	tpnt->rtld_flags |= (flag & RTLD_GLOBAL);

	dyn_chain->next_handle = _dl_handles;
	_dl_handles = dyn_ptr = dyn_chain;

	if (tpnt->usage_count > 1) {
		_dl_if_debug_print("Lib: %s already opened\n", libname);
		/* see if there is a handle from a earlier dlopen */
		for (handle = _dl_handles->next_handle; handle; handle = handle->next_handle) {
			if (handle->dyn == tpnt) {
				dyn_chain->init_fini.init_fini = handle->init_fini.init_fini;
				dyn_chain->init_fini.nlist = handle->init_fini.nlist;
				for(i=0; i < dyn_chain->init_fini.nlist; i++)
					dyn_chain->init_fini.init_fini[i]->rtld_flags |= (flag & RTLD_GLOBAL);
				dyn_chain->next = handle->next;
				break;
			}
		}
		return dyn_chain;
	} else {
		tpnt->init_flag |= DL_OPENED;
	}

	_dl_if_debug_print("Looking for needed libraries\n");
	nlist = 0;
	runp = alloca(sizeof(*runp));
	runp->tpnt = tpnt;
	runp->next = NULL;
	dep_list = runp2 = runp;
	for (; runp; runp = runp->next)
	{
		ElfW(Dyn) *dpnt;
		char *lpntstr;

		nlist++;
		runp->tpnt->init_fini = NULL; /* clear any previous dependcies */
		for (dpnt = (ElfW(Dyn) *) runp->tpnt->dynamic_addr; dpnt->d_tag; dpnt++) {
			if (dpnt->d_tag == DT_NEEDED) {
				lpntstr = (char*) (runp->tpnt->dynamic_info[DT_STRTAB] +
						dpnt->d_un.d_val);
				_dl_if_debug_print("Trying to load '%s', needed by '%s'\n",
						lpntstr, runp->tpnt->libname);
				tpnt1 = _dl_load_shared_library(0, &rpnt, runp->tpnt, lpntstr, 0);
				if (!tpnt1)
					goto oops;

				tpnt1->rtld_flags |= (flag & RTLD_GLOBAL);

				if (tpnt1->usage_count == 1) {
					tpnt1->init_flag |= DL_OPENED;
					/* This list is for dlsym() and relocation */
					dyn_ptr->next = (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
					_dl_memset (dyn_ptr->next, 0, sizeof (struct dyn_elf));
					dyn_ptr = dyn_ptr->next;
					dyn_ptr->dyn = tpnt1;
				}
				if (tpnt1->init_flag & DL_OPENED) {
					/* Used to record RTLD_LOCAL scope */
					tmp = alloca(sizeof(struct init_fini_list));
					tmp->tpnt = tpnt1;
					tmp->next = runp->tpnt->init_fini;
					runp->tpnt->init_fini = tmp;

					for (tmp=dep_list; tmp; tmp = tmp->next) {
						if (tpnt1 == tmp->tpnt) { /* if match => cirular dependency, drop it */
							_dl_if_debug_print("Circular dependency, skipping '%s',\n",
									tmp->tpnt->libname);
							tpnt1->usage_count--;
							break;
						}
					}
					if (!tmp) { /* Don't add if circular dependency detected */
						runp2->next = alloca(sizeof(*runp));
						runp2 = runp2->next;
						runp2->tpnt = tpnt1;
						runp2->next = NULL;
					}
				}
			}
		}
	}
	init_fini_list = malloc(nlist * sizeof(struct elf_resolve *));
	dyn_chain->init_fini.init_fini = init_fini_list;
	dyn_chain->init_fini.nlist = nlist;
	i = 0;
	for (runp2 = dep_list; runp2; runp2 = runp2->next) {
		init_fini_list[i++] = runp2->tpnt;
		for(runp = runp2->tpnt->init_fini; runp; runp = runp->next){
			if (!(runp->tpnt->rtld_flags & RTLD_GLOBAL)) {
				tmp = malloc(sizeof(struct init_fini_list));
				tmp->tpnt = runp->tpnt;
				tmp->next = runp2->tpnt->rtld_local;
				runp2->tpnt->rtld_local = tmp;
			}
		}

	}
	/* Sort the INIT/FINI list in dependency order. */
	for (runp2 = dep_list; runp2; runp2 = runp2->next) {
		unsigned int j, k;
		for (j = 0; init_fini_list[j] != runp2->tpnt; ++j)
			/* Empty */;
		for (k = j + 1; k < nlist; ++k) {
			struct init_fini_list *ele = init_fini_list[k]->init_fini;

			for (; ele; ele = ele->next) {
				if (ele->tpnt == runp2->tpnt) {
					struct elf_resolve *here = init_fini_list[k];
					_dl_if_debug_print("Move %s from pos %d to %d in INIT/FINI list.\n", here->libname, k, j);
					for (i = (k - j); i; --i)
						init_fini_list[i+j] = init_fini_list[i+j-1];
					init_fini_list[j] = here;
					++j;
					break;
				}
			}
		}
	}
#ifdef __SUPPORT_LD_DEBUG__
	if(_dl_debug) {
		fprintf(stderr, "\nINIT/FINI order and dependencies:\n");
		for (i=0;i < nlist;i++) {
			fprintf(stderr, "lib: %s has deps:\n", init_fini_list[i]->libname);
			runp = init_fini_list[i]->init_fini;
			for (; runp; runp = runp->next)
				printf(" %s ", runp->tpnt->libname);
			printf("\n");
		}
	}
#endif

	_dl_if_debug_print("Beginning dlopen relocation fixups\n");
	/*
	 * OK, now all of the kids are tucked into bed in their proper addresses.
	 * Now we go through and look for REL and RELA records that indicate fixups
	 * to the GOT tables.  We need to do this in reverse order so that COPY
	 * directives work correctly */
#ifdef __mips__
	/*
	 * Relocation of the GOT entries for MIPS have to be done
	 * after all the libraries have been loaded.
	 */
	_dl_perform_mips_global_got_relocations(tpnt, !now_flag);
#endif

	if (_dl_fixup(dyn_chain, now_flag))
		goto oops;

	if (relro_ptr) {
		for (rpnt = relro_ptr->next; rpnt; rpnt = rpnt->next) {
			if (rpnt->dyn->relro_size)
				_dl_protect_relro(rpnt->dyn);
		}
	}
	/* TODO:  Should we set the protections of all pages back to R/O now ? */

#if USE_TLS
	tpnt = dyn_chain->dyn;
	/*
	 * Only add TLS memory if this object is loaded now and
	 * therefore is not yet initialized.
	 */
	if (!(tpnt->init_flag & INIT_FUNCS_CALLED)
		/* Only if the module defines thread local data. */
		&& __builtin_expect(tpnt->l_tls_blocksize > 0, 0))
	{
		/*
		 * Now that we know the object is loaded successfully the
		 * modules containing TLS data have been added to the slot
		 * info table. We might have to increase its size.
		 */
		if(tpnt->l_need_tls_init)
		{
			tpnt->l_need_tls_init = 0;

# ifdef SHARED
			/*
			 * Update the slot information data for at least the
			 * generation of the DSO we are allocating data for.
			 */
			_dl_update_slotinfo (tpnt->l_tls_modid);
# endif

			_dl_init_static_tls((struct link_map *) tpnt);

			_dl_assert(tpnt->l_need_tls_init == 0);
		}

		/* We have to bump the generation counter. */
		any_tls = true;
	}

	/* Bump the generation number if necessary.  */
	if (any_tls && __builtin_expect (++_dl_tls_generation == 0, 0)) {
		_dl_debug_early("TLS generation counter wrapped! Please report this.");
		_dl_exit(30);
	}

#endif

	/* Notify the debugger we have added some objects. */
	if (_dl_debug_addr) {
		dl_brk = (void (*)(void)) _dl_debug_addr->r_brk;
		if (dl_brk != NULL) {
			_dl_debug_addr->r_state = RT_ADD;
			(*dl_brk) ();

			_dl_debug_addr->r_state = RT_CONSISTENT;
			(*dl_brk) ();
		}
	}

#ifdef SHARED
	/* Run the ctors and setup the dtors */
	for (i = nlist; i; --i) {
		tpnt = init_fini_list[i-1];
		if (tpnt->init_flag & INIT_FUNCS_CALLED)
			continue;
		tpnt->init_flag |= INIT_FUNCS_CALLED;

		if (tpnt->dynamic_info[DT_INIT]) {
			void (*dl_elf_func) (void);
			dl_elf_func = (void (*)(void)) (tpnt->loadaddr + tpnt->dynamic_info[DT_INIT]);
			if (dl_elf_func && *dl_elf_func != NULL) {
				_dl_if_debug_print("running ctors for library %s at '%p'\n",
						tpnt->libname, dl_elf_func);
				(*dl_elf_func) ();
			}
		}

		_dl_run_init_array(tpnt);
	}
#endif /* SHARED */

	_dl_unmap_cache();
	return (void *) dyn_chain;

oops:
	/* Something went wrong.  Clean up and return NULL. */
	_dl_unmap_cache();
	do_dlclose(dyn_chain, 0);
	return NULL;
}

#ifndef SHARED
link_warning(dlopen, "libdl support is severely limited in static binaries");
#endif

void *dlsym(void *vhandle, const char *name)
{
	struct elf_resolve *tpnt, *tfrom;
	struct dyn_elf *handle;
	ElfW(Addr) from;
	struct dyn_elf *rpnt;
	void *ret;

	handle = (struct dyn_elf *) vhandle;

	/* First of all verify that we have a real handle
	   of some kind.  Return NULL if not a valid handle. */

	if (handle == NULL)
		handle = _dl_symbol_tables;
	else if (handle != RTLD_NEXT && handle != _dl_symbol_tables) {
		for (rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle)
			if (rpnt == handle)
				break;
		if (!rpnt) {
			_dl_error_number = LD_BAD_HANDLE;
			return NULL;
		}
	} else if (handle == RTLD_NEXT) {
		/*
		 * Try and locate the module we were called from - we
		 * need this so that we know where to start searching
		 * from.  We never pass RTLD_NEXT down into the actual
		 * dynamic loader itself, as it doesn't know
		 * how to properly treat it.
		 */
		from = (ElfW(Addr)) __builtin_return_address(0);

		tfrom = NULL;
		for (rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next) {
			tpnt = rpnt->dyn;
			if (tpnt->loadaddr < from
					&& (tfrom == NULL || tfrom->loadaddr < tpnt->loadaddr)) {
				tfrom = tpnt;
				handle = rpnt->next;
			}
		}
	}


#if defined(USE_TLS) && defined(SHARED)
	ret = _dl_find_hash((char*)name, handle, NULL, 1);

	if (ret)
	{
		if(ELF_ST_TYPE(_dl_tls_reloc_sym->st_info) == STT_TLS)
		{
			/*
			 * The found symbol is a thread-local storage variable.
			 * Return the address for it to the current thread.
			 */
			ret = _dl_tls_symaddr((struct link_map *) _dl_tls_reloc_tpnt,
				_dl_tls_reloc_sym);
		}
	}
	else
#else
	ret = _dl_find_hash((char*)name, handle, NULL, 0);
#endif

	/*
	 * Nothing found.
	 */
	if (!ret)
		_dl_error_number = LD_NO_SYMBOL;
	return ret;
}

#if 0
void *dlvsym(void *vhandle, const char *name, const char *version)
{
	return dlsym(vhandle, name);
}
#endif

static int do_dlclose(void *vhandle, int need_fini)
{
	struct dyn_elf *rpnt, *rpnt1, *rpnt1_tmp;
	struct init_fini_list *runp, *tmp;
	ElfW(Phdr) *ppnt;
	struct elf_resolve *tpnt, *run_tpnt;
	int (*dl_elf_fini) (void);
	void (*dl_brk) (void);
	struct dyn_elf *handle;
	unsigned int end;
	unsigned int i, j;
#if USE_TLS
	bool any_tls = false;
	size_t tls_free_start = NO_TLS_OFFSET;
	size_t tls_free_end = NO_TLS_OFFSET; 
	struct link_map *tls_lmap;
#endif

	handle = (struct dyn_elf *) vhandle;
	if (handle == _dl_symbol_tables)
		return 0;
	rpnt1 = NULL;
	for (rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle) {
		if (rpnt == handle)
			break;
		rpnt1 = rpnt;
	}

	if (!rpnt) {
		_dl_error_number = LD_BAD_HANDLE;
		return 1;
	}
	if (rpnt1)
		rpnt1->next_handle = rpnt->next_handle;
	else
		_dl_handles = rpnt->next_handle;
	_dl_if_debug_print("%s: usage count: %d\n",
			handle->dyn->libname, handle->dyn->usage_count);
	if (handle->dyn->usage_count != 1) {
		handle->dyn->usage_count--;
		free(handle);
		return 0;
	}
	/* OK, this is a valid handle - now close out the file */
	for (j = 0; j < handle->init_fini.nlist; ++j) {
		tpnt = handle->init_fini.init_fini[j];
		if (--tpnt->usage_count == 0) {
			if ((tpnt->dynamic_info[DT_FINI]
			     || tpnt->dynamic_info[DT_FINI_ARRAY])
			    && need_fini &&
			    !(tpnt->init_flag & FINI_FUNCS_CALLED)) {
				tpnt->init_flag |= FINI_FUNCS_CALLED;
#ifdef SHARED
				_dl_run_fini_array(tpnt);
#endif

				if (tpnt->dynamic_info[DT_FINI]) {
					dl_elf_fini = (int (*)(void)) (tpnt->loadaddr + tpnt->dynamic_info[DT_FINI]);
					_dl_if_debug_print("running dtors for library %s at '%p'\n",
							tpnt->libname, dl_elf_fini);
					(*dl_elf_fini) ();
				}
			}

			_dl_if_debug_print("unmapping: %s\n", tpnt->libname);
			end = 0;
			for (i = 0, ppnt = tpnt->ppnt;
					i < tpnt->n_phent; ppnt++, i++) {
				if (ppnt->p_type != PT_LOAD)
					continue;
				if (end < ppnt->p_vaddr + ppnt->p_memsz)
					end = ppnt->p_vaddr + ppnt->p_memsz;
			}

#if USE_TLS
			/* Do the cast to make things easy. */
			tls_lmap = (struct link_map *) tpnt;

			/* Remove the object from the dtv slotinfo array if it uses TLS. */
			if (__builtin_expect (tls_lmap->l_tls_blocksize > 0, 0))
			{
				any_tls = true;

				if (_dl_tls_dtv_slotinfo_list != NULL
						&& ! remove_slotinfo (tls_lmap->l_tls_modid,
						_dl_tls_dtv_slotinfo_list, 0,
						(tpnt->init_flag & INIT_FUNCS_CALLED)))
					/* All dynamically loaded modules with TLS are unloaded. */
					_dl_tls_max_dtv_idx = _dl_tls_static_nelem;

				if (tls_lmap->l_tls_offset != NO_TLS_OFFSET)
				{
					/*
					 * Collect a contiguous chunk built from the objects in
					 * this search list, going in either direction.  When the
					 * whole chunk is at the end of the used area then we can
					 * reclaim it.
					 */
# if TLS_TCB_AT_TP
					if (tls_free_start == NO_TLS_OFFSET
						|| (size_t) tls_lmap->l_tls_offset == tls_free_start)
					{
						/* Extend the contiguous chunk being reclaimed. */
						tls_free_start
							= tls_lmap->l_tls_offset -
							  tls_lmap->l_tls_blocksize;

						if (tls_free_end == NO_TLS_OFFSET)
							tls_free_end = tls_lmap->l_tls_offset;
					}
					else if (tls_lmap->l_tls_offset - tls_lmap->l_tls_blocksize
							== tls_free_end)
						/* Extend the chunk backwards.  */
						tls_free_end = tls_lmap->l_tls_offset;
					else
					{
						/*
						 * This isn't contiguous with the last chunk freed.
						 * One of them will be leaked unless we can free
						 * one block right away.
						 */
						if (tls_free_end == _dl_tls_static_used)
						{
							_dl_tls_static_used = tls_free_start;
							tls_free_end = tls_lmap->l_tls_offset;
							tls_free_start
								= tls_free_end - tls_lmap->l_tls_blocksize;
						}
						else if ((size_t) tls_lmap->l_tls_offset
								== _dl_tls_static_used)
							_dl_tls_static_used = tls_lmap->l_tls_offset -
								tls_lmap->l_tls_blocksize;
						else if (tls_free_end < (size_t) tls_lmap->l_tls_offset)
						{
							/*
							 * We pick the later block. It has a chance
							 * to be freed.
							 */
							tls_free_end = tls_lmap->l_tls_offset;
							tls_free_start = tls_free_end -
								tls_lmap->l_tls_blocksize;
						}
					}
# elif TLS_DTV_AT_TP
					if ((size_t) tls_lmap->l_tls_offset == tls_free_end)
						/* Extend the contiguous chunk being reclaimed. */
						tls_free_end -= tls_lmap->l_tls_blocksize;
					else if (tls_lmap->l_tls_offset + tls_lmap->l_tls_blocksize
							== tls_free_start)
						/* Extend the chunk backwards. */
						tls_free_start = tls_lmap->l_tls_offset;
					else
					{
						/*
						 * This isn't contiguous with the last chunk
						 * freed. One of them will be leaked.
						 */
						if (tls_free_end == _dl_tls_static_used)
							_dl_tls_static_used = tls_free_start;
						tls_free_start = tls_lmap->l_tls_offset;
						tls_free_end = tls_free_start +
							tls_lmap->l_tls_blocksize;
					}
# else
#  error "Either TLS_TCB_AT_TP or TLS_DTV_AT_TP must be defined"
# endif
				}
			}
#endif

			_dl_munmap((void*)tpnt->loadaddr, end);
			/* Free elements in RTLD_LOCAL scope list */ 
			for (runp = tpnt->rtld_local; runp; runp = tmp) {
				tmp = runp->next;
				free(runp);
			}

			/* Next, remove tpnt from the loaded_module list */
			if (_dl_loaded_modules == tpnt) {
				_dl_loaded_modules = tpnt->next;
				if (_dl_loaded_modules)
					_dl_loaded_modules->prev = 0;
			} else
				for (run_tpnt = _dl_loaded_modules; run_tpnt; run_tpnt = run_tpnt->next)
					if (run_tpnt->next == tpnt) {
						_dl_if_debug_print("removing loaded_modules: %s\n", tpnt->libname);
						run_tpnt->next = run_tpnt->next->next;
						if (run_tpnt->next)
							run_tpnt->next->prev = run_tpnt;
						break;
					}

			/* Next, remove tpnt from the global symbol table list */
			if (_dl_symbol_tables) {
				if (_dl_symbol_tables->dyn == tpnt) {
					_dl_symbol_tables = _dl_symbol_tables->next;
					if (_dl_symbol_tables)
						_dl_symbol_tables->prev = 0;
				} else
					for (rpnt1 = _dl_symbol_tables; rpnt1->next; rpnt1 = rpnt1->next) {
						if (rpnt1->next->dyn == tpnt) {
							_dl_if_debug_print("removing symbol_tables: %s\n", tpnt->libname);
							rpnt1_tmp = rpnt1->next->next;
							free(rpnt1->next);
							rpnt1->next = rpnt1_tmp;
							if (rpnt1->next)
								rpnt1->next->prev = rpnt1;
							break;
						}
					}
			}
			free(tpnt->libname);
			free(tpnt);
		}
	}
	free(handle->init_fini.init_fini);
	free(handle);

#if USE_TLS
	/* If we removed any object which uses TLS bump the generation counter.  */
	if (any_tls)
	{
		if (__builtin_expect (++_dl_tls_generation == 0, 0))
		{
			_dl_debug_early ("TLS generation counter wrapped!  Please report to the uClibc mailing list.\n");
			_dl_exit(30);
		}

		if (tls_free_end == _dl_tls_static_used)
			_dl_tls_static_used = tls_free_start;
	}
#endif

	if (_dl_debug_addr) {
		dl_brk = (void (*)(void)) _dl_debug_addr->r_brk;
		if (dl_brk != NULL) {
			_dl_debug_addr->r_state = RT_DELETE;
			(*dl_brk) ();

			_dl_debug_addr->r_state = RT_CONSISTENT;
			(*dl_brk) ();
		}
	}

	return 0;
}

int dlclose(void *vhandle)
{
	return do_dlclose(vhandle, 1);
}

char *dlerror(void)
{
	const char *retval;

	if (!_dl_error_number)
		return NULL;
	retval = dl_error_names[_dl_error_number];
	_dl_error_number = 0;
	return (char *)retval;
}

/*
 * Dump information to stderrr about the current loaded modules
 */
#ifdef __USE_GNU
static char *type[] = { "Lib", "Exe", "Int", "Mod" };

int dlinfo(void)
{
	struct elf_resolve *tpnt;
	struct dyn_elf *rpnt, *hpnt;

	fprintf(stderr, "List of loaded modules\n");
	/* First start with a complete list of all of the loaded files. */
	for (tpnt = _dl_loaded_modules; tpnt; tpnt = tpnt->next) {
		fprintf(stderr, "\t%p %p %p %s %d %s\n",
		        tpnt->loadaddr, tpnt, tpnt->symbol_scope,
		        type[tpnt->libtype],
		        tpnt->usage_count, tpnt->libname);
	}

	/* Next dump the module list for the application itself */
	fprintf(stderr, "\nModules for application (%p):\n", _dl_symbol_tables);
	for (rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next)
		fprintf(stderr, "\t%p %s\n", rpnt->dyn, rpnt->dyn->libname);

	for (hpnt = _dl_handles; hpnt; hpnt = hpnt->next_handle) {
		fprintf(stderr, "Modules for handle %p\n", hpnt);
		for (rpnt = hpnt; rpnt; rpnt = rpnt->next)
			fprintf(stderr, "\t%p %s\n", rpnt->dyn, rpnt->dyn->libname);
	}
	return 0;
}

int dladdr(const void *__address, Dl_info * __info)
{
	struct elf_resolve *pelf;
	struct elf_resolve *rpnt;

	_dl_map_cache();

	/*
	 * Try and locate the module address is in
	 */
	pelf = NULL;

#if 0
	fprintf(stderr, "dladdr( %p, %p )\n", __address, __info);
#endif

	for (rpnt = _dl_loaded_modules; rpnt; rpnt = rpnt->next) {
		struct elf_resolve *tpnt;

		tpnt = rpnt;
#if 0
		fprintf(stderr, "Module \"%s\" at %p\n",
				tpnt->libname, tpnt->loadaddr);
#endif
		if (tpnt->loadaddr < (ElfW(Addr)) __address
				&& (pelf == NULL || pelf->loadaddr < tpnt->loadaddr)) {
			pelf = tpnt;
		}
	}

	if (!pelf) {
		return 0;
	}

	/*
	 * Try and locate the symbol of address
	 */

	{
		char *strtab;
		ElfW(Sym) *symtab;
		unsigned int hn, si, sn, sf;
		ElfW(Addr) sa;

		sa = 0;
		symtab = (ElfW(Sym) *) (pelf->dynamic_info[DT_SYMTAB]);
		strtab = (char *) (pelf->dynamic_info[DT_STRTAB]);

		sf = sn = 0;
		for (hn = 0; hn < pelf->nbucket; hn++) {
			for (si = pelf->elf_buckets[hn]; si; si = pelf->chains[si]) {
				ElfW(Addr) symbol_addr;

				symbol_addr = pelf->loadaddr + symtab[si].st_value;
				if (symbol_addr <= (ElfW(Addr))__address && (!sf || sa < symbol_addr)) {
					sa = symbol_addr;
					sn = si;
					sf = 1;
				}
#if 0
				fprintf(stderr, "Symbol \"%s\" at %p\n",
						strtab + symtab[si].st_name, symbol_addr);
#endif
			}
		}

		if (sf) {
			__info->dli_fname = pelf->libname;
			__info->dli_fbase = (void *)pelf->loadaddr;
			__info->dli_sname = strtab + symtab[sn].st_name;
			__info->dli_saddr = (void *)sa;
		}
		return 1;
	}
}
#endif
