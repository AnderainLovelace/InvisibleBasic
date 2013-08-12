#include <setjmp.h>
#include <fxlib.h>
#include "intpr.h"
#include "dconsole.h"

extern jmp_buf exit_buf;
extern void d_exit();

extern const char * pline;
extern char token[];
extern int token_type;

#define TIMER_ID 2

void timer_exit_break ()
{
	static times = 0;short unused = 0;
	int kcode1 = 0, kcode2 = 0;

	if (Bkey_GetKeyWait(&kcode1, &kcode2, KEYWAIT_HALTOFF_TIMEROFF,0,0, &unused)==KEYREP_KEYEVENT)
	{
		if ((kcode1==4)&&(kcode2==8))
			++times;
		if (times>=2)
		{
			// draw a win
			PopUpWin(3);
			locate(8,3);Print((unsigned char*)"Break!");
			locate(6,5);Print((unsigned char*)"Press [EXE]");
			while(WaitKey()!=KEY_CTRL_EXE);
			//		
			merror_msg("error:EXIT Break");
		}
	}
}

int AddIn_main(int isAppli, unsigned short OptionNum)
{
	int r;
	SOURCE_FILE * sf;
	Bdisp_AllClr_DDVRAM();

	if(setjmp(exit_buf)!=0)
	{
		KillTimer(TIMER_ID);
		WaitKey();
		return 1;
	}

	SetTimer(TIMER_ID,1000,timer_exit_break);

	sf = l_open_file("\\\\crd0\\3.txt");
	if (sf==0) return 0;
	/*while(1)
	{
		r=l_get_line(sf);
		puts(line);WaitKey();if (!r)return 0;
	}*/
	i_execute(sf);
	WaitKey();
	return 0;
}


#pragma section _BR_Size
unsigned long BR_Size;
#pragma section

#pragma section _TOP
int InitializeSystem(int isAppli, unsigned short OptionNum)
{
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}
#pragma section

