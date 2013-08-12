#include <stdlib.h>
#include "list.h"

void list_init (list * l)
{
	l->head	= NULL;
	l->last	= NULL;
	l->size	= 0;
}

void list_destory (list * l)
{
	node *node1,*node2;

	node1 = l->head;
	
	while (node1)
	{
		node2 = node1;
		node1 = node1->next ;
		
		free (node2->p);
		free (node2);
	}

	l->head = l->last = NULL;
	l->size = 0;
}

void list_push (list * l,void * p)
{
	if (l->size == 0)
	{
		l->head			= (node*)calloc(sizeof(node),1);
		l->last			= l->head;
	}
	else
	{
		node * n = l->last;
		l->last->next	= (node*)calloc(sizeof(node),1);
		l->last			= l->last->next;
		l->last->pre	= n;
	}

	l->last->p = p;

	l->size ++;

}

void list_pop (list * l)
{
	if (l->si)
}