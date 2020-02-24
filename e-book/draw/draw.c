#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <config.h>
#include <draw.h>
#include <encoding_manager.h>
#include <fonts_manager.h>
#include <disp_manager.h>
#include <string.h>


typedef struct PageDesc {
	int iPage;
	unsigned char *pucLcdFirstPosAtFile;          
	unsigned char *pucLcdNextPageFirstPosAtFile; 
	struct PageDesc *ptPrePage;
	struct PageDesc *ptNextPage;
} T_PageDesc, *PT_PageDesc;

static int g_iFdTextFile;
static unsigned char *g_pucTextFileMem;
static unsigned char *g_pucTextFileMemEnd;
static PT_EncodingOpr g_ptEncodingOprForFile;

static PT_DispOpr g_ptDispOpr;

static unsigned char *g_pucLcdFirstPosAtFile;
static unsigned char *g_pucLcdNextPosAtFile;

static int g_dwFontSize;

static PT_PageDesc g_ptPages   = NULL;
static PT_PageDesc g_ptCurPage = NULL;


int OpenTextFile(char *pcFileName)
{
	struct stat tStat;

	g_iFdTextFile = open(pcFileName, O_RDONLY);
	if (0 > g_iFdTextFile){
		DBG_PRINTF("can't open text file %s\n", pcFileName);
		return -1;
	}

	if(fstat(g_iFdTextFile, &tStat))      {
		DBG_PRINTF("can't get fstat\n");
		return -1;
	}
	g_pucTextFileMem = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ, MAP_SHARED, g_iFdTextFile, 0);
	if (g_pucTextFileMem == (unsigned char *)-1){
		DBG_PRINTF("can't mmap for text file\n");
		return -1;
	}

	g_pucTextFileMemEnd = g_pucTextFileMem + tStat.st_size;
	
	g_ptEncodingOprForFile = SelectEncodingOprForFile(g_pucTextFileMem); 

	if (g_ptEncodingOprForFile){
		g_pucLcdFirstPosAtFile = g_pucTextFileMem + g_ptEncodingOprForFile->iHeadLen; 
		return 0;                                                                     
	}else{
		return -1;
	}
}

¡/*Æ¥Åä×ÖÌå¿âÑ¡Ôñ×ÖÌå*/
int SetFontDetail(char *pcHZKFile, char *pcFileFreetype, unsigned int dwFontSize)    
{
	int iError = 0;
	PT_FontOpr ptFontOpr;
	PT_FontOpr ptTmp;
	int iRet = -1;

	g_dwFontSize = dwFontSize;   
	

	ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;  
	while (ptFontOpr){
		if (strcmp(ptFontOpr->name, "ascii") == 0)
			iError = ptFontOpr->FontInit(NULL, dwFontSize);
		else if (strcmp(ptFontOpr->name, "gbk") == 0)
			iError = ptFontOpr->FontInit(pcHZKFile, dwFontSize);  
		else
			iError = ptFontOpr->FontInit(pcFileFreetype, dwFontSize);

		DBG_PRINTF("%s, %d\n", ptFontOpr->name, iError);

		ptTmp = ptFontOpr->ptNext;

		if (iError == 0){
			/* ±ÈÈç¶ÔÓÚascii±àÂëµÄÎÄ¼þ, ¿ÉÄÜÓÃascii×ÖÌåÒ²¿ÉÄÜÓÃgbk×ÖÌå, 
			 * ËùÒÔÖ»ÒªÓÐÒ»¸öFontInit³É¹¦, SetFontDetail×îÖÕ¾Í·µ»Ø³É¹¦
			 */
			iRet = 0;
                }else{
			DelFontOprFrmEncoding(g_ptEncodingOprForFile, ptFontOpr);
                }
		ptFontOpr = ptTmp;
	}
	return iRet;
}

/*¸ù¾ÝÃû×Ö±éÀúÁ´±í*/
int SelectAndInitDisplay(char *pcName)    
{
	int iError;
	g_ptDispOpr = GetDispOpr(pcName);
	if (!g_ptDispOpr)
		return -1;

	iError = g_ptDispOpr->DeviceInit();
	return iError;
}

/*½«Êä³öÉè±¸µÄ·Ö±æÂÊ´«¸ø´¥ÃþÆÁ*/
int GetDispResolution(int *piXres, int *piYres)  
{
	if (g_ptDispOpr){
		*piXres = g_ptDispOpr->iXres;
		*piYres = g_ptDispOpr->iYres;
		return 0;
	}else{
		return -1;
	}
}

void ExitDevice(void)
{
	if(g_ptDispOpr->DeviceExit())
		g_ptDispOpr->DeviceExit();
}

int IncLcdX(int iX)
{
	if (iX + 1 < g_ptDispOpr->iXres)
		return (iX + 1);
	else
		return 0;
}

int IncLcdY(int iY)
{
	if (iY + g_dwFontSize < g_ptDispOpr->iYres)
		return (iY + g_dwFontSize);
	else
		return 0;
}

int RelocateFontPos(PT_FontBitMap ptFontBitMap)
{
	int iLcdY;
	int iDeltaX;
	int iDeltaY;

	if (ptFontBitMap->iYMax > g_ptDispOpr->iYres){
		/* ÂúÒ³ÁË */
		return -1;
	}

	/* ³¬¹ýLCD×îÓÒ±ß */
	if (ptFontBitMap->iXMax > g_ptDispOpr->iXres){
		/* »»ÐÐ */		
		iLcdY = IncLcdY(ptFontBitMap->iCurOriginY);
		if (0 == iLcdY){
			/* ÂúÒ³ÁË */
			return -1;
		}else{
			/* Ã»ÂúÒ³ */
			iDeltaX = 0 - ptFontBitMap->iCurOriginX;
			iDeltaY = iLcdY - ptFontBitMap->iCurOriginY;

			ptFontBitMap->iCurOriginX  += iDeltaX;
			ptFontBitMap->iCurOriginY  += iDeltaY;

			ptFontBitMap->iNextOriginX += iDeltaX;
			ptFontBitMap->iNextOriginY += iDeltaY;

			ptFontBitMap->iXLeft += iDeltaX;
			ptFontBitMap->iXMax  += iDeltaX;

			ptFontBitMap->iYTop  += iDeltaY;
			ptFontBitMap->iYMax  += iDeltaY;;
			
			return 0;	
		}
	}	
	return 0;
}

int ShowOneFont(PT_FontBitMap ptFontBitMap)
{
	int x;
	int y;
	unsigned char ucByte = 0;
	int i = 0;
	int bit;
	
	if (ptFontBitMap->iBpp == 1){
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++){
			i = (y - ptFontBitMap->iYTop) * ptFontBitMap->iPitch;
			for (x = ptFontBitMap->iXLeft, bit = 7; x < ptFontBitMap->iXMax; x++){
				if (bit == 7)
					ucByte = ptFontBitMap->pucBuffer[i++];
				
				if (ucByte & (1<<bit))
					g_ptDispOpr->ShowPixel(x, y, COLOR_FOREGROUND);
				else
					g_ptDispOpr->ShowPixel(x, y, 0); /* ºÚ */
				bit--;
				if (bit == -1)
					bit = 7;	
			}
		}
	}else if (ptFontBitMap->iBpp == 8){
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++)
			for (x = ptFontBitMap->iXLeft; x < ptFontBitMap->iXMax; x++){
				if (ptFontBitMap->pucBuffer[i++])
					g_ptDispOpr->ShowPixel(x, y, COLOR_FOREGROUND);
			}
	}else{
		DBG_PRINTF("ShowOneFont error, can't support %d bpp\n", ptFontBitMap->iBpp);
		return -1;
	}
	return 0;
}

int ShowOnePage(unsigned char *pucTextFileMemCurPos)
{
	int iLen;
	int iError;
	unsigned char *pucBufStart;
	unsigned int dwCode;
	PT_FontOpr ptFontOpr;
	T_FontBitMap tFontBitMap;
	
	int bHasNotClrSceen = 1;
	int bHasGetCode = 0;

	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = g_dwFontSize;
	pucBufStart = pucTextFileMemCurPos;

	
	while (1){
		iLen = g_ptEncodingOprForFile->GetCodeFrmBuf(pucBufStart, g_pucTextFileMemEnd, &dwCode);
		if (0 == iLen){
			if (!bHasGetCode)
				return -1;
			else
				return 0; 
		}

		bHasGetCode = 1;
		
		pucBufStart += iLen;     

		/* ÓÐÐ©ÎÄ±¾, \n\rÁ½¸öÒ»Æð²Å±íÊ¾»Ø³µ»»ÐÐ
		 * Åöµ½ÕâÖÖÁ¬ÐøµÄ\n\r, Ö»´¦ÀíÒ»´Î
		 */
		if (dwCode == '\n'){
			g_pucLcdNextPosAtFile = pucBufStart;
			
			/* »Ø³µ»»ÐÐ */
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
			if (0 == tFontBitMap.iCurOriginY){
				/* ÏÔÊ¾Íêµ±Ç°Ò»ÆÁÁË */
				return 0;
			}else{
				continue;
			}
		}else if (dwCode == '\r'){
			continue;
		}else if (dwCode == '\t'){
			/* TAB¼üÓÃÒ»¸ö¿Õ¸ñ´úÌæ */
			dwCode = ' ';
		}

		DBG_PRINTF("dwCode = 0x%x\n", dwCode);
		
		ptFontOpr = g_ptEncodingOprForFile->ptFontOprSupportedHead;
		while (ptFontOpr){
			DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
			iError = ptFontOpr->GetFontBitmap(dwCode, &tFontBitMap); 
			DBG_PRINTF("%s %s %d, ptFontOpr->name = %s, %d\n", __FILE__, __FUNCTION__, __LINE__, ptFontOpr->name, iError);
			if (0 == iError){
				DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
				if (RelocateFontPos(&tFontBitMap)){
					/* Ê£ÏÂµÄLCD¿Õ¼ä²»ÄÜÂú×ãÏÔÊ¾Õâ¸ö×Ö·û */
					return 0;
				}
				DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

				if (bHasNotClrSceen){
					/* Ê×ÏÈÇåÆÁ */
					g_ptDispOpr->CleanScreen(COLOR_BACKGROUND);
					bHasNotClrSceen = 0;
				}
				DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
				/* ÏÔÊ¾Ò»¸ö×Ö·û */
				if (ShowOneFont(&tFontBitMap))
					return -1;
				
				tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
				tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
				g_pucLcdNextPosAtFile = pucBufStart;

				/* ¼ÌÐøÈ¡³öÏÂÒ»¸ö±àÂëÀ´ÏÔÊ¾ */
				break;
			}
			ptFontOpr = ptFontOpr->ptNext;
		}		
	}
	return 0;
}

static void RecordPage(PT_PageDesc ptPageNew)
{
	PT_PageDesc ptPage;
		
	if (!g_ptPages){
		g_ptPages = ptPageNew;
	}else{
		ptPage = g_ptPages;
		while (ptPage->ptNextPage){
			ptPage = ptPage->ptNextPage;
		}
		ptPage->ptNextPage   = ptPageNew;
		ptPageNew->ptPrePage = ptPage;
	}
}

int ShowNextPage(void)
{
	int iError;
	PT_PageDesc ptPage;
	unsigned char *pucTextFileMemCurPos;

	if (g_ptCurPage)
		pucTextFileMemCurPos = g_ptCurPage->pucLcdNextPageFirstPosAtFile;
	else
		pucTextFileMemCurPos = g_pucLcdFirstPosAtFile;
		
	iError = ShowOnePage(pucTextFileMemCurPos);
	DBG_PRINTF("%s %d, %d\n", __FUNCTION__, __LINE__, iError);
	if (iError == 0){
		if (g_ptCurPage && g_ptCurPage->ptNextPage){
			g_ptCurPage = g_ptCurPage->ptNextPage;
			return 0;
		}
		
		ptPage = malloc(sizeof(T_PageDesc));
		if (ptPage){
			ptPage->pucLcdFirstPosAtFile         = pucTextFileMemCurPos;
			ptPage->pucLcdNextPageFirstPosAtFile = g_pucLcdNextPosAtFile;
			ptPage->ptPrePage                    = NULL;
			ptPage->ptNextPage                   = NULL;
			g_ptCurPage = ptPage;
			DBG_PRINTF("%s %d, pos = 0x%x\n", __FUNCTION__, __LINE__, (unsigned int)ptPage->pucLcdFirstPosAtFile);
			RecordPage(ptPage); 
			return 0;
		}else{
			return -1;
		}
	}
	return iError;
}

int ShowPrePage(void)
{
	int iError;

	DBG_PRINTF("%s %d\n", __FUNCTION__, __LINE__);
	if (!g_ptCurPage || !g_ptCurPage->ptPrePage)
		return -1;

	DBG_PRINTF("%s %d, pos = 0x%x\n", __FUNCTION__, __LINE__, (unsigned int)g_ptCurPage->ptPrePage->pucLcdFirstPosAtFile);
	iError = ShowOnePage(g_ptCurPage->ptPrePage->pucLcdFirstPosAtFile);
	if (iError == 0){
		DBG_PRINTF("%s %d\n", __FUNCTION__, __LINE__);
		g_ptCurPage = g_ptCurPage->ptPrePage;
	}
	return iError;
}
