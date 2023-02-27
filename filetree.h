#ifndef FILETREE_H
#define FILETREE_H

#include <stdbool.h>

#define NAME_MAX 255		/* Caracteres en un filename. Igual a linux/limits.h */
#define PATH_MAX 4096           /* Caracteres en un path. Igual a linux/limits.h */

typedef struct tnode Node;
struct tnode {
  char name[NAME_MAX];
  char path[PATH_MAX];
  Node *parent;
  Node *childs;
  bool is_dir;		
  bool is_last_watched;	        /* Si es el ultimo episodio visto  */
  unsigned int ncurs_pos;	/* Posicion del cursor al abrir ese nodo en curses */
  unsigned int index_sel;	/* Index del array de childs */
  unsigned int ntop_slice;	/* Top slice: 1er elemento de los childs que se mostrara. Al scrollear, se modifica */
  unsigned int nchilds;
};

Node 	*build_root_node(char *root_name, char *root_path);
int 	build_tree(Node *root_node);
void 	free_tree(Node *node);
Node 	*search_node(char *keyword, Node *root_node);

#endif
