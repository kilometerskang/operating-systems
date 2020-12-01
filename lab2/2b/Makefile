# NAME: Miles Kang
# EMAIL: milesjkang@gmail.com
# ID: 405106565


default:
	gcc -Wall -Wextra -g -o lab2_list -pthread -lprofiler lab2_list.c SortedList.c
tests: default
	chmod +x test.sh
	-./test.sh 2>/dev/null
profile:
	rm -f profile.gperf profile.out
	LD_PRELOAD=/usr/lib64/libprofiler.so.0
	CPUPROFILE=profile.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	/u/cs/ugrad/miles/111/2/bin/pprof --text lab2_list profile.gperf > profile.out
	/u/cs/ugrad/miles/111/2/bin/pprof --list=thread_func lab2_list profile.gperf >> profile.out
	rm -f profile.gperf
graphs: tests
	chmod +x lab2_list.gp
	./lab2_list.gp
clean:
	rm -f lab2_list *.tar.gz
dist: graphs
	tar -czvf lab2b-405106565.tar.gz SortedList.h SortedList.c profile.out \
	lab2_list.c lab2b_list.csv *.png lab2_list.gp README Makefile test.sh