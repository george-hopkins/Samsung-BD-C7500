typedef enum _MSD_OPEN_MODE
{
	MSD_OPEN_FILE,
	MSD_OPEN_BUFFER,
	MSD_OPEN_RECORD
} MSD_OPEN_MODE;

typedef enum _MSD_DLA_TYPE
{
	MSD_SILENT_DLA,
	MSD_NON_SILENT_DLA,
	MSD_NO_DLA
} MSD_DLA_TYPE;

typedef enum  _MSD_SEEK_POS
{
	MSD_POS_BEG,
	MSD_POS_CUR,
	MSD_POS_END,
	MSD_POS_RESET
} MSD_SEEK_POS;

typedef enum  _MSD_RET
{
	MSD_SUCCESS = 0,
	MSD_ERROR_MEMALLOC = 0x3001,
	MSD_ERROR_NODATA,
	MSD_ERROR_CANNOT_ACCESSROOTDIR,
	MSD_ERROR_CANNOT_FINDPKG,
	MSD_ERROR_INVALID_MEMBER,
	MSD_ERROR_NOT_SUPPORTED_FUNC = 0x3100,
	MSD_ERROR_INVALID_ARGS,
	MSD_ERROR_DLA_FAILURE,
	MSD_ERROR_INSTALL_LICENSE,
	MSD_ERROR_DELETE_LICENSE,
	MSD_ERROR_NO_LICENSE,
	MSD_ERROR_NO_RIGHTS,
	MSD_ERROR_INVALID_HANDLE,
	MSD_ERROR_ALREADY_OPEN,
	MSD_ERROR_CANNOT_OPEN,
	MSD_ERROR_READ_FAILURE,
	MSD_ERROR_SEEK_FAILURE
} MSD_RET;

typedef struct  _MSD_DRM_STATUS 
{
	MSD_OPEN_MODE	m_eOpenMode;
	MSD_DLA_TYPE	m_eDLAType;
	unsigned int	m_nMaxPacket;
	void* m_pAsfHeader;
	unsigned int m_nAsfHeader;
	unsigned int m_nNumOfPackets;
} MSD_DRM_STATUS;

typedef struct  _MSD_RIGHTS_INFO
{
	unsigned int  m_nAvailableCount;
	// will be added. date info. keyid, license id, url, etc.
} MSD_RIGHTS_INFO;

extern int WR_get_MSD_Interface(char *cpName);
extern unsigned int WR_wmdrm_GetVersion(void);
extern unsigned char* WR_wmdrm_GetUID(void);
extern unsigned char* WR_wmdrm_GetDRMName(void);
extern int WR_wmdrm_Setup(void);
extern int WR_wmdrm_Open(unsigned char* pURI, MSD_OPEN_MODE eMode, MSD_DLA_TYPE  eType);
extern int WR_wmdrm_Close(void);
extern int WR_wmdrm_Read(unsigned char *pInBuf, unsigned int nIsize, unsigned char *pOutBuf, unsigned char *state);
extern int WR_wmdrm_Record(unsigned char *pIn, unsigned int nInLen, unsigned char *pOut);
extern int WR_wmdrm_Seek( MSD_SEEK_POS ePos, unsigned int nOffset);
extern int WR_wmdrm_Reset(void);
extern int WR_wmdrm_Status(MSD_DRM_STATUS *pStatus);
extern int WR_wmdrm_GetLicense(void** challenge, unsigned int* challenge_Len);
extern int WR_wmdrm_Activation(void* pInMsg, void* pOutMsg);
extern int WR_wmdrm_InstallLicense(void* pInLic, unsigned int nInLicLen);
extern int WR_wmdrm_DeleteLicense(void* pInMsg, unsigned int nInMsgLen);
extern int WR_wmdrm_CleanAllLicense(void);
extern int WR_wmdrm_CleanInvalidLicense(void);
extern int WR_wmdrm_HasDeviceRightsToPlay(void* pInMsg, unsigned int nInMsgLen);
extern int WR_wmdrm_GetRightsInfo(unsigned char* pMacro4NFX, unsigned char* pCGMS4NFX, unsigned char* pICT4NFX);
extern int WR_wmdrm_ConsumeRights(void* pInMsg, unsigned int  nInMsgLen);
extern int WR_wmdrm_GetAvailableCount(unsigned int* pnCount, void* pInMsg, unsigned int  nInMsgLen);
extern int WR_wmdrm_GetAvailableDate(unsigned char* pDate, void* pInMsg, unsigned int  nInMsgLen);
extern int WR_wmdrm_GetMetadata(void** pOutMetadata);
extern int WR_wmdrm_GetKey(void** pOutKey);
extern int WR_wmdrm_DoMetering(unsigned char* pURL, MSD_DLA_TYPE  eType);
extern int WR_wmdrm_SetSecureClock(unsigned char* pURL);
extern int WR_wmdrm_CreateDomain(void* pData);
extern int WR_wmdrm_DeleteDomain(void* pData);
extern int WR_wmdrm_JoinDomain(void* pData);
extern int WR_wmdrm_LeaveDomain(void* pData);

