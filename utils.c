#include <stdio.h>  
#include <math.h>  
#include <string.h>  
#include "dconsole.h"

char *d_ftoa(double num,char *buf)  
{  
	int		i;
	char	*s = buf;
	i = sprintf(buf,"%.8lf",num);
	buf += i-1;
	while(*buf == '0')
		*buf-- = '\0';
	if (*buf=='.') *buf = '\0';
	return s;
}  