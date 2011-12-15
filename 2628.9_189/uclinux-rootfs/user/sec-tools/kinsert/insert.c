/*
 * Insert a public_key set into the .digsig_keys section of a digsig enabled
 * kernel.
 *
 * The elf processing code is based on the tutorial "libelf by Example".
 * http://people.freebsd.org/~jkoshy/download/libelf/article.html
 * There's a decent ELF overview at 
 * http://www.cs.bgu.ac.il/~arch041/tirgul7/elf.ppt
 * There's lots of code to print elf, program, and section headers.
 * The code's not used in production but is very usefull for debugging.
 */

#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <string.h>

typedef __off64_t off64_t;
#include <gelf.h> 

extern char *progname;
extern int debug;

void dump_key(unsigned char *msg, unsigned char *key, int len);

 
char *
get_ptype(size_t pt)
{
        char *s;

        switch (pt) {
            case PT_NULL:
	    	s = "NULL";
		break;
	
	    case PT_LOAD:
	    	s = "LOAD";
		break;

	    case PT_DYNAMIC:
	    	s = "DYNAMIC";
		break;

            case PT_INTERP:
	    	s = "INTERP";
		break;

	    case PT_NOTE:
	    	s = "NOTE";
		break;

	    case PT_SHLIB:
	    	s = "SHLIB";
		break;

            case PT_PHDR:
	    	s = "PHDR";
		break;

	    case PT_TLS:
	    	s = "TLS";
		break;

        default:
                s = "unknown";
                break;
        }
        return(s);
}

void 
print_elf_hdr( GElf_Ehdr *ehdr)
{
	int i;
	char *id;

#define        PRINT_FIELD(N) do {                                        \
                (void) printf("    %-20s 0x%jx\n" ,#N, (uintmax_t) ehdr->N);      \
        } while (0)

        PRINT_FIELD(e_type);
        PRINT_FIELD(e_machine);
        PRINT_FIELD(e_version);
        PRINT_FIELD(e_entry);
        PRINT_FIELD(e_phoff);
        PRINT_FIELD(e_shoff);
        PRINT_FIELD(e_flags);
        PRINT_FIELD(e_shnum);
        PRINT_FIELD(e_ehsize);
        PRINT_FIELD(e_phnum);
        PRINT_FIELD(e_phentsize);
}

void
print_phdr( Elf *e)
{
	int i;
	GElf_Ehdr ehdr;
        GElf_Phdr phdr;

        if (gelf_getehdr(e, &ehdr) == NULL) {
		fprintf(stderr, " print_phdr: gel_getehdr failed: %s.\n",
                    elf_errmsg(-1));
		return;
	}

	printf("PH type		type	offset		vaddr	  	paddr	  	filesz		memsz		flags	align\n");
        for (i = 0; i < ehdr.e_phnum; i++) {
                if (gelf_getphdr(e, i, &phdr) != &phdr)
                        fprintf(stderr,  "getphdr() failed: %s.\n",
                            elf_errmsg(-1));

	//	i: type, type 	offset	vaddr	paddr	filesz	memsz	flags (rwx)	align
	printf("%d: %08x %8s 	0x%08x	0x%08x	0x%08x	0x%08x	0x%08x	0x%x %c%c%c	0x%08x\n",
		i,
		(int)phdr.p_type,
		get_ptype(phdr.p_type),
                (int)phdr.p_offset,
                (int)phdr.p_vaddr,
                (int)phdr.p_paddr,
                (int)phdr.p_filesz,
                (int)phdr.p_memsz,
                (int)phdr.p_flags,
                (phdr.p_flags & PF_R)?'r':'-',
                (phdr.p_flags & PF_W)?'w':'-',
                (phdr.p_flags & PF_X)?'x':'-',
                (int)phdr.p_align);  
        }
}

void
print_shdr( Elf *e)
{
	int i;
	GElf_Ehdr	ehdr;

	Elf_Scn		*scn;
        Elf_Data	*data;
        GElf_Shdr	shdr;
        size_t		shstrndx, sz;
	char		*name, *p, *pc;

        if (gelf_getehdr(e, &ehdr) == NULL) {
		fprintf(stderr, " print_phdr: gel_getehdr failed: %s.\n",
                    elf_errmsg(-1));
		return;
	}

        if (elf_getshstrndx(e, &shstrndx) == -1)
                errx(EX_SOFTWARE, "getshstrndx() failed: %s.",
                    elf_errmsg(-1));

        scn = NULL;
        while ((scn = elf_nextscn(e, scn)) != NULL) {
                if (gelf_getshdr(scn, &shdr) != &shdr)
                        errx(EX_SOFTWARE, "getshdr() failed: %s.",
                            elf_errmsg(-1));

                if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
                        errx(EX_SOFTWARE, "elf_strptr() failed: %s.",
                            elf_errmsg(-1));

                (void) printf("Section %-4.4jd %s\n", (uintmax_t) elf_ndxscn(scn),
                    name);
        }


        if ((scn = elf_getscn(e, shstrndx)) == NULL) 
                errx(EX_SOFTWARE, "getscn() failed: %s.",
                    elf_errmsg(-1));

        if (gelf_getshdr(scn, &shdr) != &shdr)
                errx(EX_SOFTWARE, "getshdr(shstrndx) failed: %s.",
                    elf_errmsg(-1));

        (void) printf(".shstrab: size=%jd\n", (uintmax_t) shdr.sh_size);

        data = NULL;
	i = 0;

        while (i < shdr.sh_size && (data = elf_getdata(scn, data)) != NULL) {
                p = (char *) data->d_buf;
                while (p < (char *) data->d_buf + data->d_size) {
                                printf("%c", p[0]);
                        i++;
			p++;
                        (void) putchar((i % 16) ? ' ' : '\n');
                }
        }
        (void) putchar('\n');

}

/*
 * This has to match up with the kernel
 * This should be in a common include file.
 */
#define N_KEY_MAX	260
#define E_KEY_MAX	16
struct digsig_keys {
        int             valid;
        unsigned int    n_key_max;
        unsigned int    e_key_max;
        unsigned char   n_key[N_KEY_MAX];
        unsigned char   e_key[E_KEY_MAX];
} new_keys;


extern unsigned char  *n_key, *e_key;
extern int      n_key_len, e_key_len;

/*
 * Just seek to the correct offset in the target file
 * and overwrite the existing keys with the new ones.
 *
 * Originally, I used elf_update to write back the image after
 * modifiying the data section.  However, the elf library and
 * the linker used for the kernel are out of sync and treat alignment
 * a bit differently.  This caused the target image to be corrupted.
 */
int
insert_keys(int fd, int offset, int size, struct digsig_keys *tkp)
{

	/* 
         * do some size consistency checks between the tool, the target kernel,
	 * and the keys.
	 */
	if (n_key_len > N_KEY_MAX) {
	    fprintf(stderr,"n_key len %d greater than max %d\n",
		    n_key_len, N_KEY_MAX);
	    return(1);
	}

	if (e_key_len > E_KEY_MAX) {
	    fprintf(stderr,"e_key len %d greater than max %d\n",
		    e_key_len, E_KEY_MAX);
	    return(1);
	}

	if (tkp->n_key_max != N_KEY_MAX) {
	    fprintf(stderr,"kernel n_key max len %d not equal to tools max len: %d\n",
		    tkp->n_key_max, N_KEY_MAX);
	    return(1);
	}

	if (tkp->e_key_max != E_KEY_MAX) {
	    fprintf(stderr,"kernel e_key max len %d not equal to tools max len: %d\n",
		    tkp->e_key_max, E_KEY_MAX);
	    return(1);
	}


	if (debug)
	    printf("insert n_key, length %d\n", n_key_len);
	memcpy(new_keys.n_key, n_key, n_key_len);

	if (debug)
	    printf("insert e_key, length %d\n", e_key_len);
	memcpy(new_keys.e_key, e_key, e_key_len);

	new_keys.valid = 1;
	new_keys.n_key_max = tkp->n_key_max;
	new_keys.e_key_max = tkp->e_key_max;

	if (lseek(fd, offset, SEEK_SET) == -1) {
		perror("lseek failed on target");
		return(1);
	}

	if (write(fd, &new_keys, sizeof(new_keys)) != sizeof(new_keys)) {
		perror("write failed on target");
		return(1);
	}

	return(0);
}

/*
 * find the .digsig_keys section in the target
 */
process_digsig_section(Elf *e, int fd)
{
	int		i, index;
	GElf_Ehdr	ehdr;

	Elf_Scn		*scn;
        Elf_Data	*data;
        GElf_Shdr	shdr;
        size_t 		shstrndx, sz;
	char		*name, *p, *pc;

	struct digsig_keys *keys;

        if (gelf_getehdr(e, &ehdr) == NULL) {
		printf(" print_phdr: gel_getehdr failed: %s.\n",
                    elf_errmsg(-1));
		return;
	}

        if (elf_getshstrndx(e, &shstrndx) == -1)
                errx(EX_SOFTWARE, "getshstrndx() failed: %s.",
                    elf_errmsg(-1));

        scn = NULL;
        while ((scn = elf_nextscn(e, scn)) != NULL) {
                if (gelf_getshdr(scn, &shdr) != &shdr)
                        errx(EX_SOFTWARE, "getshdr() failed: %s.",
                            elf_errmsg(-1));

                if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL)
                        errx(EX_SOFTWARE, "elf_strptr() failed: %s.",
                            elf_errmsg(-1));


		if (strcmp(name, ".digsig_keys") == 0) {
			break;
		}
        }
	if (scn == NULL) {
		printf("%s: could not find digsig section, FAILED\n", __FUNCTION__);
		return;
	}

	index = (uintmax_t) elf_ndxscn(scn);

	//printf("found section %s\n", name);
	if (debug)
	    printf("Section %-4.4d %s size 0x%x (%d)\n", index, name, (int)shdr.sh_size, (int)shdr.sh_size);

	/*
	 * find data in section
	 * There's only one data element
         */
        data = NULL;
	i = 0;

        while ((i < shdr.sh_size) && ((data = elf_getdata(scn, data)) != NULL)) {
		int k;
#if 0
		printf("%d: type: 0x%x size 0x%x offset 0x%x align 0x%x ptr 0x%x\n",
			i,
			data->d_type,
			(int)data->d_size,
			(int)data->d_off,
			(int)data->d_align,
			(int)data->d_buf);
#endif
		i +=  (int)data->d_size;
		
		keys = (struct digsig_keys *) data->d_buf;
		if (debug)
		    printf("valid: 0x%x\n", keys->valid);

		if (keys->valid  == 1) {
			printf("Target already has keys inserted.\n");
			return;
		}
		if (keys->valid  != 0xcafe1234)
			continue;
		
		if (debug)
		    printf("vaddr offset size align\n 0x%08x 0x%08x 0x%08x 0x%08x\n",
			(int)shdr.sh_addr,
			(int)shdr.sh_offset,
			(int)shdr.sh_size,
			(int)shdr.sh_addralign);

		//dump_key("orig n_key", keys->n_key, 260);
		//dump_key("orig e_key", keys->e_key, 16);

		if (insert_keys(fd, (int)shdr.sh_offset,(int)shdr.sh_size, keys) == 0) {
		    printf("Target updated with new keys\n");
		    break;
		}
		
		printf("Failed to update target with new keys\n");
		break;
		
        }
	if (data == NULL) {
		printf("Failed to udpate target with new keys, can't find data\n");
		return;
	}

#if 0
	if (elf_update(e, ELF_C_WRITE) < 0) 
	    printf("elf_update() failed: %s.", elf_errmsg(-1));
#endif
}


/*
 * open the target file and init the elf library
 */
int
update_target(char *target)
{
        int i, fd;
        Elf *e;
        char *id, bytes[5];
        size_t n;
        GElf_Ehdr ehdr;
        GElf_Phdr phdr;



        if (elf_version(EV_CURRENT) == EV_NONE) {
                fprintf(stderr, " library initialization failed: %s\n",
                    elf_errmsg(-1));
		return(1);
	}

        if ((fd = open(target, O_RDWR, 0)) < 0) {
                fprintf(stderr, "open \"%s\" failed", target);
		perror(" ");
		return(1);
	}


        if ((e = elf_begin(fd, ELF_C_RDWR, NULL)) == NULL) {
                fprintf(stderr, " failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

        if (elf_kind(e) != ELF_K_ELF) {
                fprintf(stderr, "%s is not an ELF object.\n", target);
		return(1);
	}


        if (elf_getshnum(e, &n) == -1) {
                fprintf(stderr, "elf_getshnum failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

        // printf( "shnum: 0x%jx\n", (uintmax_t) n);

        if (elf_getshstrndx(e, &n) == -1)  {
                fprintf(stderr, "elf_getshstrndx failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

//      printf("shstrndx: 0x%jx\n", (uintmax_t) n);

        if (gelf_getehdr(e, &ehdr) == NULL) {
		fprintf(stderr, " failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

        if ((i = gelf_getclass(e)) == ELFCLASSNONE) {
                fprintf(stderr, " failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

//        printf("%s: %d-bit ELF object\n", target,
//            i == ELFCLASS32 ? 32 : 64);

        if ((id = elf_getident(e, NULL)) == NULL)  {
                fprintf(stderr, " failed: %s.\n",
                    elf_errmsg(-1));
		return(1);
	}

//     printf("%3s e_ident[0..%1d] %7s", " ", EI_ABIVERSION, "\n");

	//print_elf_hdr(&ehdr);
	//print_phdr(e);
	//print_shdr(e);
	process_digsig_section(e, fd);

        (void) elf_end(e);
        (void) close(fd);
        return(0);
}
