testaro: testaro.c
	gcc -Wall -Werror -Wextra -o testaro testaro.c

test: testaro test.sh
	bash test.sh
