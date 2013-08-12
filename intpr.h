#ifndef INTPR_H
#define INTPR_H

typedef double real;

real calc(const char * exp);

typedef struct
{
	long		pos;
	long		size;
	int			file;
}
SOURCE_FILE;

#define LINE_MAX 128
extern char line[];

SOURCE_FILE *	l_open_file		(const char *);
void			l_scan			(SOURCE_FILE *);
void			i_execute		(SOURCE_FILE *);

#endif