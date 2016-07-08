#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#define VISIBLE_TEXT_SIZE 9000

//global variables
int cur_position = 0;
int mistakes = 0;
int words = 0;
WINDOW *bot, *top;

void handle_top(char *buf, char *buf_opt){
	int cur_x, cur_y, i, COLOR_;
	werase(top);
	wmove(top, 1, 0);
	for(i = 0; i < strlen(buf); i++){
		if((int) buf_opt[i]  == 1)
			COLOR_ = COLOR_PAIR(1);
		if(i == cur_position)
			getyx(top, cur_y, cur_x);
		waddch(top, buf[i] | COLOR_);
		COLOR_ = 0;
	}
	wmove(top, cur_y, cur_x);
	wrefresh(top);
}

void handle_bot(char ch, char *buf){
	if(buf[cur_position] != ch)
		mistakes += 1;
	if(buf[cur_position] == ch && isblank(ch)){
		words += 1;
	}
	char mstk[20];
	char wrds[20];
	sprintf(mstk, "%d", mistakes);
	sprintf(wrds, "%d", words);
	int cur_x, cur_y;
	getyx(top, cur_y, cur_x);
	mvwaddch(bot, 1, 6, ch);
	mvwaddstr(bot, 2, 11, mstk);
	mvwaddstr(bot, 3, 8, wrds);
	wrefresh(bot);
	wmove(top, cur_y, cur_x);
}

void main_loop(char *buf, char *buf_opt){
	for(;;){
		chtype ch = wgetch(top);
		handle_bot(ch, buf);
		if(buf[cur_position] == ch){
			buf_opt[cur_position] = 1;
			cur_position++;
			handle_top(buf, buf_opt);
		}
	}

}

int main(int argc, char **argv){
	//Initial variables
	if(argc != 2){
		fprintf(stderr, "Usage: %s [file] \n", argv[0]);
		exit(EXIT_FAILURE);
	}


	char buf[VISIBLE_TEXT_SIZE];
	char buf_opt[VISIBLE_TEXT_SIZE] = {0};

	FILE *file_src;
	if((file_src = fopen(argv[1], "r")) == NULL){
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	if(fread(buf, VISIBLE_TEXT_SIZE - 1, 1, file_src) == 0){
		printf("end of file or read error, dunno");
	}
	buf[VISIBLE_TEXT_SIZE] = '\0';


	//ncurses init
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);


	//init color pairs
	use_default_colors();
	init_pair(1, COLOR_GREEN, -1);


	top = newwin(LINES - LINES / 3 - 1, COLS - 2, 1, 1);
	//box(top, 0, 0);
	wrefresh(top);
	bot = newwin(LINES / 3, COLS - 2, LINES - LINES/3, 1);
	//box(bot, 0, 0);
	wrefresh(bot);


	// Initial text
	int i;
	wmove(top, 1, 1);
	for(i = 0; i < strlen(buf); i++){
		waddch(top, buf[i]);
	}
	wrefresh(top);
	wmove(top, 1, 1);

	mvwaddstr(bot, 1, 1, "Key: ");
	mvwaddstr(bot, 2, 1, "Mistakes: ");
	mvwaddstr(bot, 3, 1, "Words: ");


	//call main loop
	main_loop(buf, buf_opt);
	

	//End of program
	endwin();
	exit(EXIT_SUCCESS);
}
