#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "filetree.h"

static int 	not_hidden(const struct dirent *dir_entry);
static void 	init_charray(char *arr, int len);
static int 	is_dir(char *path);
static int 	join_path(char *dest, char *dirname, char *filename); 
static int 	build_node(struct dirent *file, Node *parent_node, Node *node);

Node *
build_root_node(char *root_name, char *root_path)
{
	if ((is_dir(root_path)) == -1) 
		return NULL;
  
	Node *root_node;
	root_node = (Node *) calloc(1, sizeof(Node));
	/* root_node = (Node *) malloc(sizeof(Node)); */
	if (root_node == NULL)
		return NULL;

	init_charray(root_node->name, NAME_MAX);
	init_charray(root_node->path, PATH_MAX);  

	strncpy(root_node->name, root_name, NAME_MAX-1);
	strncpy(root_node->path, root_path, PATH_MAX-1);
	root_node->parent = NULL;
	root_node->is_dir = true;
	root_node->is_last_watched = false;
	root_node->childs = NULL;
	root_node->nchilds = 0;

	return root_node;
}

int
build_tree(Node *root_node)
{
	struct dirent **namelist;
	int n, i;

	n = scandir(root_node->path, &namelist, not_hidden, alphasort);
	if (n == -1) {
		return -1;
	}

	root_node->childs = (Node *) calloc (n, sizeof(Node));
	if (root_node->childs == NULL)
		return -1;

	/* Loop sobre cada archivo del directorio */
	for (i=0; i<n; i++) {
		if ((build_node(namelist[i], root_node, &root_node->childs[i])) == -1) {
			while (n--)
				free(namelist[n]);
			free(namelist);
			return -1;
		}

		if (namelist[i]->d_type == DT_DIR) {
			root_node->childs[i].is_dir = true;
			build_tree(&root_node->childs[i]);
		}
		else {
			root_node->childs[i].is_dir = false;
		}
	}
	root_node->nchilds = n;
	while (n--)
		free(namelist[n]);
	free(namelist);
	return 0;
}

void
free_tree(Node *node)
{
	for (int i=0; i<node->nchilds; i++) {
		if (node->childs[i].is_dir)
			free_tree(&node->childs[i]);
	}
	free(node->childs);
	return;
}

Node *
search_node(char *keyword, Node *root_node)
{
	Node *finding = NULL;
	int i;
	for (i=0; i<root_node->nchilds && finding == NULL; i++) {
		if (strcmp(keyword, root_node->childs[i].name) == 0)
			finding = &root_node->childs[i];
		else if (root_node->childs[i].is_dir == true)
			finding = search_node(keyword, &root_node->childs[i]);
	}
	return finding;
}

/* Chequea que no sea un archivo oculto. */
static int
not_hidden(const struct dirent *dir_entry)
{
	if (dir_entry->d_name[0] == '.')
		return 0;
	else
		return 1;
}

static void
init_charray(char *arr, int len)
{
	int i;
	for (i=0; i<len; i++)
		arr[i] = '\0';
}

static int
is_dir(char *path)
{
	DIR* dir = opendir(path);
	if (dir) {
		/* Directory exists. */
		closedir(dir);
		return 0;
	} else if (ENOENT == errno) {
		/* Directory does not exist. */
		fprintf(stderr, "Error: el directorio no existe.\n");
		return -1;
	} else {
		/* opendir() failed for some other reason. */
		fprintf(stderr, "Error: no se pudo abrir el directorio.\n");
		return -1;
	}
}

static int
join_path(char *dest, char *dirname, char *filename)
{
	int dlen, flen, barlen, pathlen;

	dlen = strlen(dirname);
	flen = strlen(filename);
	barlen = 1;
	pathlen = dlen + flen + barlen;

	if (pathlen >= PATH_MAX - 1)
		return -1;

	snprintf(dest, PATH_MAX, "%s/%s%c", dirname, filename, '\0');
	return pathlen;  
}

static int 
build_node(struct dirent *file, Node *parent_node, Node *node)
{
	if (parent_node == NULL || node == NULL || file == NULL)
		return -1;

	init_charray(node->name, NAME_MAX);  
	init_charray(node->path, PATH_MAX);  

	strncpy(node->name, file->d_name, NAME_MAX-1);
	node->name[NAME_MAX] = '\0';

	join_path(node->path, parent_node->path, file->d_name);

	node->parent = parent_node;
	node->index_sel = 0;
	node->ncurs_pos = 0;
	node->ntop_slice = 0;
	node->is_dir = false;
	node->childs = NULL;
	node->nchilds = 0;

	return 0;
}
