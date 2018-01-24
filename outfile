CC = gcc
CFLAGS = -Wall -g
EXEC = manager listener worker
DEPS = hlpfncts.h

all: $(EXEC)

manager: manager.o workersQueue.o hlpfncts.o
	$(CC) $^ -o $@

listener: listener.o
	$(CC) $^ -o $@

worker: worker.o domainList.o
	$(CC) $^ -o $@

.c.o:
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o

manager.o: $(DEPS) workersQueue.h
listener.o: $(DEPS)
worker.o: $(DEPS) domainList.h
hlpfncts.o: hlpfncts.h
workersQueue.o: workersQueue.h
domainList.o: domainList.h
