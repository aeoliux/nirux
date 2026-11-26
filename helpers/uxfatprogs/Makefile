CC := cc

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

all: mkuxfs uxmount

mkuxfs: mkuxfs.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

uxmount: uxmount.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)