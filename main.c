#include <sys/wait.h>
#include <sys/stat.h>

#include <curses.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "filetree.h"
#include "hemeroteca.h"

enum prgnstatus {
  CONT = 0,
  EXIT = -1,
};

int scr_rows;			/* Filas de la pantalla */
int scr_cols;			/* Columnas de la pantalla */

WINDOW *title_win;
WINDOW *statusbar_win;
WINDOW *left_win;
WINDOW *right_win;

TextWin LeftWin;
TextWin RightWin;

Node *root_node;		
Node *lcurrnode;		/* Current node del LeftWin */
Node *lselnode;			/* Selected node (el que esta bajo el cursor) del LeftWin */

static void	 build_leftwin(void);
static void	 build_rightwin(void);
static void	 build_statusbar(void);
static void	 build_title(void);
static WINDOW 	 *build_window(struct win_frame frame);
static void	 close_node(void);
static void 	 die(const char *);
static void	 goto_bottom(void);
static void	 goto_top(void);
static void	 init_tui(void);
static void 	 mv_curs_down(void);
static void 	 mv_curs_up(void);
static void 	 open_node(void);
static int	 open_pdf(void);
static void	 print_dir(TextWin WIN, char *buffer, int line);
static void	 print_highlight(TextWin WIN, char *buffer, int line);
static void	 print_nodename(TextWin WIN, Node *node);
static void	 print_relative_number(TextWin WIN, int relative_number, int line);
static void	 print_statusbar(char *msg);
static void	 print_textwin(TextWin WIN, Node *node);
static void	 print_text_windows(void);
static void	 print_title(void);
static void	 rebuild_all(void);
static void	 refresh_wins(void);
static void	 relative_num_move(char first_num);
static void	 relnum_mv_down(int num);
static void	 relnum_mv_up(int num);
static int	 start_hemeroteca_tui(char *dir_to_use);
static char 	 *truncate_filename(char *oldname, int max_length);
static void 	 usage(void);


int
main(int argc, char *argv[])
{

  	int dflag, sflag;
	int opt;
	char *dir_to_use;

	dflag = sflag = 0;

	while ((opt = getopt(argc, argv, "s:d:h")) != -1) {
		switch (opt) {

			/* IMPLEMENTAR -s search! */
		case 's':
			sflag = 1;
			break;
		case 'd':
			dflag = 1;
			dir_to_use = strdup(optarg);
			/* strncpy(dir_to_use, optarg, PATH_MAX - 1); */
			break;
		case '?':
			/* FALLTHROUGH */
		case 'h':
			/* FALLTHROUGH */
		default:
			usage();
			return 1;
			/* flag = SHOW_HELP; */
		}
	}
	argc -= optind;
	argv += optind;
	

	if (argc > 0) {
		usage();
		exit(1);
	} else if (dflag == 0) {
		dir_to_use = strdup(HEMEROTECA_PATH);
		start_hemeroteca_tui(dir_to_use);
	} else if (dflag == 1)
		start_hemeroteca_tui(dir_to_use);

	return 0;
}



int
start_hemeroteca_tui(char *dir_to_use) {

	int tuistatus, key;

	if ((root_node = build_root_node(basename(dir_to_use), dir_to_use)) == NULL) 
		die("Error al crear root_node\n");

	if ((build_tree(root_node)) == -1)
		die("No se pudo crear el arbol de archivos.\n");;

	if (root_node->nchilds == 0) 
		die("El directorio se encuentra vacio.\n");

	init_tui();
	tuistatus = CONT;
	while (tuistatus == CONT) 
		{
			print_statusbar(lselnode->name);
			refresh_wins();

			key = getch();
			switch (key) {

			case 'j':
			case KEY_DOWN:
				mv_curs_down();
				break;
			case 'k':
			case KEY_UP:
				mv_curs_up();
				break;
			case 'l':
			case KEY_RIGHT:
				open_node();
				break;
			case 'h':
			case KEY_LEFT:
				close_node();
				break;

			case 'G':
				goto_bottom();
				break;
			case 'g':
				if ((key = getch()) == 'g')
					goto_top();
				break;

			case '0': case '1': case '2': case '3': case '4': 
			case '5': case '6': case '7': case '8': case '9':
				relative_num_move(key);
				break;

			case 'p':
			case KEY_ENTER:
			case 10:
				if ((open_pdf()) == -1)
					die("Error en fork: no se pudo abrir VLC\n");
				break;

			case KEY_RESIZE:
				rebuild_all();
				break;
	
			case 'q':
				tuistatus = EXIT;
				break;

			default:
				break;
	
			}
		}
	endwin();
	free_tree(root_node);
	free(root_node);
	free(dir_to_use);
	return 0;
}



static void
die(const char *errstr)
{
	if (root_node != NULL)
		free_tree(root_node);

	fprintf(stderr, errstr);
	exit(EXIT_FAILURE);
}



static WINDOW *
build_window(struct win_frame frame)
{
	return newwin(frame.rows, frame.cols, frame.y_pos, frame.x_pos);
}

static void
build_title(void)
{
	struct win_frame frame;
	frame.rows = title_rows;
	frame.cols = scr_cols-1;
	frame.y_pos = 0;
	frame.x_pos = 0;
  
	title_win = build_window(frame);
	wattrset(title_win, COLOR_PAIR(1));
	box(title_win, ACS_VLINE, ACS_HLINE);
	return;
}

static void
print_title(void)
{
	int x_coor;
	x_coor = (scr_cols/2 - strlen(PROGRAM_NAME) / 2);
	wattron(title_win, A_BOLD);
	mvwaddstr(title_win, 1, x_coor, PROGRAM_NAME);
	wattroff(title_win, A_BOLD);
	return;
}

static void
build_statusbar(void)
{
	struct win_frame frame;
	frame.rows = 1;
	frame.cols = scr_cols-1;
	frame.y_pos = scr_rows-1;
	frame.x_pos = 0;

	statusbar_win = build_window(frame);
	wbkgd(statusbar_win, COLOR_PAIR(2));
	return;
}

static void
print_statusbar(char *msg)
{
	wclear(statusbar_win);
	mvwaddstr(statusbar_win, 0, 1, msg);
	return;
}

static void
build_leftwin(void)
{
	struct win_frame frame;
	frame.rows = scr_rows - title_rows - statusbar_row;
	frame.cols = scr_cols/2 - 1;
	frame.y_pos = title_rows;
	frame.x_pos = 0;

	LeftWin.frame = frame;
	LeftWin.is_active = true;
	LeftWin.curs_pos = 0;
	LeftWin.max_curs_pos = frame.rows - 3; /* -3 porque las lineas 0 y rows-1 las usa box, y otra porque line arranca de 1 */

	left_win = build_window(frame);
	LeftWin.win = left_win;
	return;
}

static void
build_rightwin(void)
{
	struct win_frame frame;
	frame.rows = scr_rows - title_rows - statusbar_row;
	frame.cols = scr_cols/2 - 1;
	frame.y_pos = title_rows;
	frame.x_pos = scr_cols/2;

	RightWin.frame = frame;
	RightWin.is_active = false;
	RightWin.index_sel = 0;
	RightWin.curs_pos = 0;
	RightWin.max_curs_pos = frame.rows - 3; /* Idem que en LeftWin  */

	right_win = build_window(frame);
	RightWin.win = right_win;
}

static char *
truncate_filename(char *oldname, int max_length)
{
	char *newname, *buffer;

	newname = (char *) calloc ( max_length , sizeof(char) );

	if (strlen(oldname) >= max_length - 1) {
		buffer = (char *) calloc ( (max_length - 4) , sizeof(char) );
		strncpy( buffer, oldname, (max_length - 5) );
		snprintf(newname, max_length, "%s...%c", buffer, '\0');

		free(buffer);
	}
	else {
		snprintf(newname, max_length, "%s%c", oldname, '\0');
	}
	return newname;
}

static void
print_relative_number(TextWin WIN, int relative_number, int line)
{
	if (WIN.win != left_win)
		return;

	wattron(WIN.win, A_DIM);
	mvwprintw(WIN.win, line, 0, "%3d", relative_number);
	wattroff(WIN.win, A_DIM);
	return;
}

/* Highlight nombre de archivo y toda su fila */
static void
print_highlight(TextWin WIN, char *buffer, int line)
{
	wattron(WIN.win, COLOR_PAIR(2));
	mvwprintw(WIN.win, line, SANGRIA, "%-*.*s", WIN.frame.cols-SANGRIA, WIN.frame.cols-SANGRIA, buffer);
	wattroff(WIN.win, COLOR_PAIR(2));
	return;
}

/* Print nombre de archivo en negrita */
static void
print_dir(TextWin WIN, char *buffer, int line)
{
	wattron(WIN.win, A_BOLD);
	mvwprintw(WIN.win, line, SANGRIA, "%-*.*s", WIN.frame.cols-SANGRIA, WIN.frame.cols-SANGRIA, buffer);
	wattroff(WIN.win, A_BOLD);
	return;
}

/* Print el nombre del directorio que se muestra en esa ventana */
static void
print_nodename(TextWin WIN, Node *node)
{
	int x_pos;
	size_t len;
	char *buffer;

	buffer = truncate_filename(node->name, WIN.frame.cols-1);
	len = strlen(buffer);
	x_pos = (WIN.frame.cols/2) - (len/2);

	wattron(WIN.win, A_BOLD | COLOR_PAIR(1));
	mvwaddstr(WIN.win, 0, x_pos, buffer);
	wattroff(WIN.win, A_BOLD | COLOR_PAIR(1));

	free(buffer);
	return;
}

/* Print los nombres de archivos en una ventana  */
static void
print_textwin(TextWin WIN, Node *node)
{
	char *buffer;
	int i, line, relative_number, rnum_len;

	wclear(WIN.win);
	print_nodename(WIN, node);	
   
	/* Si el nodo a mostrar no es directorio, que no haga nada */
	if (node->is_dir == false)
		return;

	/* Avisar si el dir esta vacio */
	if (lselnode->is_dir == true && node->nchilds == 0) {
		mvwaddstr(WIN.win, 1, SANGRIA, "Directorio vacio.");
		return;
	}

	i = node->ntop_slice;
	rnum_len = 4;		/* Length del relative number. Ejemplo "  0 " */

	for (line=1; line < WIN.max_curs_pos + 2 && line < node->nchilds + 1; line++) { /* El +2 es empirico, lo encontre con prueba y error */

		if (line-1 == WIN.curs_pos)
			relative_number = -1;
		else
			relative_number = abs(line-1 - WIN.curs_pos);

		buffer = truncate_filename(node->childs[i++].name, (WIN.frame.cols - rnum_len - 1) );

		/* Highlight si esta bajo el cursor */
		if (WIN.is_active == true && line-1 == WIN.curs_pos) 
			print_highlight(WIN, buffer, line);
		/* Bold si es dir */
		else if (node->childs[i-1].is_dir == true) {
			print_relative_number(WIN, relative_number, line);
			print_dir(WIN, buffer, line);
		}
		/* Normal si es archivo comun */
		else {
			print_relative_number(WIN, relative_number, line);
			mvwaddnstr(WIN.win, line, SANGRIA, buffer, WIN.frame.cols-1);
		}

		free(buffer);
	}
}

/* Print nombres de archivos en ventanas izquierda y derecha */
static void
print_text_windows(void)
{
	print_textwin(LeftWin, lcurrnode);
	print_textwin(RightWin, lselnode);
}

static void
mv_curs_down(void)
{
	/* Si aun hay un nodo mas... */
	if (lcurrnode->index_sel != (lcurrnode->nchilds - 1)) {
		/* Si NO es la ultima fila, bajar una posición mas */
		if (LeftWin.curs_pos != LeftWin.max_curs_pos) {
			LeftWin.curs_pos++;
		}
		/* Si ES la ultima fila, SCROLLEAR */
		else if (LeftWin.curs_pos == LeftWin.max_curs_pos) {
			lcurrnode->ntop_slice++;
		}

		lcurrnode->index_sel++;
		lselnode = &lcurrnode->childs[lcurrnode->index_sel];
		print_text_windows();
	}
	return;
}

static void
mv_curs_up(void)
{
	/* Si no es el tope de la lista */
	if (lcurrnode->index_sel != 0) {
		if (LeftWin.curs_pos == 0)	/* Si el cursor esta en la 1ra fila, scrollear */
			lcurrnode->ntop_slice--;
		else
			LeftWin.curs_pos--;      
    
		lcurrnode->index_sel--;
		lselnode = &lcurrnode->childs[lcurrnode->index_sel];
		print_text_windows();
	}
	return;
}

/* Ingresa a un nodo */
static void
open_node(void)
{
	if (lselnode->is_dir == true) {
		/* Guardo estado del nodo que abro */
		lcurrnode->ncurs_pos = LeftWin.curs_pos;
		lcurrnode = lselnode;
		lselnode = &lselnode->childs[0];
		lcurrnode->index_sel = 0;
		LeftWin.curs_pos = 0;
		print_text_windows();
	}
	return;
}

/* Sale de un nodo */
static void
close_node(void)
{
	if (LeftWin.is_active && lcurrnode != root_node) {
		/* Al cerrar, limpio info del cursor en el nodo */
		lcurrnode->index_sel = 0;
		lcurrnode->ncurs_pos = 0;
		lcurrnode->ntop_slice = 0;

		/* Paso al nodo parent */
		lcurrnode = lcurrnode->parent;
		LeftWin.curs_pos = lcurrnode->ncurs_pos;
		lselnode = &lcurrnode->childs[lcurrnode->index_sel];

		print_text_windows();
	}
	return;
}

static void
relnum_mv_down(int num)
{
	while(num > 0) {
		mv_curs_down();
		num--;
	}
	return;
}

static void
relnum_mv_up(int num)
{
	while(num > 0) {
		mv_curs_up();
		num--;
	}
	return;
}

static void
relative_num_move(char first_num)
{
	int num = first_num - 48; /* Key gives the number in ASCII. Substracting 48 gives the real number */

	int k = getch();
	if (k == 'j' || k == KEY_DOWN)
		relnum_mv_down(num);
	else if (k == 'k' || k == KEY_UP)
		relnum_mv_up(num);

	/* Solo se permiten 2 digitos para evitar excesivas llamadas.
	   Ademas jamas se moveria de a 100 lugares */
	else if (k == '0' || k == '1' || k == '2' || k == '3' || k == '4' ||
		 k == '5' || k == '6' || k == '7' || k == '8' || k == '9') {
		num = (10*num) + (k-48);

		int k = getch();
		if (k == 'j' || k == KEY_DOWN)
			relnum_mv_down(num);

		else if (k == 'k' || k == KEY_UP)
			relnum_mv_up(num);
	}
	return;
}

static void
rebuild_all(void)
{
	getmaxyx(stdscr, scr_rows, scr_cols);
	clear();
	refresh();
	build_title();
	print_title();
	build_statusbar();
	print_statusbar("Soy una statusbar");
	build_leftwin();
	build_rightwin();
	print_text_windows();
}

static int
open_pdf(void)
{
	if (lselnode->is_dir)
		return 1;

	pid_t pid;
	pid = fork();

	if (pid == -1)
		return -1;

	else if (pid == 0) {
		char *mupdf_args[] = {"mupdf", lselnode->path, NULL};
		if (execvp("mupdf", mupdf_args) == -1) {
			perror("Error en la función open_pdf. No se pudo abrir MuPDF\n");
		}
	}
	return 0;
}

static void
goto_top(void)
{
	/* Si no hay nodos hijos, return */
	if (lcurrnode->nchilds == 0)
		return;  

	lselnode = &lcurrnode->childs[0];
	lcurrnode->index_sel = 0;
	LeftWin.curs_pos = 0;
	lcurrnode->ntop_slice = 0;
	print_text_windows();
	return;
}

static void
goto_bottom(void)
{
	/* Si no hay nodos hijos, return */
	if (lcurrnode->nchilds == 0)
		return;

	/* Busca ultimo child */
	int i = lcurrnode->nchilds - 1;
	lselnode = &lcurrnode->childs[i];
	lcurrnode->index_sel = i;

	if (lcurrnode->index_sel < LeftWin.max_curs_pos)
		LeftWin.curs_pos = lcurrnode->index_sel;
	else {
		LeftWin.curs_pos = LeftWin.max_curs_pos;
		lcurrnode->ntop_slice = lcurrnode->index_sel - LeftWin.max_curs_pos;
	}

	print_text_windows();
	return;
}

static void
init_tui(void)
{
	/* Inicio de curses */
	initscr();
	cbreak();
	noecho();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	curs_set(0);

	start_color();
	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_BLUE);

	/* Consigo FILAS y COLUMNAs */
	getmaxyx(stdscr, scr_rows, scr_cols);

	refresh();
	build_title();
	print_title();
	build_statusbar();
	print_statusbar("Soy una statusbar");
	build_leftwin();
	build_rightwin();
  
	lcurrnode = root_node;
	lselnode = &root_node->childs[0];

	print_text_windows();
	return;
}

static void
refresh_wins(void)
{
	wnoutrefresh(title_win);
	wnoutrefresh(statusbar_win);
	wnoutrefresh(left_win);
	wnoutrefresh(right_win);
	doupdate();
}


static void
usage(void)
{
  	fprintf(stderr, "Modo de uso:\n");
	fprintf(stderr, "Hemeroteca [-d dir] [-s keyword]\n");
	fprintf(stderr, " -d DIR	Abre hemeroteca en [dir]\n");
	fprintf(stderr, " -s KEYWORD	Busca archios con KEYWORD en el nombre\n");
}
