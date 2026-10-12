DATA_PREFIX = ../../data
#DATA = $(DATA_PREFIX)/railwayplanning/secret/3large.in
#DATA = $(DATA_PREFIX)/railwayplanning/secret/4huge.in
#DATA = $(DATA_PREFIX)/big/002.in
#DATA = $(DATA_PREFIX)/tiny/0.in
DATA = $(DATA_PREFIX)/railwayplanning/secret/0mini.in
#DATA = ../data/railwayplanning/secret/1small.in
#DATA = ../data/huge/001.in

all:
	time sh check-solution.sh clojure preflow.clj
	@echo PASS all tests

test:
	clojure preflow.clj < $(DATA)
