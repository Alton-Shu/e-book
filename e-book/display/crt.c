#include<config.h>
#include<disp_manager.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<linux/fb.h>
#include<sys/ioctl.h>
#include<sys/mman.h>
#include<string.h>
#include <vga.h>
#include <vgagl.h>

static int CRTDeviceInit(void);
static int CRTDeviceExit(void);
static int CRTShowPixel(int iPenx, int iPeny, unsigned int dwColor);
static int CRTCleanScreen(unsigned int dwBackColor);

static T_DispOpr  g_tCRTDispopr = {
	.name        = "crt",
	.DeviceInit  = CRTDeviceInit,     
	.DeviceExit  = CRTDeviceExit,       
	.ShowPixel   = CRTShowPixel,     
	.CleanScreen = CRTCleanScreen,  
};

static int CRTDeviceInit(void)
{
	vga_init();                          
	vga_setmode(G320x200x256);   
	g_tCRTDispopr.iXres=320;
	g_tCRTDispopr.iYres=200;
	g_tCRTDispopr.iBpp=8;	
	return 0;
}

static int CRTDeviceExit(void)
{
	vga_setmode(TEXT);
	return 0;
}

static int CRTShowPixel(int iPenx, int iPeny, unsigned int dwColor)
{
	int iRed,iGreen,iBlue;
	iRed = (dwColor>>16)&0xff;        
	iGreen = (dwColor>>8)&0xff;
	iBlue = (dwColor>>0)&0xff;

	gl_setpalettecolor(5, iRed>>2, iGreen>>2, iBlue>>2); 
	vga_setcolor(5);                            

	vga_drawpixel(iPenx, iPeny);      
	return 0;
}

static int CRTCleanScreen(unsigned int dwBackColor)
{
	int iRed,iGreen,iBlue;
	int iX,iY;
	iRed = (dwBackColor>>16)&0xff; 
	iGreen = (dwBackColor>>8)&0xff;
	iBlue = (dwBackColor>>0)&0xff;

	gl_setpalettecolor(4, iRed>>2, iGreen>>2, iBlue>>2);   /* 0xE7DBB5*/ /* ·º»ÆµÄÖ½ */
	vga_setcolor(4);                          

	for(iX = 0;iX < 320;iX++)
		for(iY = 0;iY < 200;iY++)
			vga_drawpixel(iX, iY);     
	return 0;
}

int CRTInit(void)
{
	return RegisterDispOpr(&g_tCRTDispopr);
}


