compile:
	gcc src/main.c -o bin/text-splitter -lncurses
run: compile
	bin/text-splitter bin/tasks
