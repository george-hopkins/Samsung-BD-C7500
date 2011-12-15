

#ifdef __cplusplus
extern "C" {
#endif


#define EBADBLK     165  /*Bad block was found*/
//#define MEMWRITEOOBFREE         _IOWR('M', 18, struct mtd_oob_buf)


typedef void (*callbackFunc)(
                void *callback_context, 
                size_t total_input_bytes, 
                bool   erasing, 
                size_t bytes_erased_so_far, 
                bool   writing, 
                size_t bytes_written_so_far,
                size_t total_device_space);

typedef struct nand_write_param 
{
    const  char  *mtd_device;
    const  char  *img;
    int    mtdoffset;
    int    quiet;
    int    writeoob;
    int    autoplace;
    int    forcejffs2;
    int    forceyaffs;
    int    forcelegacy;
    int    noecc;
    int    pad;
    int    blockalign;
} NAND_WRITE_PARAM;


typedef struct nand_write_extra_param
{
    int bErase;
}NAND_WRITE_EXTRA_PARAM;

typedef struct nand_erase_param 
{
    const  char  *mtd_device;
    int    quiet;
    int    jffs2;
}NAND_ERASE_PARAM;

// Prototypes
int nandutils_setExtraParms(struct nand_write_extra_param* p_ExtraParam);
int nandutils_nandwrite(NAND_WRITE_PARAM *param, void *callback_context, callbackFunc callback);
int nandutils_flash_erase(NAND_ERASE_PARAM *param, void *callback_context, callbackFunc callback);

// Inline
inline int INVALID_PAGE_SIZE(int oobsize, int writesize)
{
        if ( !(oobsize == 8 && writesize == 256) &&
             !(oobsize == 16 && writesize == 512) && 
             !(oobsize == 64 && writesize == 2048)&& 
             !(oobsize == 128 && writesize == 4096))
        {
            return 1;
        }
        else
        {
            return 0;
        }
}

#ifdef __cplusplus
}
#endif

