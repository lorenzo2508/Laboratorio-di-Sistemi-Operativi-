CC = gcc
CFLAGS += -std=gnu99 -Wall -g
INCLUDE = -I ./include/
LIBS = -lpthread
BUILD = ./
OBJ_DIR := ./obj
SRC_DIR := ./src
TEST_DIR := ./testdir
DUMMIES_DIR = ./testDir

TARGETS = farm generafile

.DEFAULT_GOAL := all

OBJ-generafile = obj/generafile.o
OBJ-farm = obj/node.o obj/linkedList.o obj/queue.o obj/master_thread.o obj/thread_pool.o obj/farm.o 

obj/node.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/node.c $(LIBS)
	@mv node.o $(OBJ_DIR)/node.o

obj/linkedList.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/linkedList.c $(LIBS)
	@mv linkedList.o $(OBJ_DIR)/linkedList.o

obj/queue.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/queue.c $(LIBS)
	@mv queue.o $(OBJ_DIR)/queue.o

obj/master_thread.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/master_thread.c $(LIBS)
	@mv master_thread.o $(OBJ_DIR)/master_thread.o

obj/thread_pool.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/thread_pool.c $(LIBS)
	@mv thread_pool.o $(OBJ_DIR)/thread_pool.o

obj/generafile.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/generafile.c $(LIBS)
	@mv generafile.o $(OBJ_DIR)/generafile.o

obj/farm.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c src/farm.c $(LIBS)
	@mv farm.o $(OBJ_DIR)/farm.o

generafile:	$(OBJ-generafile)
			$(CC) $(CFLAGS) $(INCLUDE) -o $(BUILD)/generafile $(OBJ-generafile) $(LIBS)

farm:   $(OBJ-farm)
		$(CC) $(CFLAGS) $(INCLUDE) -o $(BUILD)/farm $(OBJ-farm) $(LIBS)

normalexe:	farm
			@chmod +x scripts/normalexe.sh
			scripts/normalexe.sh

test:	farm
		@chmod +x ./test.sh
		./test.sh


all: $(TARGETS)

.PHONY: cleanall all 
cleanall:
	rm -f $(OBJ_DIR)/* ./*.dat ./*.txt 
	rm --recursive -f $(TEST_DIR)
	@touch $(OBJ_DIR)

	
