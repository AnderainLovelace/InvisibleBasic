#ifndef LIST_H
#define LIST_H

typedef struct tag_node
{
	void			*	p		 ;
	struct tag_node *	next,*pre;
}
node;

typedef struct
{
	node	 	*head,*last	;
	int					size;
}
list;

void list_init		(list * l);
void list_destory	(list * l);
void list_push		(list * l,void * p);

#endif