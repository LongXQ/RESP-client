compile=gcc -o RESP-client
obj-name=RESP-client
source_file=main.c hiredis.c net.c read.c sds.c
flag="-std=c99"
all:
	$(compile) $(flag) -o $(obj-name) $(source_file)
