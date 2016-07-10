/*
GLOBAL todo:
1. implement highlighting with chtype
2. nonalphanumeric characters count as multiple errors
3. during multiple mistakes, cursor remains red

*/
#define _DEFAULT_SOURCE // for some reason, signal handler is fired only once without this feature test macro
#include <curses.h>
#include <stdlib.h>// exit
#include <ctype.h> // isblank
#include <signal.h>


//global variables
int cur_position = 0;
int mistakes = 0;
int words = 0;
WINDOW *bot, *top;
void (*old_handler)(int);
int buf_count; 
char *buf;
char *buf_opt; // 0 - no highlight, 1 - green ( correct), 2 - red (mistake)





void draw_top(){
	int cur_x, cur_y, i;
	int COLOR_ = 0;
	werase(top);
	wmove(top, 0, 0);
	for(i = 0; i < buf_count; i++){
		if((int) buf_opt[i]  == 1)
			COLOR_ = COLOR_PAIR(1);
		if((int) buf_opt[i] == 2)
			COLOR_ = COLOR_PAIR(2);
		if(i == cur_position)
			getyx(top, cur_y, cur_x);
		waddch(top, buf[i] | COLOR_);
		COLOR_ = 0;
	}
	wmove(top, cur_y, cur_x);
	wrefresh(top);
}

void handle_bot(char ch){
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
		handle_bot(ch);
		if(buf[cur_position] == ch){
			buf_opt[cur_position] = 1;
			cur_position++;
			draw_top();
			wchgat(top, 1, 0, 3, NULL);
		} else if(ch == 127 || ch == 8){
			buf_opt[cur_position - 1] = 0;
			cur_position--;
			draw_top();
			wchgat(top, 1, 0, 3, NULL);
		} else {
			buf_opt[cur_position] = 2;
			cur_position++;
			draw_top();
			wchgat(top, 1, 0, 2, NULL);
		}

	}

}


void resize_handler(int a){
	old_handler(a);
	draw_top();

}

int main(int argc, char **argv){
	//Initial variables
	if(argc != 2){
		fprintf(stderr, "Usage: %s [file] \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//ncurses init
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	//init color pairs
	use_default_colors();
	init_pair(1, COLOR_GREEN, -1);
	init_pair(2, COLOR_YELLOW, COLOR_RED);
	init_pair(3, -1, COLOR_YELLOW);
	curs_set(0);

	//init windows
	top = newwin(LINES - LINES / 3 - 1, COLS - 2, 1, 1);
	wrefresh(top);
	bot = newwin(LINES / 3, COLS - 2, LINES - LINES/3, 1);
	wrefresh(bot);

	//initiate SIGWINCH handler
	old_handler = signal(SIGWINCH, resize_handler); //TODO: - cfe

	//read to buffer
	int x, y;
	getmaxyx(top, y, x);
	buf_count = y * x;

	buf = malloc(buf_count);
	buf_opt = malloc(buf_count);


	FILE *file_src;
	if((file_src = fopen(argv[1], "r")) == NULL){
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	if(fread(buf, buf_count  - 1, 1, file_src) == 0){
		printf("end of file or read error, dunno");
	}

	// Initial text
	draw_top();

	//Init bottom
	mvwaddstr(bot, 1, 1, "Key: ");
	mvwaddstr(bot, 2, 1, "Mistakes: ");
	mvwaddstr(bot, 3, 1, "Words: ");
	mvwaddstr(bot, 4, 1, "WPM: ");


	//call main loop
	main_loop(buf, buf_opt);
	

	//End of program
	endwin();
	exit(EXIT_SUCCESS);
}
