#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "filetree.h"

static Node 	*build_node(struct dirent *file, Node *parent_node);
static Node	*compare_with_keyword(Node *target_node, const char *search_keyword);
static int 	join_path(char *dest, char *dirname, char *filename); 
static int 	not_hidden(const struct dirent *dir_entry);
static void 	init_charray(char *arr, int len);
static int 	is_dir(char *path);
static int	recursive_tree_search(Node *results_node, Node *root_node, char *search_keyword);



Node *
build_root_node(char *root_name, char *root_path)
{
	if ((is_dir(root_path)) == -1) 
		return NULL;
  
	Node *root_node;
	root_node = (Node *) malloc(1 * sizeof(Node));

	if (root_node == NULL)
		return NULL;

	init_charray(root_node->name, NAME_MAX);
	init_charray(root_node->path, PATH_MAX);  
	strncpy(root_node->name, root_name, NAME_MAX-1);
	strncpy(root_node->path, root_path, PATH_MAX-1);

	root_node->parent = NULL;
	root_node->childs = NULL;
	root_node->is_dir = true;
	root_node->is_last_watched = false;

	root_node->ncurs_pos = 0;
	root_node->index_sel = 0;
	root_node->ntop_slice = 0;
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

	root_node->childs = (Node **) calloc (n, sizeof(Node *));
	if (root_node->childs == NULL)
		return -1;

	/* Loop sobre cada archivo del directorio */
	for (i=0; i<n; i++) {
		if ((root_node->childs[i] = build_node(namelist[i], root_node)) == NULL)
			return -1;

		if (namelist[i]->d_type == DT_DIR) {
			root_node->childs[i]->is_dir = true;
			build_tree(root_node->childs[i]);
		}
		else {
			root_node->childs[i]->is_dir = false;
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
	for (int i=0; i < node->nchilds; i++) {
		if (node->childs[i]->is_dir)
			free_tree(node->childs[i]);
		free(node->childs[i]);
	}
	free(node->childs);
	return;
}

Node *
search_node(char *keyword, Node *root_node)
{
	Node *finding = NULL;
	int i;
	for (i=0; i < root_node->nchilds && finding == NULL; i++) {
		if (strcmp(keyword, root_node->childs[i]->name) == 0)
			finding = root_node->childs[i];
		else if (root_node->childs[i]->is_dir == true)
			finding = search_node(keyword, root_node->childs[i]);
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

static Node *
build_node(struct dirent *file, Node *parent_node)
{
	if (parent_node == NULL || file == NULL)
		return NULL;

	Node *new_node;
	new_node = (Node *) malloc (sizeof(Node));

	init_charray(new_node->name, NAME_MAX);  
	init_charray(new_node->path, PATH_MAX);  

	strncpy(new_node->name, file->d_name, NAME_MAX-1);
	new_node->name[NAME_MAX] = '\0';

	join_path(new_node->path, parent_node->path, file->d_name);

	new_node->parent = parent_node;
	new_node->childs = NULL;
	new_node->is_dir = false;
	new_node->is_last_watched = false;
	new_node->ncurs_pos = 0;
	new_node->index_sel = 0;
	new_node->ntop_slice = 0;
	new_node->nchilds = 0;

	return new_node;
}

static int 
build_results_node(Node *results_node, Node *root_node, char *search_keyword)
{
	init_charray(results_node->name, NAME_MAX);  
	init_charray(results_node->path, PATH_MAX);  

	snprintf(results_node->name, NAME_MAX-1, "Resultados para '%s'", search_keyword);
	results_node->name[NAME_MAX] = '\0';

	strncpy(results_node->path, "No-path", PATH_MAX-1);

	results_node->parent = root_node;
	results_node->childs = NULL;
	results_node->is_dir = true;
	results_node->is_last_watched = false;
	results_node->ncurs_pos = 0;
	results_node->index_sel = 0;
	results_node->ntop_slice = 0;
	results_node->nchilds = 0;

	return 0;
}



/* Busca si SEARCH_KEYWORD esta en el nombre del TARGET_NODE
  Si esta, devuelve un puntero a TARGET_NODE
  Si no esta, devuelve NULL */
static Node *
compare_with_keyword(Node *target_node, const char *search_keyword)
{
	Node *p;
	int i;
	char *lower_node_name;

	p = NULL;
	lower_node_name = strdup(target_node->name);
	for (i=0; lower_node_name[i] != '\0'; i++) {
		lower_node_name[i] = tolower(lower_node_name[i]);
	}
	
	if ((strstr(lower_node_name, search_keyword)) != NULL) {
		p = target_node;
	}

	free(lower_node_name);
	return p;
}


static int 
add_node_to_results(Node *results_node, Node *node)
{
	int n;
	
	results_node->nchilds += 1;
	n = results_node->nchilds;

	results_node->childs = (Node **) realloc (results_node->childs, n * sizeof(Node*));

	results_node->childs[n - 1] = node;

	return 0;
}


static int
recursive_tree_search(Node *results_node, Node *root_node, char *search_keyword)
{
	Node *p;
	int i;
	
	for (i=0; i < root_node->nchilds ; i++)  {
		if ((p = compare_with_keyword(root_node->childs[i], search_keyword)) != NULL)

			add_node_to_results(results_node, p);

		if (root_node->childs[i]->is_dir) {
			recursive_tree_search(results_node, root_node->childs[i], search_keyword);
		}
	}
	return 0;
}


Node *
find_in_tree(char *search_keyword, Node *root_node)
{
	Node *results_node;

	results_node = (Node *) malloc (sizeof(Node));
	build_results_node(results_node, root_node, search_keyword);
	recursive_tree_search(results_node, root_node, search_keyword);

	return results_node;
}
