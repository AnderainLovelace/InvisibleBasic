//#include <stdio.h>
extern int sprintf(char*,const char*,...);

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <fxlib.h>
#include "intpr.h"
#include "list.h"
#include "file.h"
#include "utils.h"
#include "dconsole.h"

extern void d_exit ();

#define TRUE	1
#define FALSE	0

typedef struct
{
	char *	name;
	real	value;
}VAR;

typedef struct
{
	char *	name;
	real *	array;
	long	size;
}
ARRAY;

//--------------------------------------------------------------------------------------
// lexer part

typedef enum
{
	RES_SIN = 100,
	RES_COS		,
	RES_TAN		,
	RES_LOG		,
	RES_EXP		,
	RES_ABS		,
	RES_CEIL	,
	RES_FLOOR	,
	RES_FMOD	,
}
RESTOKEN;

typedef enum
{
	TT_LINE_END = 0,
	TT_COM,	
	TT_ID,
	TT_INT,
	TT_FLOAT,
	TT_STRING,
	//TT_SEM,
	TT_LBK,
	TT_RBK,
}
TOKEN_TYPE;

typedef enum
{
	// binary operator
	OPR_MEM = 200,	// o.p
	OPR_ADD,		// a+b
	OPR_SUB,		// a-b
	OPR_MUL,		// a*b
	OPR_DIV,		// a/b
	OPR_POW,		// a^b
	OPR_BE,			// a>=b
	OPR_BT,			// a>b
	OPR_LE,			// a<=b
	OPR_LT,			// a<b
	OPR_EQ,			// a=b
	OPR_NEQ,		// a<>b
	OPR_AND,		// a and b
	OPR_OR,			// a or b
	// unary operator
	OPR_NEG,		// -a
	OPR_NOT,		// not a


}OPR_TOKEN;

// error
void merror_msg(const char * format,...);
void merror_illegal_token();
void merror_expect(const char * s);
void merror_msg_only(const char * format,...);

extern const char * error_line;

#define str_eq(s1,s2) (strcmp(s1,s2)==0)

#define IS_OPR(t)			((t)>=OPR_MEM)
#define IS_UNARY_OPR(t)		((t)>=OPR_NEG)
#define IS_BINARY_OPR(t)	((t)< OPR_NEG)
#define IS_RESWORD(t)		((t)>=RES_SIN && (t)<OPR_MEM)
#define IS_CONSTANT(t)		((t)==TT_INT || (t)==TT_FLOAT)
#define IS_KEYWORD(t)		((t)>=KEY_SUB)

// 
const char *pline = NULL;
char token[LINE_MAX];
int  token_type;
int  token_ext ;

// reserved words (built-in functions!)
static struct
{
	int		token;
	char	*name;
	int		argc;
}
RESERVED_WORDS [] =
{
	{RES_SIN	,"sin"		,1},
	{RES_COS	,"cos"		,1},
	{RES_TAN	,"tan"		,1},
	{RES_LOG	,"ln"		,1},
	{RES_EXP	,"exp"		,1},
	{RES_ABS	,"abs"		,1},
	{RES_CEIL	,"ceil"		,1},
	{RES_FLOOR	,"floor"	,1},
	{RES_FMOD	,"mod"		,2},
	{0			,NULL		,0}
};
static struct
{
	int		token;
	char	*name;
}
RESERVED_WORDS_OPR [] =
{
	{OPR_AND	,"and"		},
	{OPR_OR		,"or"		},
	{OPR_NOT	,"not"		},
	{0			,NULL		}
};

// check reserved words
static int l_check_res ()
{
	int i;
	for(i=0;RESERVED_WORDS[i].name != NULL;++i)
	{
		if (str_eq(RESERVED_WORDS[i].name,token))
		{
			token_ext = RESERVED_WORDS[i].argc;
			return RESERVED_WORDS[i].token;
		}
	}
	return 0;
}
static int l_check_res_opr ()
{
	int i;
	for(i=0;RESERVED_WORDS_OPR[i].name != NULL;++i)
	{
		if (str_eq(RESERVED_WORDS_OPR[i].name,token))
			return RESERVED_WORDS_OPR[i].token;
	}
	return 0;
}

enum KEYWORD
{
	KEY_SUB = 300		,KEY_END			,KEY_IF				,KEY_ELSEIF			,
	KEY_ELSE			,KEY_WHILE			,KEY_FOR			,KEY_CASE			,
	KEY_WHEN			,KEY_GOSUB			,KEY_EXIT			,KEY_BREAK			,
	KEY_RETURN			,KEY_DIM			,KEY_REPEAT			,KEY_UNTIL			,
	KEY_VAR				
};
// key words
static struct
{
	int		token;
	char	*name;
}
KEY_WORDS [] =
{
	{KEY_SUB	,"sub"			},
	{KEY_END	,"end"			},
	{KEY_IF		,"if"			},
	{KEY_ELSEIF	,"elseif"		},
	{KEY_ELSE	,"else"			},
	{KEY_WHILE	,"while"		},
	{KEY_FOR	,"for"			},
	{KEY_CASE	,"case"			},
	{KEY_WHEN	,"when"			},
	{KEY_REPEAT	,"repeat"		},
	{KEY_UNTIL	,"until"		},
	{KEY_VAR	,"var"			},
	{KEY_GOSUB	,"gosub"		},
	{KEY_EXIT	,"exit"			},
	{KEY_BREAK	,"break"		},
	{KEY_RETURN	,"return"		},
	{KEY_DIM	,"dim"			},
};
// check key words
static int l_check_key ()
{
	int i;
	for(i=0;KEY_WORDS[i].name != NULL;++i)
	{
		if (str_eq(KEY_WORDS[i].name,token))
		{
			return KEY_WORDS[i].token;
		}
	}
	return 0;
}


void l_put_back ()
{
	pline -= strlen(token);
}
// get token
void l_get_token ()
{
	char c;
	char *t = token;

	while(isspace(*pline))++pline;
	c = *pline++;

	if (isalpha(c) || c=='_')
	{
		do
		{
			*t++ = c;
			c = *pline++;
		}while(isalpha(c) || isdigit(c) || c=='_');
		pline--;
		*t = '\0';
		// is a key word?
		token_type = l_check_key();
		if (token_type != 0) return;
		// is a reserved word?
		token_type = l_check_res();
		if (token_type != 0) return;
		// is a operator ?
		token_type = l_check_res_opr();
		if (token_type != 0) return;
		// is a identifier
		token_type = TT_ID;
		return;
	}
	else if (c=='=' || c=='.' || c=='*' || c=='/' || c=='^' || c=='(' || c==')' || c=='+' || c=='-' || c==',')
	{
		*t++ = c;*t = '\0';
		switch (c)
		{
			case '=':token_type = OPR_EQ ;break;
			case ',':token_type = TT_COM ;break;
			case '-':token_type = OPR_SUB;break;
			case '+':token_type = OPR_ADD;break;
			case '.':token_type = OPR_MEM;break;
			case '*':token_type = OPR_MUL;break;
			case '/':token_type = OPR_DIV;break;
			case '^':token_type = OPR_POW;break;
			case '(':token_type = TT_LBK ;break;
			case ')':token_type = TT_RBK ;break;
		}
		return;
	}
	else if (c=='>')
	{
		*t++ = c;
		if (*pline=='=')
		{
			*t++ = *pline;*t = '\0';
			pline++;
			token_type = OPR_BE;
		}
		else token_type = OPR_BT;
		return ;
	}
	else if (c=='<')
	{
		*t++ = c;
		if (*pline=='=')
		{
			*t++ = *pline;*t = '\0';
			pline++;
			token_type = OPR_LE;
		}
		else if (*pline=='>')
		{
			*t++ = *pline;*t = '\0';
			pline++;
			token_type = OPR_NEQ;
		}
		else token_type = OPR_LT;
		return ;
	}
	else if (isdigit(c))
	{
		do
		{
			*t++ = c;
			c = *pline++;
		}while(isdigit(c));
		// not a float
		if (c!='.')
		{
			pline--;
			*t = '\0';
			token_type = TT_INT;
			return;
		}
		// '.' if float
		if (isdigit(*pline))
		{
			do
			{
				*t++ = c;
				c = *pline++;
			}while(isdigit(c));
			pline--;
			*t = '\0';
			token_type = TT_FLOAT;
			return;
		}
		// '.' is a member operator
		else
		{
			pline--;
			*t = '\0';
			token_type = TT_INT;
			return;
		}
	}
	// get string
	else if (c=='\"')
	{
		while(*pline && *pline!='\"')
		{
			if (*pline=='\\')
			{
				pline++;
				if (*pline=='\\')
					*t++ = *pline++;
				else if (isdigit(*pline))
				{
					unsigned char c;
					char buf[32],*p=buf;
					while(isdigit(*pline))
					{
						*p++ = *pline++;
					}
					c = (unsigned char)atol(buf);
					*t++ = c;
				}
				else merror_msg ("illegal escape character");
			}
			else
				*t++ = *pline++;
		}
		*t = '\0';
		pline++;
		token_type = TT_STRING;
		return;
	}	
	else if (c=='\0')
	{
		*t = '\0';--pline;
		token_type = TT_LINE_END;
		return;
	}
	else
		merror_msg("illegal char:'%c'",c);

}

void match_str (const char * str)
{
	l_get_token ();
	if (!str_eq(str,token))
	{
		merror_expect(str);
	}
}

const char * OPR_TYPE_STR[]	= {".","+","-","*","/","^",">=",">","<=","<","=","and","or","-","not"};
const char * TYPE_STR[]		= {"EOF","comma","identifier","integer","float","string","'('","')'"};

void match_type (int type)
{
	l_get_token ();
	if (token_type != type)
	{
		if (IS_OPR(type))
		{
			merror_msg("illegal token:'%s','%s' expected",token,OPR_TYPE_STR[type-OPR_MEM]);
		}
		else if (type<100)
		{
			merror_msg("illegal token:'%s',%s expected",token,TYPE_STR[type-TT_LINE_END]);
		}
		else
		{
			merror_msg("illegal token:'%s',[%d] expected",token,type);
		}
	}
}

// assign 'state'
#define move(n) state=n;continue

// match optional expression
void match_exp (const char * exp)
{
	int state = 1;

	pline = exp;

	while(1)
	{
		//printf("state=%d\t",state);getch();
		switch(state)
		{
			//-----------------------------------------------------------------------
			// <CONSTANT>			=> 2
			// <ID>					=> 2
			// <UNARY OPR>			=> 1
			// (					=> 3
			// <FUNC>				=> 4
			case 1:
			{
				l_get_token();//printf("[%s]\n",token);
				if (token_type==OPR_SUB)
				{
					token_type = OPR_NEG;
					move(1);
				}
				else if (IS_OPR(token_type) && IS_UNARY_OPR(token_type))
				{
					move(1);
				}
				else if (IS_CONSTANT(token_type)) 
				{
					move(2);
				}
				else if (token_type==TT_ID)
				{
					move(5);
				}
				else if (token_type==TT_LBK)
				{
					move(3);
				}
				else if (IS_RESWORD(token_type))
				{
					move(4);
				}
				else
					merror_illegal_token();
			}break;
			//-----------------------------------------------------------------------
			// <OPR>				=> 1
			// <OTHER THINGS!>		=> END!
			case 2:
			{
				l_get_token();//printf("[%s]\n",token);
				if (IS_OPR(token_type))
				{
					if(IS_BINARY_OPR(token_type))
					{
						move(1);
					}
					else
					{
						merror_illegal_token();
					}
				}
				// END!
				l_put_back();
				return;
			}break;
			//-----------------------------------------------------------------------
			// 	<EXP> then )		=> 2
			case 3:
			{
				//puts("<match inner expr>");
				match_exp(pline);
				match_str(")");
				move(2);
			}break;
			//-----------------------------------------------------------------------
			// ( then <EXP>,<EXP>,<EXP>... then )	=> 2
			case 4:
			{
				int argc = token_ext,i;
				match_str("(");
				for(i=0;i<argc;++i)
				{
					match_exp(pline);
					if (i != argc-1)	match_str(",");
					else				match_str(")");
				}
				move(2);
			}break;
			//-----------------------------------------------------------------------
			// 
			case 5:
			{
				l_get_token();
				if (token_type==TT_LBK)	
				{
					move(3);
				}
				else
				{
					l_put_back();
					move(2);
				}
			}break;
		}
	}
}
//----------------------------------------------------------------------------------------
// calc part
typedef real (*FUNCTION)(real * );

real func_sin(real * param) {return sin(param[0]);}
real func_cos(real * param) {return cos(param[0]);}
real func_tan(real * param) {return tan(param[0]);}
real func_log(real * param) {return log(param[0]);}
real func_exp(real * param) {return exp(param[0]);}
real func_abs(real * param) {return fabs(param[0]);}
real func_ceil(real * param) {return ceil(param[0]);}
real func_floor(real * param) {return floor(param[0]);}
real func_fmod(real * param) {return fmod(param[0],param[1]);}

FUNCTION BUILT_IN_FUNC[]=
{
	func_sin		,
	func_cos		,
	func_tan		,
	func_log		,
	func_exp		,
	func_abs		,
	func_ceil		,
	func_floor		,
	func_fmod		,
};

#define get_func(i)			(BUILT_IN_FUNC[(i)-RES_SIN])
#define move2(n)			state=n
#define push_opr(o)			opr_stack[opr_size++] = (o)
#define push_opd(o)			opd_stack[opd_size++] = (o)
#define STACK_SIZE			64

const int PRIOROTY[] = 
{			 //----------------------------
	0xF		,// OPR_MEM,			// o.p
			 // ----------------------------
	0x5		,// OPR_ADD,			// a+b
	0x5		,// OPR_SUB,			// a-b
			 // ----------------------------
	0x7		,// OPR_MUL,			// a*b
	0x7		,// OPR_DIV,			// a/b
			 // ----------------------------
	0x8		,// OPR_POW,			// a^b
			 // ----------------------------
	0x4		,// OPR_BE,				// a>=b
	0x4		,// OPR_BT,				// a>b
	0x4		,// OPR_LE,				// a<=b
	0x4		,// OPR_LT,				// a<b
	0x4		,// OPR_EQ				// a=b
	0x4		,// OPR_NEQ				// a<>b
			 // ----------------------------
	0x2		,// OPR_AND,			// a and b
	0x2		,// OPR_OR,				// a or b
			 // ----------------------------
	0x6		,// OPR_NEG,			// -a
			 // ----------------------------
	0x3		,// OPR_NOT,			// not a
};

#define priority(opr)		(PRIOROTY[(opr)-OPR_MEM])	

real calc_pop (real * opd_stack,int * opr_stack,int * opd_size,int * opr_size)
{
	int		opr;
	
	opr = opr_stack[--(*opr_size)];
	//printf("%d %d %d\n",*opr_size,*opd_size,opr);

	if (IS_UNARY_OPR(opr))
	{
		real l;
		l  = opd_stack[--(*opd_size)];
		switch(opr)
		{
			case OPR_NEG:	return -l;
			case OPR_NOT:	return (real)(!(int)l);
			default:merror_msg("unknown operator [%d]!",opr);
		}
	}
	else
	{
		real l,r;
		r  = opd_stack[--(*opd_size)];
		l  = opd_stack[--(*opd_size)];
		switch(opr)
		{
			case OPR_ADD:	return l+r;
			case OPR_SUB:	return l-r;
			case OPR_MUL:	return l*r;
			case OPR_DIV:	return l/r;
			case OPR_POW:	return pow(l,r);
			case OPR_BE:	return (real)(l>=r);
			case OPR_BT:	return (real)(l>r);
			case OPR_LE:	return (real)(l<=r);
			case OPR_LT:	return (real)(l<r);
			case OPR_EQ:	return (real)(l==r);
			case OPR_NEQ:	return (real)(l!=r);
			case OPR_AND:	return (real)((int)l && (int)r);
			case OPR_OR:	return (real)((int)l || (int)r);
			default:merror_msg("unknown operator [%d]!",opr);
		}
	}

	return 0.0;
}


extern ARRAY * find_array (const char * array_name);
extern void assign_element (const ARRAY * a,int index,real r);
extern real get_element (const ARRAY * a,int index);

#define SHOW_STACK							\
{											\
	int i;									\
	puts("---show stack---");				\
	for (i=0;i<opd_size;++i)				\
	{										\
		printf("[%.4lf]",opd_stack[i]);		\
	}puts("");								\
	for (i=0;i<opr_size;++i)				\
	{										\
		printf("<%d>",opr_stack[i]);		\
	}puts("");								\
}WaitKey()

real calc_check(int check,const char * exp)
{
	real	opd_stack[STACK_SIZE];
	int		opr_stack[STACK_SIZE];
	int		opd_size = 0,opr_size = 0;
	int		state = 1/*,i*/;
	extern VAR * find_var (const char *);

	if (check)
	{
		match_exp(exp);
		l_get_token();
		if (token_type!=TT_LINE_END)
		{
			merror_msg("illegal expr");
		}
	}

	pline = exp;

	while(1)
	{
		l_get_token();//puts(token);SHOW_STACK;
		// exit
		if (token_type==TT_LINE_END)
		{
			break;
		}
		else if (token_type==TT_ID)
		{
			VAR * v;ARRAY * a;
			if (state!=1 && !check)
			{
				l_put_back();
				break;
			}
			v = find_var(token);
			if (v!=NULL)
			{
				push_opd(v->value);
				move2(2);
				continue;
			}
			a = find_array (token);
			if (a!=NULL)
			{
				int index;
				l_get_token();//skip (
				index = (int)calc_check(FALSE,pline);
				// delete this line,calc_check call skip ( automatically
				//l_get_token();//skip )
				push_opd(get_element(a,index));
				move2(2);
				continue;
			}
			merror_msg("Unrecognized identifier '%s'",token);
		}
		// constant
		else if (token_type==TT_INT || token_type==TT_FLOAT)
		{
			push_opd((real)atof(token));
			move2(2);
		}
		// operator
		else if (IS_OPR(token_type))
		{
			// neg "-" judgement
			if (token_type==OPR_SUB && state==1)
			{
				token_type = OPR_NEG;
			}
			//
			if (opr_size>0 &&
				opr_stack[opr_size-1]!=TT_LBK &&
				priority(token_type) <= priority(opr_stack[opr_size-1]))
			{
				real result;
				result = calc_pop(opd_stack,opr_stack,&opd_size,&opr_size);
				push_opd(result);
			}
			push_opr(token_type);
			move2(1);
		}
		// process '(' and ')'
		else if (token_type==TT_LBK)
		{
			push_opr(token_type);
		}
		else if (token_type==TT_RBK)
		{
			while(opr_stack[opr_size-1] /* top */ != TT_LBK && opr_size > 0)
			{
				real result;
				result = calc_pop(opd_stack,opr_stack,&opd_size,&opr_size);
				push_opd(result);
			}
			if (opr_stack[opr_size-1] /* top */ == TT_LBK)
			{
				opr_size--; // pop '('
			}
			else // funcition call end
			{
				break;
			}
		}
		// built-in function call
		else if (IS_RESWORD(token_type))
		{
			int			argc = token_ext,i;
			real		result;
			FUNCTION	func = get_func(token_type);
			//printf("call <%s>\n",token);
			// skip '('
			l_get_token ();
			// get all arg and push them in stack
			for (i=0;i<argc;++i)
			{
				result = calc_check(FALSE,pline);
				push_opd(result);
				//printf("[%d]  |%.4lf|\n",i,result);
			}
			// call func
			result = func(opd_stack + opd_size - argc);
			// pop args
			opd_size -= argc;
			// push result
			push_opd(result);
			//printf("call end\n");
		}
		else
		{
			if (check)
				merror_illegal_token();
			else
			{
				l_put_back();
				break;
			}
		}
	}
	// pop up all
	while(opr_size > 0)
	{
		real result;
		result = calc_pop(opd_stack,opr_stack,&opd_size,&opr_size);
		push_opd(result);
	}
	if (opd_size != 1)
	{
		merror_msg("calc:unknown error!");
	}
	return opd_stack[0];
}

real calc (const char * exp)
{
	return calc_check(TRUE,exp);
}

//-------------------------------------------------------------------------
// interpreter part
//
char line [LINE_MAX];

SOURCE_FILE * l_open_file (const char * file_name)
{
	SOURCE_FILE * sf;
	sf = (SOURCE_FILE*)calloc(sizeof(SOURCE_FILE),1);
	// alloc fail
	if (sf==NULL)
		return NULL;
	sf->file = open_file(file_name,_OPENMODE_READ_SHARE);
	// open fail
	if (sf->file<0)
		return NULL;
	// get size
	sf->size = Bfile_GetFileSize(sf->file);
	sf->pos = 0;
	return sf;
}

char* strchr1(char* s,char c)
{ 
  while(*s != '\0' && *s != c) 
  {
    ++s; 
  } 
  return *s == c ?s:NULL;
}

int l_get_line (SOURCE_FILE * sf)
{
	int bytes_read = 0,in_str,e = 0;
	char * p;
	bytes_read = Bfile_ReadFile(sf->file,line,LINE_MAX,sf->pos);
	line[bytes_read] = '\0';

	for (p=line;*p;++p)
	{
		if (*p=='\n')
		{
			*p = '\0';
			++p;
			if (*p=='\r')
				++p;
			break;
		}
		else if (*p=='\0')
			break;
	}

	sf->pos += p - line ;
	
	p = line;
	in_str = FALSE;
	while(*p)
	{
		if (*p=='\"')in_str = !in_str;
		else if(*p=='#' && !in_str) break;
		++p;
	}
	*p = '\0';
	//error_line = line;
	if (sf->pos>=sf->size)
		return FALSE;
	else
		return TRUE;
}
//--------------------------------------------------------------------------------------
// prescan part
typedef struct
{
	char *	name;
	long	pos;
}USER_SUB;

char * s_strdup(const char * s)
{
	int len = strlen(s)+1;
	char * d = (char*)calloc(len,1);
	strcpy(d,s);
	return d;
}

list list_global_var;
list list_array;
list list_sub;

VAR * find_var (const char * var_name)
{
	node		* n;
	VAR			* v;

	n = list_var.head;
	while (n)
	{
		v = n->p;
		if(str_eq(v->name,var_name))
			return v;
		n = n->next ;
	}
	return NULL;
}

VAR * create_var(const char * var_name)
{
	VAR * v;
	v			= (VAR*)calloc(sizeof(VAR),1);
	v->name		= s_strdup(var_name);
	v->value	= 0.0;
	list_push(&list_var,v);
	return v;
}

VAR * get_var (const char * var_name)
{
	VAR * v = find_var(var_name);
	if (v)
		return v;
	else
	{
		return create_var(var_name);
	}
}

ARRAY * find_array (const char * array_name)
{
	node		* n;
	ARRAY		* a;

	n = list_array.head;
	while (n)
	{
		a = n->p;
		if(str_eq(a->name,array_name))
			return a;
		n = n->next ;
	}
	return NULL;
}

void assign_element (const ARRAY * a,int index,real r)
{
	if (index<=0 || index>a->size)
		merror_msg("Access array '%s' over the border");
	a->array[index] = r;
}

real get_element (const ARRAY * a,int index)
{
	if (index<=0 || index>a->size)
		merror_msg("Access array '%s' over the border");
	return a->array[index-1];
}

USER_SUB * i_find_sub (const char * sub_name)
{
	node		* n;
	USER_SUB	* sub;

	n = list_sub.head;
	while (n)
	{
		sub = n->p;
		if(str_eq(sub->name,sub_name))
			return sub;
		n = n->next ;
	}
	return NULL;
}

#define push_block(b)			block_stack[block_size++] = (b)
#define pop_block				(block_stack[--block_size])
#define block_top				(block_stack[block_size-1])

enum
{
	B_IF,B_WHILE,B_FOR,B_CASE,B_SUB,B_REPEAT
}BLOCK;

const char * BLOCK_NAME[] = {"if","while","for","case","sub","repeat"};

void l_scan (SOURCE_FILE * sf)
{
	int r = 1,in_sub = FALSE;
	int block_stack[STACK_SIZE],block_size = 0;

	list_init(&list_sub);
	list_init(&list_var);
	list_init(&list_array);

	while(r)
	{
		r = l_get_line(sf);
		pline = line;
		l_get_token ();
		// skip empty line
		if (token_type==TT_LINE_END) continue;
		// check command
		if (IS_KEYWORD(token_type))
		{
			if (token_type==KEY_SUB)
			{
				USER_SUB * sub;
				
				if(in_sub)
				{
					merror_msg("sub procedure can not be nested");
				}
				in_sub = TRUE;

				match_type(TT_ID);
				sub = (USER_SUB*)calloc(sizeof(USER_SUB),1);
				sub->name	= s_strdup(token);
				sub->pos	= sf->pos;
				list_push(&list_sub,sub);
				match_type(TT_LINE_END);
				push_block(B_SUB);
				continue;
			}
			else if (token_type==KEY_END)
			{
				if (block_size<=0 || block_top==B_REPEAT)
					merror_illegal_token();
				match_type(TT_LINE_END);
				if(pop_block==B_SUB)
				{
					in_sub = FALSE;
				}
				continue;
			}
			else if(token_type==KEY_VAR)
			{
				while(1)
				{
					match_type(TT_ID);
					l_get_token();
					if (token_type==TT_END)			break;
					else if (token_type==TT_COM)	continue;
					else 							merror_illegal_token();
				}
			}
			if (!in_sub) merror_illegal_token();
			if(token_type==KEY_IF)
			{
				match_exp(pline);
				match_type(TT_LINE_END);
				push_block(B_IF);
			}
			else if (token_type==KEY_ELSEIF)
			{
				if (block_size<=0 || block_top!=B_IF)
					merror_illegal_token();
				match_exp(pline);
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_ELSE)
			{
				if (block_size<=0 || !(block_top==B_IF || block_top==B_CASE))
					merror_illegal_token();
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_WHILE)
			{
				match_exp(pline);
				match_type(TT_LINE_END);
				push_block(B_WHILE);
			}
			else if (token_type==KEY_FOR)
			{
				// 'for' ID '=' EXP 'to' EXP
				match_type(TT_ID);
				match_type(OPR_EQ);
				match_exp(pline);
				match_str("to");
				match_exp(pline);
				l_get_token();
				if (token_type!=TT_LINE_END)
				{
					if (!str_eq(token,"step"))
						merror_expect("step");
					match_exp(pline);
					match_type(TT_LINE_END);
				}
				push_block(B_FOR);
			}
			else if (token_type==KEY_CASE)
			{
				match_type(TT_ID);
				match_type(TT_LINE_END);
				push_block(B_CASE);
			}
			else if (token_type==KEY_REPEAT)
			{
				match_type(TT_LINE_END);
				push_block(B_REPEAT);
			}
			else if (token_type==KEY_UNTIL)
			{
				if (block_size<=0 || block_top!=B_REPEAT)
					merror_illegal_token();
				match_exp(pline);
				match_type(TT_LINE_END);
				pop_block;
			}
			else if (token_type==KEY_WHEN)
			{
				if (block_size<=0 || block_top!=B_CASE)
					merror_illegal_token();
				match_exp(pline);
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_GOSUB)
			{
				match_type(TT_ID);
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_EXIT)
			{
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_BREAK)
			{
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_RETURN)
			{
				match_type(TT_LINE_END);
			}
			else if (token_type==KEY_DIM)
			{
				while(1)
				{
					match_type	(TT_ID);
					match_type	(TT_LBK);
					match_exp	(pline);
					match_type	(TT_RBK);
					l_get_token();
					if		(token_type==TT_COM)		continue;
					else if	(token_type==TT_LINE_END)	break;
					else								merror_illegal_token();
				}
			}
		}
		else if (token_type==TT_ID)
		{
			if (!in_sub) merror_illegal_token();
			if (str_eq(token,"print"))
			{
				while(1)
				{
					l_get_token();
					if(token_type!=TT_STRING)
					{
						l_put_back();
						match_exp(pline);
					}
					l_get_token();
					if(token_type==TT_LINE_END) break;
					else if (token_type==TT_COM) continue;
					else merror_expect(",");
				}
			}
			else if (str_eq(token,"input"))
			{
				while(1)
				{
					match_type(TT_ID);
					l_get_token();
					if(token_type==TT_LINE_END) break;
					else if (token_type==TT_COM) continue;
					else merror_expect(",");
				}
			}
			else
			{	// match: [var][=][exp]
				l_get_token();
				if (token_type==OPR_EQ)
				{
					match_exp(pline);
					match_type(TT_LINE_END);
				}
				else
				{
					if (token_type!=TT_LBK)merror_expect("(");
					match_exp(pline);
					match_type(TT_RBK);
					match_type(OPR_EQ);
					match_exp(pline);
					match_type(TT_LINE_END);
				}
			}
		}
		else 
			merror_illegal_token ();
	}
	if (block_size>0)
		merror_msg("incompleted '%s' block!",BLOCK_NAME[block_top]);
}
//--------------------------------------------------------------------------------------
// interpreter part

#define EXE_END			0x0
#define EXE_DO			0x1
#define EXE_ELSEIF		0x3
#define EXE_ELSE		0x4
#define EXE_WHEN		0x5
#define EXE_BREAK		0x6
#define EXE_RETURN		0x7
#define EXE_UNTIL		0x8

extern int i_execute_line (SOURCE_FILE * sf,const int todo);

int i_execute_if (SOURCE_FILE * sf,const int todo)
{
	int this_todo,r,done = FALSE;

	if (!todo)
	{
		this_todo = FALSE;
	}
	else
	{
		this_todo = (calc_check(FALSE,pline) != 0.0);
		if (this_todo) done = TRUE;
	}

	while (TRUE)
	{
		r = i_execute_line(sf,this_todo);
		if (r==EXE_END) break;
		else if (r==EXE_ELSEIF)
		{
			if (todo)
			{
				if (done)	this_todo = FALSE;
				else
				{
					this_todo = (calc_check(FALSE,pline) != 0.0);
					if (this_todo) done = TRUE;
				}
			}
		}
		else if (r==EXE_ELSE)
		{
			if(todo)this_todo = !done;
		}
		else if (r==EXE_RETURN) return EXE_RETURN;
		else if (r==EXE_BREAK)  return EXE_BREAK;

	}
	return EXE_DO;
}

int i_execute_while (SOURCE_FILE * sf,const int todo)
{
	int this_todo,r,pos = sf->pos;
	char cond_buf[LINE_MAX];
	
	strcpy(cond_buf,pline);

	if (!todo)
	{
		this_todo = FALSE;
	}
	else
	{
		this_todo = (calc_check(FALSE,cond_buf) != 0.0);
	}

	while (TRUE)
	{
		r = i_execute_line(sf,this_todo);
		if (r==EXE_END)
		{
			if (this_todo)
			{
				this_todo = (calc_check(FALSE,cond_buf) != 0.0);
				sf->pos = pos;
			}
			else
				break;
		}
		else if (r==EXE_RETURN) return EXE_RETURN;
		else if (r==EXE_BREAK)
		{
			this_todo = FALSE;
		}
	}

	return EXE_DO;
}

int i_execute_repeat (SOURCE_FILE * sf,const int todo)
{
	int this_todo,r,pos = sf->pos;

	if (!todo)
	{
		this_todo = FALSE;
	}
	else
	{
		this_todo = TRUE;
	}

	while (TRUE)
	{
		r = i_execute_line(sf,this_todo);
		if (r==EXE_UNTIL)
		{
			if (todo)
			{
				this_todo = (calc_check(FALSE,pline) == 0.0);
				if (!this_todo)
					break;
				else
					sf->pos = pos;
			}
			else
				break;
		}
		else if (r==EXE_RETURN) return EXE_RETURN;
		else if (r==EXE_BREAK)
		{
			this_todo = FALSE;
		}
	}

	return EXE_DO;
}

int i_execute_for (SOURCE_FILE * sf,const int todo)
{
	int		this_todo,r,pos = sf->pos;
	real	begin,end,step;
	VAR		* v;

	l_get_token();v = get_var(token);
	l_get_token();// skip '='
	begin	= calc_check(FALSE,pline);
	l_get_token();// skip 'to'
	end		= calc_check(FALSE,pline);
	l_get_token();
	if (token_type!=TT_LINE_END) step= calc_check(FALSE,pline);
	else						 step = 1.0;
	
	v->value = begin;

	if (!todo)
	{
		this_todo = FALSE;
	}
	else
	{
		this_todo = v->value<=end;
	}

	while (TRUE)
	{
		r = i_execute_line(sf,this_todo);
		if (r==EXE_END)
		{
			if (this_todo)
			{
				v->value+=step;
				this_todo = v->value<=end;
				sf->pos = pos;
			}
			else
				break;
		}
		else if (r==EXE_RETURN) return EXE_RETURN;
		else if (r==EXE_BREAK)
		{
			this_todo = FALSE;
		}
	}

	return EXE_DO;
}

int i_execute_case (SOURCE_FILE * sf,const int todo)
{
	int this_todo,r,done = FALSE;
	VAR * v;

	l_get_token();
	v = find_var(token);

	if (!todo)
	{
		this_todo = FALSE;
	}

	while (TRUE)
	{
		r = i_execute_line(sf,this_todo);
		if (r==EXE_END) break;
		else if (r==EXE_WHEN)
		{
			if (todo)
			{
				if (done)	this_todo = FALSE;
				else
				{
					this_todo = (calc_check(FALSE,pline) == v->value);
					if (this_todo) done = TRUE;
				}
			}
		}
		else if (r==EXE_ELSE)
		{
			if(todo)this_todo = !done;
		}
		else if (r==EXE_RETURN) return EXE_RETURN;
		else if (r==EXE_BREAK)  return EXE_BREAK;

	}
	return EXE_DO;
}

int i_execute_line (SOURCE_FILE * sf,const int todo)
{
	l_get_line(sf);pline = line;
	l_get_token();
	if (token_type==TT_LINE_END)
	{
		return EXE_DO;// skip empty line
	}
	else if (IS_KEYWORD(token_type))
	{
		if (token_type==KEY_END)
		{
			return EXE_END;
		}
		else if (token_type==KEY_IF)
		{
			return i_execute_if (sf,todo);
		}
		else if (token_type==KEY_WHILE)
		{
			return i_execute_while(sf,todo);
		}
		else if (token_type==KEY_REPEAT)
		{
			return i_execute_repeat(sf,todo);
		}
		if (token_type==KEY_UNTIL)
		{
			return EXE_UNTIL;
		}
		else if (token_type==KEY_FOR)
		{
			return i_execute_for(sf,todo);
		}
		else if (token_type==KEY_CASE)
		{
			return i_execute_case(sf,todo);
		}
		else if (token_type==KEY_ELSEIF)
		{
			return EXE_ELSEIF;
		}
		else if (token_type==KEY_WHEN)
		{
			return EXE_WHEN;
		}
		else if (token_type==KEY_ELSE)
		{
			return EXE_ELSE;
		}
		if (!todo) return EXE_DO;
		if (token_type==KEY_BREAK)
		{
			return EXE_BREAK;
		}
		else if (token_type==KEY_RETURN)
		{
			return EXE_RETURN;
		}
		else if (token_type==KEY_GOSUB)
		{
			extern int i_call_sub (SOURCE_FILE *,const char *);
			l_get_token();
			return i_call_sub(sf,token);
		}
		else if (token_type==KEY_DIM)
		{
			ARRAY *	array;
			long	size;
			
			while(1)
			{
				l_get_token();
				// new array
				array = (ARRAY*)calloc(sizeof(ARRAY),1);
				array->name  = s_strdup(token);
				list_push(&list_array,array);
				//
				l_get_token();// skip '('
				size = (long)calc_check(FALSE,pline);
				if (size<=0)
				{
					merror_msg("illegal array size %d!",size);
				}
				array->size  = size;
				array->array = calloc(sizeof(real),size);
				{
					int i;
					for (i=0;i<size;++i) array->array[i] = 0.0;
				}
				l_get_token();// skip ')'
				l_get_token();
				if		(token_type==TT_COM)		continue;
				else if	(token_type==TT_LINE_END)	break;
			}
		}
		else if (token_type==KEY_EXIT)
		{
			d_exit();
		}
	}
	else if(token_type==TT_ID)
	{
		if (str_eq(token,"print"))
		{
			while(1)
			{
				l_get_token();
				if (token_type==TT_STRING)
				{
					printf("%s",token);
				}
				else
				{
					real result;
					char buf[128];
				
					l_put_back();
					result = calc_check(FALSE,pline);
					d_ftoa(result,buf);
					printf(buf);
				}
				l_get_token();
				if (token_type==TT_LINE_END)break;
			}//while
		}//print
		else if (str_eq(token,"input"))
		{
			VAR * v;char buf[64];
			while(1)
			{
				l_get_token();
				v = get_var(token);
				gets(64,buf);puts(buf);
				v->value = atof(buf);
				l_get_token();
				if (token_type==TT_LINE_END)break;
			}//while
		}//input
		else
		{
			ARRAY * a;
			VAR * var;
			var = find_var(token);
			if (var!=NULL)
			{
				l_get_token(); //skip '='
				var->value = calc_check(FALSE,pline);
				return EXE_DO;
			}
			a = find_array(token);
			if(a!=NULL)
			{
				int index;
				l_get_token();//skip (
				index = (int)calc_check(FALSE,pline);
				// delete this line,calc_check call skip ( automatically
				//l_get_token();//skip )
				l_get_token();// skip =
				assign_element(a,index,calc_check(FALSE,pline));
				return EXE_DO;
			}
			var = create_var(token);
			l_get_token(); //skip '='
			var->value = calc_check(FALSE,pline);
			return EXE_DO;
		}//assign
	}// TT_ID
	else
	{
		merror_illegal_token();
	}
	return EXE_DO;
}

int i_call_sub (SOURCE_FILE * sf,const char * sub_name)
{
	USER_SUB *	sub;
	long		pre_pos;
	int			todo = TRUE,r;

	sub = i_find_sub(sub_name);
	if (sub==NULL)
	{
		merror_msg_only("function '%s' not found",sub_name);
	}
	pre_pos = sf->pos;
	// locate 'main'
	sf->pos = sub->pos;

	while (TRUE)
	{
		r = i_execute_line(sf,todo);
		if (r==EXE_END || r==EXE_RETURN) break;
		else if (r==EXE_BREAK) merror_msg("No loop to jump out!");
	}

	sf->pos = pre_pos;

	return EXE_DO;
}

void i_execute(SOURCE_FILE * sf)
{
	// pre scan
	l_scan(sf);
	// call main
	i_call_sub(sf,"main");
	// clean up
	list_destory(&list_var);
	list_destory(&list_sub);
	Bfile_CloseFile(sf->file);
	free(sf);
}
