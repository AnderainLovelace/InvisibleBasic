#ifndef DCONSOLE_H
#define DCONSOLE_H

int		dGetLineBox 		(char * s,int max,int width,int x,int y);
void 	dConsoleRedraw 		();
void 	dConsolePut			(const char * str);
int 	dGetLine 			(int max,char * s);
void 	dConsolePuts 		(const char * str);
int		dPrintf				(const char * format,...);

unsigned int WaitKey();

#define gets		dGetLine
#define puts		dConsolePuts
#define put			dConsolePut
#define printf		dPrintf

#endif
