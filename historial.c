/*
 * Muestra el historial del megatron en forma de tabla
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "historial.h"

#define NAME_MAX 255

static char 	*copy_value(char *s);
static void 	free_table(char ***table, int n_rows, int n_cols);
static int 	get_rows_and_cols(FILE *);
static void 	save_col_len(char *buffer, int *max_col_len);

static int n_rows;
static int n_cols;
static char ***table;
static int *cols_width;		/* Ancho de cada columna de la tabla */

int
print_historial(void)
{
	FILE *f;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	int n, m;		/* Dimensiones de la tabla: n = rows , m = cols */
	char *s;		/* String que se usa con strtok */

	f = fopen(HISTORY_FILE, "r");
	if (f == NULL)
		return -1;

	/* Obtengo numero de rows y cols */  
	get_rows_and_cols(f);


	/* Creat Tabla */

	table = (char ***) malloc ( n_rows * sizeof(char *) );
	cols_width = (int *) malloc ( n_cols * sizeof(int) ); /* Guarda la maxima long de cada columna */

	n = m = 0;
	while ((nread = getline(&line, &len, f)) != -1) {
		table[n] = (char **) calloc ( n_cols , sizeof(char *) ); 

		s = strtok(line, "/");
		m = 0;
		table[n][m] = copy_value(s);
		save_col_len(s, &cols_width[m]);
		m++;
		while ((s = strtok(NULL, "/")) != NULL) {
			table[n][m] = copy_value(s);
			save_col_len(s, &cols_width[m]);
			m++;
		}
		n++;
	}

	/* Print tabla */
	n = 0;
	m = 2;			/* m=2 para evitar columnas home/usr. Lo mismo en el loop de abajo */
	for (n = 0; n < n_rows; n++) {
		printf("%3d\t", n+1);

		for (m = 2; m < n_cols; m++) {
			if (table[n][m] != NULL)
				printf("%-*s\t", cols_width[m] + 1, table[n][m]);
		}
		printf("\n");
	}

	free_table(table, n_rows, n_cols);
	free(cols_width);
	free(line);
	fclose(f);
	return 0;
}

int
save_to_historial(const char *path)
{
	FILE *phistory;
	phistory = fopen(HISTORY_FILE, "a");
	if (phistory == NULL) {
		perror("Error save_to_history: no se pudo abrir el archivo historial\n");
		return -1;
	}
	fprintf(phistory, "%s\n", path);
	fclose(phistory);
	return 0;
}

static char *
copy_value(char *s)
{
	char *buffer;

	if (s[strlen(s) - 1] == '\n')
		s[strlen(s) - 1] = '\0';
	buffer = strdup(s);
	return buffer;
}

static void
save_col_len(char *buffer, int *max_col_len)
{
	if (buffer == NULL)
		return;
	int len = strlen(buffer);
	if (len > *max_col_len)
		*max_col_len = len;
}
  
static int
get_rows_and_cols(FILE *fp)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	int n_seps;

	n_rows = n_cols = 0;
	while ((nread = getline(&line, &len, fp)) != -1) {
		/* Cuenta filas */
		n_rows++;
		/* Cuenta maximo n de columnas */
		n_seps = 0;
		for (int i=0; line[i] != '\n'; i++) {
			if (line[i] == '/')
				n_seps++;
		}
		n_cols = n_seps > n_cols ? n_seps : n_cols;
	}
	fseek(fp, 0, SEEK_SET);
	return 0;
}

static void
free_table(char ***table, int n_rows, int n_cols)
{
	int n = 0;
	int m = 0;

	for (n=0; n < n_rows; n++) {
		for (m = 0; m < n_cols; m++) {
			if (table[n][m] != NULL) 
				free(table[n][m]);
		}
		free(table[n]);
	}
	free(table);
}
