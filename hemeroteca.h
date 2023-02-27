#ifndef _MEGATRON_H
#define _MEGATRON_H

#include <stdbool.h>

#include "filetree.h"

#define SANGRIA 5

const unsigned int title_rows = 3;
const unsigned int statusbar_row = 1;

enum scroll_direction {
  DOWN = -1,
  UP   =  1,
};

extern int scr_rows;		
extern int scr_cols;		

struct win_frame {
  unsigned int rows;
  unsigned int cols;
  unsigned int y_pos;
  unsigned int x_pos;
};

typedef struct {
  struct win_frame frame;
  WINDOW *win;
  bool is_active;
  unsigned int index_sel;
  unsigned int curs_pos;
  unsigned int max_curs_pos;	
} TextWin;


extern WINDOW *left_win;
extern WINDOW *right_win;
extern WINDOW *title_win;
extern WINDOW *statusbar_win;


extern TextWin LeftWin;
extern TextWin RightWin;

#endif
