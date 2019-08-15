# Install libcurl and json-c
# yum install libcurl-devel json-c-devel
LIBS = -lrt -lcurl -ljson-c `mysql_config --libs_r`
INC = -I. `mysql_config --include`
#DEFS = DDEBUG

CFLAGS = -w -O3 -g

TRANSACTIONS = neword.o payment.o ordstat.o delivery.o slev.o
OBJS = main.o spt_proc.o driver.o support.o sequence.o rthist.o sb_percentile.o http.o $(TRANSACTIONS)

.SUFFIXES:
.SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) $(INC) $(DEFS) -c $*.c

all: ../tpcc_load ../tpcc_start

../tpcc_load : load.o support.o
	$(CC) load.o support.o $(LIBS) -o ../tpcc_load

../tpcc_start : $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o ../tpcc_start

clean :
	rm -f *.o
	rm -f ../tpcc_load
	rm -f ../tpcc_start
