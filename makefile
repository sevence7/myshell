myshell : myshell.h myshell.c test.c
	  sudo gcc myshell.h myshell.c test.c -o myshell -lreadline -lcrypt
	  sudo chmod u+s ./myshell
