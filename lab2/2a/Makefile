# NAME: Miles Kang
# EMAIL: milesjkang@gmail.com
# ID: 405106565


default:
	gcc -Wall -Wextra -g -o lab2_add -pthread lab2_add.c
	gcc -Wall -Wextra -g -o lab2_list -pthread lab2_list.c SortedList.c
build:
	gcc -Wall -Wextra -g -o lab2_add -pthread lab2_add.c
	gcc -Wall -Wextra -g -o lab2_list -pthread lab2_list.c SortedList.c
tests: build
	-./test.sh 2>/dev/null
graphs: tests
	./lab2_add.gp
	./lab2_list.gp
clean:
	rm -f lab2_add lab2_list *.tar.gz
dist: graphs
	tar -czvf lab2a-405106565.tar.gz SortedList.h SortedList.c lab2_list.c lab2_add.c lab2_add.csv lab2_list.csv *.png *.gp README Makefile test.sh