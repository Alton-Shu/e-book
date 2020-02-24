#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <draw.h>
#include <encoding_manager.h>
#include <fonts_manager.h>
#include <disp_manager.h>
#include <input_manager.h>
#include <string.h>


/* ./show_file [-s Size] [-f freetype_font_file] [-h HZK] <text_file> */
int main(int argc, char **argv)
{
	int iError;
	unsigned int dwFontSize = 16;
	char acHzkFile[128];
	char acFreetypeFile[128];
	char acTextFile[128];

	char acDisplay[128];

	int bList = 0;

	T_InputEvent tInputEvent;

	acHzkFile[0]  = '\0';
	acFreetypeFile[0] = '\0';
	acTextFile[0] = '\0';

	strcpy(acDisplay, "fb");
	
	while ((iError = getopt(argc, argv, "ls:f:h:d:")) != -1)
	{
		switch(iError)
		{
			case 'l':
			{
				  bList = 1;
				  break;
			}
			case 's':
			{
				  dwFontSize = strtoul(optarg, NULL, 0);   //optarg代表输入的参数
				  break;
			}
			case 'f':
			{
				  strncpy(acFreetypeFile, optarg, 128);   //freetype(字体库)比如simsun.ttc
				  acFreetypeFile[127] = '\0';
				  break;
			}			
			case 'h':
			{
					strncpy(acHzkFile, optarg, 128);//汉字库基本不用
					acHzkFile[127] = '\0';
					break;
			}
			case 'd':
			{
				strncpy(acDisplay, optarg, 128);   //输出设备比如-d crt
				acDisplay[127] = '\0';
				break;
			}
			default:
			{
					printf("Usage: %s [-s Size] [-d display] [-f font_file] [-h HZK] <text_file>\n", argv[0]);
					printf("Usage: %s -l\n", argv[0]);
					return -1;
					break;
			}
		}
	}

	if (!bList && (optind >= argc))     //optind剩下字符串的下标
	{
		printf("Usage: %s [-s Size] [-d display] [-f font_file] [-h HZK] <text_file>\n", argv[0]);
		printf("Usage: %s -l\n", argv[0]);
		return -1;
	}
		
	iError = DisplayInit();                  //把输出设备加入g_ptDispOprHead链表
	if (iError)
	{
		printf("DisplayInit error!\n");
		return -1;
	}

	iError = FontsInit();         //把支持的字体文件放入链表
	if (iError)
	{
		printf("FontsInit error!\n");
		return -1;
	}

	iError = EncodingInit();  //把编码文件 放入链表
	if (iError)
	{
		printf("EncodingInit error!\n");
		return -1;
	}


	iError = InputInit();       //把输入方式放入链表
	if (iError)
	{
		printf("InputInit error!\n");
		return -1;
	}


	if (bList)                 //列出链表里的内容
	{
		printf("supported display:\n");
		ShowDispOpr();

		printf("supported font:\n");
		ShowFontOpr();

		printf("supported encoding:\n");
		ShowEncodingOpr();

		printf("supported input:\n");
		ShowInputOpr();
		
		return 0;
	}

	strncpy(acTextFile, argv[optind], 128);    //需显示的文本名
	acTextFile[127] = '\0';
		
	iError = OpenTextFile(acTextFile);       //打开文件并选择编码器(这个函数在draw.c里)
	if (iError)
	{
		printf("OpenTextFile error!\n");
		return -1;
	}

	iError = SetFontDetail(acHzkFile, acFreetypeFile, dwFontSize);  //根据选择的编码器通过.name匹配字体库
	if (iError)                                                    //由draw.c调用ptFontOpr->FontInit(在链表里找)
	{                                                              //再到每个具体的FontInit,比如FreeTypeFontInit
		printf("SetTextDetail error!\n");                                          
		return -1;
	}

	DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	iError = SelectAndInitDisplay(acDisplay);                 //根据名字选择设备接着调用相应设备的
	if (iError)                                                //g_ptDispOpr->DeviceInit()比如CRTDeviceInit
	{
		printf("SelectAndInitDisplay error!\n");
		return -1;
	}

	iError = AllInputDevicesInit();                           //如果上面DeviceInit()成功了就创立一个子线程
	if (iError)
	{
		DBG_PRINTF("Error AllInputDevicesInit\n");
		return -1;
	}
	
	DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	iError = ShowNextPage();//仔细研究一下   
	DBG_PRINTF("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	if (iError)
	{
		printf("Error to show first page\n");
		return -1;
	}

	printf("Enter 'n' to show next page, 'u' to show previous page, 'q' to exit: ");

	while (1)
	{

		if (0 == GetInputEvent(&tInputEvent))       //获得事件(按键、点击、滑动等)
		{
			if (tInputEvent.iVal == INPUT_VALUE_DOWN)
			{
				ShowNextPage();
			}
			else if (tInputEvent.iVal == INPUT_VALUE_UP)
			{
				ShowPrePage();			
			}
			else if (tInputEvent.iVal == INPUT_VALUE_EXIT)
			{
				return 0;
			}
		}
	}
	return 0;
}

