#by yt

TARGS+=	./sample/sample_wr \
		./sample/sample_time \
		./sample/sample_sig \

SRCS+=	./event.o \
		./eventbase.o \
		./ioepoll.o \
		./esignal.o \

INC = -I .

all : $(TARGS)

./sample/% : $(SRCS) ./sample/%.o
	g++ -g -Wall -o $(@) $(^)  
	@echo 'Building target: $(@)' 	
	
./sample/%.o : ./sample/%.cpp 
	g++ -o $(@) -c  $(^) $(INC)

%.o : %.cpp 
	g++ -o $(@) -c  $(^) $(INC)
			
				
.PHONY : clean

clean :
	rm -rf ./*.o ./sample/*.o

