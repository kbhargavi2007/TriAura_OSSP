CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Wextra -std=gnu11 -pthread
LDLIBS  = -lm -lrt -pthread
BINS    = qserver qdoctor qclient qtriage qstatus

all: $(BINS)

qserver: qserver.c oslab.c queueless.h oslab.h
	$(CC) $(CFLAGS) -o $@ qserver.c oslab.c $(LDLIBS)
qdoctor: qdoctor.c queueless.h
	$(CC) $(CFLAGS) -o $@ qdoctor.c $(LDLIBS)
qclient: qclient.c queueless.h
	$(CC) $(CFLAGS) -o $@ qclient.c $(LDLIBS)
qtriage: qtriage.c queueless.h
	$(CC) $(CFLAGS) -o $@ qtriage.c $(LDLIBS)
qstatus: qstatus.c queueless.h
	$(CC) $(CFLAGS) -o $@ qstatus.c $(LDLIBS)

# CO-4: memory debugging tools.  make asan  -> run ./qserver with AddressSanitizer
asan:
	$(CC) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -std=gnu11 -pthread -o qserver qserver.c oslab.c $(LDLIBS)
	$(CC) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -std=gnu11 -pthread -o qdoctor qdoctor.c $(LDLIBS)
# CO-6: data race detection with ThreadSanitizer
tsan:
	$(CC) -O1 -g -fsanitize=thread -std=gnu11 -pthread -o qserver qserver.c oslab.c $(LDLIBS)

test: all
	./test_priority.sh

clean:
	rm -f $(BINS) /tmp/queueless.reg /tmp/queueless.board /tmp/queueless.reply.*
distclean: clean
	rm -rf qdata
.PHONY: all asan tsan test clean distclean
