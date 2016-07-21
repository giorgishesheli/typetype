/*
* MAJOR todo:
* 1. Add scrolling
* 2. add WPM
* MINOR tood:
* 1. implement highlighting with chtype |  not a good idea
* 2. nonalphanumeric characters count as multiple errors
* 3. fix bottom bar
*/
#define _DEFAULT_SOURCE 
/* Lesson learned: without feature test macro in glibc 2.22-9 signal function  acts as System V signal
*  only gets fired once.
*/
#include <curses.h>
#include <stdlib.h>// exit
#include <ctype.h> // isblank
#include <signal.h>
#include <sys/time.h>  //setitimer
#include <unistd.h> //kill
#include <stdio.h> // fprintf


//global variables
int cur_position = 0;
int mistakes = 0;
int words = 0;
int seconds;
WINDOW *bot, *top;
void (*old_handler)(int); // ncurses handler for SIGWINCH
int buf_count; 
char *buf;
char *buf_opt; // Holds color codes for corresponding buffer
#define CORRECT_COLOR 1
#define INCORRECT_COLOR 2
#define CURSOR_COLOR 3





void draw_top(){
	int cur_x, cur_y, i;
	//int temp_x, temp_y;
	int COLOR_ = 0;
	werase(top);
	wmove(top, 0, 0);
	for(i = 0; i < buf_count; i++){
		if((int) buf_opt[i]  == 1)
			COLOR_ = COLOR_PAIR(CORRECT_COLOR);
		if((int) buf_opt[i] == 2)
			COLOR_ = COLOR_PAIR(INCORRECT_COLOR);
		if(i == cur_position){
			getyx(top, cur_y, cur_x);
			COLOR_ = COLOR_PAIR(CURSOR_COLOR);
		}
		waddch(top, buf[i] | COLOR_);
		//getyx(top, temp_y, temp_x);
		//fprintf(stderr, "added char: %c		ROW: %ld	COL: %ld 	ROW/COL: %ld/%ld\n", buf[i], (long) temp_y,(long)  temp_x, (long) LINES, (long) COLS);
		COLOR_ = 0;
	}
	wmove(top, cur_y, cur_x);
	wrefresh(top);
}

void handle_bot(char ch){
	if(buf[cur_position] != ch)
		mistakes += 1;
	if(buf[cur_position] == ch && isblank(ch)){
		words += 1; //TODO - stupid placement
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

		if((unsigned char) ch == 0x9a) //TODO -  VERY UGLY HACK, get to the root of the problem
			continue;

		//fprintf(stderr, "freaking character typed: %x \n", (unsigned char) ch);
		if(seconds == 0){  
			//Init timer
			kill(getpid(), SIGALRM);
			struct itimerval itv;
			itv.it_value.tv_sec = 1;
			itv.it_value.tv_usec = 0;
			itv.it_interval.tv_sec = 1;
			itv.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &itv, NULL); //TODO - error checking
		}
		handle_bot(ch);
		if(buf[cur_position] == ch){
			buf_opt[cur_position] = 1;
			cur_position++;
			draw_top();
		} else if(ch == 127 || ch == 8){
			buf_opt[cur_position - 1] = 0;
			cur_position--;
			draw_top();
		} else {
			buf_opt[cur_position] = 2;
			cur_position++;
			draw_top();
		}

	}

}


void resize_handler(int a){
	old_handler(a);

	refresh(); // LL: LINES and COLS are not reseted by old handler, need to call refresh
	erase(); // LL: stdscr retains remains of top screen, even after we erase top

	delwin(top);
	delwin(bot);

	top = newwin(LINES - LINES / 2 - 1, COLS - 2, 1, 1);
	wrefresh(top);
	bot = newwin(LINES / 3, COLS - 2, LINES - LINES/3, 1);
	wrefresh(bot);

	draw_top();

}

void timer_handle(int a){
	seconds += 1;	
	int wpm =  words * 60 / seconds; 
	mvwprintw(bot, 5, 1, "words: %d  seconds: %d  wpm: %d", words, seconds, wpm);
	mvwprintw(bot, 4, 8, "%d", wpm);
	wrefresh(bot);
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
	use_default_colors(); // so we can use -1 for default values
	init_pair(CORRECT_COLOR, COLOR_GREEN, -1);
	init_pair(INCORRECT_COLOR, COLOR_YELLOW, COLOR_RED);
	init_pair(CURSOR_COLOR, -1, COLOR_YELLOW);
	curs_set(0); // makes cursor invisible

	//init windows
	top = newwin(LINES - LINES / 2 - 1, COLS - 2, 1, 1);
	wrefresh(top);
	bot = newwin(LINES / 3, COLS - 2, LINES - LINES/3, 1);
	wrefresh(bot);

	//initiate SIGWINCH handler
	struct sigaction sa, sa_old;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; // LL: without RESTART blocking syscall returns in getch
	sa.sa_handler = resize_handler;

	if(sigaction(SIGWINCH, &sa, &sa_old) == -1){
		perror("sigaction");
		endwin();
		exit(EXIT_FAILURE);
	}
	old_handler = sa_old.sa_handler;
	//old_handler = signal(SIGWINCH, resize_handler);

	signal(SIGALRM, timer_handle);




	//read to buffer
	int x, y;
	getmaxyx(top, y, x);
	buf_count = y * x;

	buf = malloc(buf_count);
	buf_opt = malloc(buf_count);


	FILE *file_src;
	if((file_src = fopen(argv[1], "r")) == NULL){
		perror("fopen");
		endwin();
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
