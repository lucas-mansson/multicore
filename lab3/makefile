#DATA = ../data/railwayplanning/secret/3large.in
#DATA = ../data/railwayplanning/secret/4huge.in
#DATA = ../data/big/002.in
#DATA = ../data/tiny/0.in
#DATA = ../data/railwayplanning/secret/1small.in
DATA = ../data/huge/001.in

main:
	gcc driver.c forsete.c -o driver  -O3 -pthread
	time sh check-solution.sh ./driver
	@echo PASS all tests

test: 
	gcc -o preflow preflow.c -g -O3 -pthread
	time ./preflow < $(DATA) 

hg:
	gcc -o preflow preflow.c -g -O3 -pthread
	valgrind --tool=helgrind ./preflow < $(DATA)

tsan:
	gcc -o preflow preflow.c -g -O3 -pthread -fsanitize=thread
	./preflow < $(DATA)
