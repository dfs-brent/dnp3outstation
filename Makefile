
#LIBS = -lhtlib -lpthread -lmysqlclient -lxml2 -lrt -lsqlbase -ltac2 
LIBS = -lhtlib -lpthread -lmysqlclient -lopendnp3 -lrt
#INCLUDE = -I /home/dfs/projects/htlib -I ../../lib -I /usr/include/libxml2 -I ../../sqlbase -I ./
INCLUDE = -I /home/dfs/projects/htlib -I ../../lib -I ./ -I ../../lib/opendnp3-dataflow/cpp/lib/include/
LINK = api.cpp.o -L /usr/local/lib -L /usr/lib/mysql -L ../../sqlbase -L ../../htlib -L /usr/lib/opendnp3 
CFLAGS = -g -O2 -std=c++11 -Dlinux -DHYPERTAC -Wall -Wno-delete-non-virtual-dtor -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable 

PROJ=	main.o \
	dnp3server.o \
	dnp3client.o \
	db_wrapper.o \
	log.o \

.SUFFIXES:
.SUFFIXES: .o .cpp .h .hpp

.cpp.o:
	g++ $(CFLAGS) $(INCLUDE) -c $<

all:	dnp3em

dnp3em: $(PROJ) $(BUILD_NUMBER_FILE)
	g++ $(CFLAGS) $(PROJ) $(LINK) $(LIBS) $(BUILD_NUMBER_LDFLAGS) -o dnp3em

clean:
	rm -f dnp3em $(PROJ)



