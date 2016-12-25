#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>

/**
 * Task struct
 */
typedef struct {
	char * taskname;
	unsigned long timeleft;
} task_item;

/**
 * Timer struct
 */
typedef struct {
	unsigned long milliseconds;
	unsigned long seconds;
	unsigned long minutes;
	unsigned long hours;
	unsigned long days;
} timer_struct;

int items_count = 0;
int selected_item = 0;
task_item ** items = NULL;
int max_line_width = 0;

bool is_running = true;

/**
 * Add an item to list
 */
void add_item(char * name) {
	items_count++;

	items = realloc(items, sizeof(task_item *) * items_count);

	task_item * new_item = (task_item*) calloc(1, sizeof(task_item));
	new_item->taskname = (char *)malloc(strlen(name)+1);
	new_item->taskname = strcpy(new_item->taskname, name);
	new_item->timeleft = 0;

	items[items_count - 1] = new_item;
}

int last_char = 0;

/**
 * Get max value of two arguments
 */
long max(long val1, long val2) {
	return (val2 > val1) ? val2 : val1;
}

/**
 * Get timer values
 */
timer_struct get_timer(unsigned long milliseconds) {
	timer_struct result;

	memset(&result, 0, sizeof(timer_struct));

	result.milliseconds = milliseconds;

	result.seconds = milliseconds / 1000;
	result.milliseconds = milliseconds % 1000;

	if(result.seconds >= 60) {
		result.minutes = result.seconds / 60;
		result.seconds = result.seconds % 60;
	}

	if(result.minutes >= 60) {
		result.hours = result.minutes / 60;
		result.minutes = result.minutes % 60;
	}

	if(result.hours >= 24) {
		result.days = result.hours / 24;
		result.hours = result.hours % 24;
	}

	return result;
}

long total_time() {
	long result = 0;

	for(int i = 0; i < items_count; i++) {
		result += items[i]->timeleft;
	}

	return result;
}

/**
 * Get nanoseconds difference
 */
uint64_t time_diff(struct timespec * start, struct timespec * end) {
	return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_nsec - start->tv_nsec) / 1000;
}

struct timespec start_time;
struct timespec last_time;

/**
 * Render function
 */
void render() {
	int max_y;
	int max_x;

	getmaxyx(stdscr, max_y, max_x);

	clear();
	
	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);

	uint64_t delta = time_diff(&last_time, &current_time); //.tv_nsec - last_time.tv_nsec;
	
	last_time = current_time;

	for(int i = 0; i < items_count; i++) {
		if(selected_item == i) {
			items[i]->timeleft += delta; 
			attrset(A_STANDOUT);
		} else {
			attrset(A_NORMAL);
		}
		timer_struct tmr = get_timer(items[i]->timeleft / 1000);
		mvprintw(i, 0, "  %d. %s", i, items[i]->taskname);
		mvprintw(i, max_line_width + 5, "%02d:%02d:%02d.%02d", tmr.hours, tmr.minutes, tmr.seconds, tmr.milliseconds / 10);
	}

	timer_struct total_tmr = get_timer(total_time() / 1000);

	attrset(A_NORMAL);
	mvprintw(max_y-1, 0, "Total time: %02d:%02d:%02d.%02d", total_tmr.hours, total_tmr.minutes, total_tmr.seconds, total_tmr.milliseconds / 10);

	refresh();

	int ch = getch();

	if(ch != -1) {
		last_char = ch;
	}

	switch (ch) {
		case KEY_UP:
			selected_item--;
			if(selected_item<0)
				selected_item = items_count-1;
			break;

		case KEY_DOWN:
			selected_item++;
			if(selected_item>=items_count)
				selected_item = 0;
			break;

		case 'q':
		case 'Q':
			is_running = false;
			break;

		default:
			if((ch >= 48) && (ch <= 57)) {
				int selection = ch - 48;

				if(selection < items_count) {
					selected_item = selection;
				}
			}
	}
}

/**
 * Entry point to the program
 */
int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Not enought (or too many) arguments");
		return 1;
	}

	char * filename = argv[1];
	FILE * file = fopen(filename, "r");

	if (file == NULL) {
		printf("Couldn't open file: %s", filename);
		return 1;
	}

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, file)) != -1) {
		max_line_width = max(max_line_width, strlen(line));
		add_item(line);
	}

	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	timeout(10);
	noecho();
	curs_set(FALSE);

	clock_gettime(CLOCK_REALTIME, &last_time);
	clock_gettime(CLOCK_REALTIME, &start_time);

	while(is_running) {
		render();
	}

	endwin();

	return 0;
}
