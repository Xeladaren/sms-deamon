export EXENAME = sms-daemon


export SRCDIR = src
export BINDIR = bin
export LIBDIR = header
export OBJDIR = obj

export CC = gcc
export LC = gcc

# options de compilation
export CCFLAGS += -I$(LIBDIR)/
export CCFLAGS += -Wall
export CCFLAGS += -std=c11
export CCFLAGS += -pthread

export LDFLAGS += -I$(LIBDIR)/
export LDFLAGS += -pthread

export PROG = $(BINDIR)/$(EXENAME)

SRC=$(wildcard $(SRCDIR)/*.c)
OBJ=$(SRC:$(SRCDIR)%.c=$(OBJDIR)%.o)

all : $(PROG)

$(PROG) : $(OBJ)
	@mkdir -p $(BINDIR)
	$(LC) $(OBJ) $(LDFLAGS) -o $@

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@mkdir -p $(LIBDIR)
	$(CC) $(CCFLAGS) -c $< -o $@

test :
	echo $(OBJ)

touch :
	touch $(SRCDIR)/*.c
	touch $(LIBDIR)/*.hpp

clear :
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/*

run :
	time ./$(PROG)

install_lib :

create :
	mkdir -p $(SRCDIR)
	mkdir -p $(BINDIR)
	mkdir -p $(LIBDIR)
	mkdir -p $(OBJDIR)
	touch $(SRCDIR)/main.c

