#include <stdarg.h>
#include <setjmp.h>
#include "dConsole.h"
#include "intpr.h"

jmp_buf exit_buf;

void d_exit ()
{
 	longjmp(exit_buf,1);
}

void merror_show_line ()
{
	int i,s;
	extern const char * pline;
	extern void l_put_back();
	puts(line);
	l_put_back();
	s = pline - line;
	for(i=0;i<s;++i) put(" ");
	puts("^");
}


void merror_msg_only(const char * format,...)
{
	char		buf[512];
	va_list		arg_list;

	va_start	(arg_list,format);
	vsprintf	((char*)buf,(char*)format,arg_list);
	va_end		(arg_list);

	puts("Error:");
	puts(buf);

	d_exit();
}

void merror_msg(const char * format,...)
{
	char		buf[512];
	va_list		arg_list;

	va_start	(arg_list,format);
	vsprintf	((char*)buf,(char*)format,arg_list);
	va_end		(arg_list);

	merror_show_line();
	puts("Error:");
	puts(buf);

	d_exit();
}

void merror_illegal_token()
{
	extern char token[];

	merror_show_line();
	puts("Error:");
	printf("Illegal token '%s'\n",token);
	d_exit();
}

void merror_expect(const char * s)
{
	extern char token[];

	merror_show_line();
	puts("Error:");
	printf("illegal '%s','%s' expect here\n",token,s);
	d_exit();
}
