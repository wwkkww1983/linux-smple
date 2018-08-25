
CC=arm-linux-gnueabihf-
DIR=./
target:
	$(CC)gcc -o uart_test uart_test.c cJSON.c -lsqlite3 -I$(DIR)include -L$(DIR)lib -lpthread -lm
clean:
	@rm -vf uart_test
