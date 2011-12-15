/*
 *
 * Copyright (c) 2002-2005 Broadcom Corporation 
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
 *
*/

/**************************************************************************
 * File Name  : boardparms.c
 *
 * Description: This file contains the implementation for the BCM97xxx board
 *              parameter access functions.
 * 
 * Updates    : 07/14/2003  Created.
 ***************************************************************************/

/* Includes. */
#include "boardparms.h"

/* Typedefs */
typedef struct boardparameters
{
    char szBoardId[BP_BOARD_ID_LEN];        /* board id string */
    ETHERNET_MAC_INFO EnetMacInfo;
} BOARD_PARAMETERS, *PBOARD_PARAMETERS;

/* Variables */
static BOARD_PARAMETERS g_bcm97401_cfg0 =
{
    "INT_PHY",                              /* szBoardId */
    {BP_ENET_INTERNAL_PHY,                  /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      0x01,                                 /* numSwitchPorts */
      BP_NOT_DEFINED,                       /* usManagementSwitch */
      BP_ENET_CONFIG_MDIO,                  /* usConfigType */
      BP_NOT_DEFINED},                      /* usReverseMii */
};

static BOARD_PARAMETERS g_bcm97401_cfg1 =
{
    "EXT_5325_MANAGEMENT",                  /* szBoardId */
    {BP_ENET_EXTERNAL_SWITCH,               /* ucPhyType */
      0x00,                                 /* ucPhyAddress */
      0x02,                                 /* numSwitchPorts */
      BP_ENET_MANAGEMENT_PORT,              /* usManagementSwitch */
      BP_ENET_CONFIG_MDIO_PSEUDO_PHY,       /* usConfigType */
      BP_ENET_REVERSE_MII},                 /* usReverseMii */
};

#ifdef CONFIG_BCMINTEMAC_7038_EXTMII
static BOARD_PARAMETERS g_bcm97401_cfg2 =
{
    "EXT_PHY",				    /* szBoardId */
    {BP_ENET_EXTERNAL_PHY,                  /* ucPhyType */
      0x01,                                 /* ucPhyAddress */
      0x01,                                 /* numSwitchPorts */
      BP_NOT_DEFINED,                       /* usManagementSwitch */
      BP_ENET_CONFIG_MDIO,                  /* usConfigType */
      BP_NOT_DEFINED},                      /* usReverseMii */
};
#endif

static PBOARD_PARAMETERS g_BoardParms[] =
    {&g_bcm97401_cfg0, &g_bcm97401_cfg1,
#ifdef CONFIG_BCMINTEMAC_7038_EXTMII
    &g_bcm97401_cfg2,
#endif
    0};


static PBOARD_PARAMETERS g_pCurrentBp = 0;

/**************************************************************************
 * Name       : bpstrcmp
 *
 * Description: String compare for this file so it does not depend on an OS.
 *              (Linux kernel and CFE share this source file.)
 *
 * Parameters : [IN] dest - destination string
 *              [IN] src - source string
 *
 * Returns    : -1 - dest < src, 1 - dest > src, 0 dest == src
 ***************************************************************************/
static int bpstrcmp(const char *dest,const char *src);
static int bpstrcmp(const char *dest,const char *src)
{
    while (*src && *dest)
    {
        if (*dest < *src) return -1;
        if (*dest > *src) return 1;
        dest++;
        src++;
    }

    if (*dest && !*src) return 1;
    if (!*dest && *src) return -1;
    return 0;
} /* bpstrcmp */


/**************************************************************************
 * Name       : BpSetBoardId
 *
 * Description: This function find the BOARD_PARAMETERS structure for the
 *              specified board id string and assigns it to a global, static
 *              variable.
 *
 * Parameters : [IN] pszBoardId - Board id string that is saved into NVRAM.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_FOUND - Error, board id input string does not
 *                  have a board parameters configuration record.
 ***************************************************************************/
int BpSetBoardId( char *pszBoardId )
{
    int nRet = BP_BOARD_ID_NOT_FOUND;
    PBOARD_PARAMETERS *ppBp;

    for( ppBp = g_BoardParms; *ppBp; ppBp++ )
    {
        if( !bpstrcmp((*ppBp)->szBoardId, pszBoardId) )
        {
            g_pCurrentBp = *ppBp;
            nRet = BP_SUCCESS;
            break;
        }
    }

    return( nRet );
} /* BpSetBoardId */

/**************************************************************************
 * Name       : BpGetBoardIds
 *
 * Description: This function returns all of the supported board id strings.
 *
 * Parameters : [OUT] pszBoardIds - Address of a buffer that the board id
 *                  strings are returned in.  Each id starts at BP_BOARD_ID_LEN
 *                  boundary.
 *              [IN] nBoardIdsSize - Number of BP_BOARD_ID_LEN elements that
 *                  were allocated in pszBoardIds.
 *
 * Returns    : Number of board id strings returned.
 ***************************************************************************/
int BpGetBoardIds( char *pszBoardIds, int nBoardIdsSize )
{
    PBOARD_PARAMETERS *ppBp;
    int i;
    char *src;
    char *dest;

    for( i = 0, ppBp = g_BoardParms; *ppBp && nBoardIdsSize;
        i++, ppBp++, nBoardIdsSize--, pszBoardIds += BP_BOARD_ID_LEN )
    {
        dest = pszBoardIds;
        src = (*ppBp)->szBoardId;
        while( *src )
            *dest++ = *src++;
        *dest = '\0';
    }

    return( i );
} /* BpGetBoardIds */

/**************************************************************************
 * Name       : BpGetEthernetMacInfo
 *
 * Description: This function returns all of the supported board id strings.
 *
 * Parameters : [OUT] pEnetInfos - Address of an array of ETHERNET_MAC_INFO
 *                  buffers.
 *
 * Returns    : BP_SUCCESS - Success, value is returned.
 *              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
 ***************************************************************************/
int BpGetEthernetMacInfo( PETHERNET_MAC_INFO pEnetInfos )
{
    int nRet;

    if( g_pCurrentBp )
    {
        unsigned char *src = (unsigned char *)
            &g_pCurrentBp->EnetMacInfo;
        unsigned char *dest = (unsigned char *) pEnetInfos;
        int len = sizeof(ETHERNET_MAC_INFO);
        while( len-- )
            *dest++ = *src++;

        nRet = BP_SUCCESS;
    }
    else
    {
        pEnetInfos->ucPhyType = BP_ENET_NO_PHY;

        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetEthernetMacInfo */
