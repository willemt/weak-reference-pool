CONTRIB_DIR = ..
HASHMAP_DIR = $(CONTRIB_DIR)/CHashMapViaLinkedList
FIXEDARRAY_DIR = $(CONTRIB_DIR)/CFixedArrayList

GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CC     = gcc
CCFLAGS = -g -O2 -Wall -Werror -W -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(HASHMAP_DIR) -I$(FIXEDARRAY_DIR)

all: tests

chashmap:
	mkdir -p $(HASHMAP_DIR)/.git
	git --git-dir=$(HASHMAP_DIR)/.git init 
	pushd $(HASHMAP_DIR); git pull git@github.com:willemt/CHashMapViaLinkedList.git; popd

cfixedarraylist:
	mkdir -p $(FIXEDARRAY_DIR)/.git
	git --git-dir=$(FIXEDARRAY_DIR)/.git init 
	pushd $(FIXEDARRAY_DIR); git pull git@github.com:willemt/CFixedArrayList.git; popd

download-contrib: chashmap cfixedarraylist

main.c:
	if test -d $(HASHMAP_DIR); \
	then echo have contribs; \
	else make download-contrib; \
	fi
	sh make-tests.sh > main.c

tests: main.c weak_reference_pool.o test_weak_reference_pool.c CuTest.c main.c $(HASHMAP_DIR)/linked_list_hashmap.c $(FIXEDARRAY_DIR)/fixed_arraylist.c 
	$(CC) $(CCFLAGS) -o $@ $^
	./tests
	gcov main.c test_weak_reference_pool.c weak_reference_pool.c

weak_reference_pool.o: weak_reference_pool.c 
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c weak_reference_pool.o tests $(GCOV_OUTPUT)
