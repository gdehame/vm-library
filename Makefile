EXEC=VThread
HMLIB=hm.so
SRCDIR=src
EXDIR=examples
HMDIR=HM
BINDIR=bin
OBJDIR=obj
CFLAGS=-Wall -O2
LIBS=-lpthread
CC=gcc
RM=rm -f
ECHO=@echo #Rajouter -e ici si make se plaint de séparateur manquant, delete tous les .d et recompiler
CFILES=$(wildcard ./$(SRCDIR)/*.c)
BFILES=$(wildcard ./$(BINDIR)/*)
CHMFILES=$(wildcard ./$(HMDIR)/*.c)
OHMFILES=$(CHMFILES:./$(HMDIR)/%.c=$(OBJDIR)/%.o)
DHMFILES=$(CHMFILES:.c=.d)
TESTSFILES=$(wildcard ./$(EXDIR)/*.c)
OFILES=$(CFILES:./$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CLEANFILES=$(OFILES:%=%.clean)
CLEANFILES+=$(OHMFILES:%=%.clean)
CLEANFILES+=$(DFILES:%=%.clean)
CLEANFILES+=$(DHMFILES:%=%.clean)
DFILES=$(CFILES:.c=.d)
DFILES+=done.done
DHMFILES+=done.done
TESTS=$(TESTSFILES:%.c=%.example)
CTESTFILES=$(TESTSFILES:./$(EXDIR)/%.c=%.cleant)
FTR=$(BFILES:./$(BINDIR)/%=%.ftr)

.SECONDARY : $(OHMFILES) $(OFILES)

exec : Generating\ $(EXEC)\ binaries.echo $(EXEC)

heap : Generating\ HM\ binairies.echo $(HMLIB)

all : create exec heap test

test : Generating\ test\ binaries.echo $(TESTS) $(CTESTFILES)

create :
	@mkdir -p ./$(OBJDIR)
	@mkdir -p ./$(BINDIR)

$(EXEC) : $(OFILES:$(OBJDIR)/%.o=%.o)
	$(CC) $(OFILES) -o $@ $(CFLAGS) $(LIBS)
	@mv ./$(EXEC) ./$(BINDIR)/$(EXEC)
	
%.example : %.c
	$(CC) $^ -o $@ $(CFLAGS)
	
$(HMLIB) : $(OHMFILES:$(OBJDIR)/%.o=%.o)
	$(CC) -shared $(OHMFILES) -o $@ $(CFLAGS) $(LIBS)
	@mv ./$(HMLIB) ./$(BINDIR)/$(HMLIB)
	
depend : done.done
	
%.d : %.c
	$(CC) -MM -c $< -o $@ $(CFLAGS) $(LIBS)
	$(ECHO) "\t$(CC) -fPIC -c $< -o $(OBJDIR)/"\$$@" $(CFLAGS) $(LIBS)" >> $@
	
done.done :
	$(ECHO) "\033[31;01mGenerating project depandancies$*\033[00m"
	@touch done.done

.PHONY : create clean mrproper
	
clean : Removing\ files.echo $(CLEANFILES) 
	
%.clean :
	$(RM) $*
	
done.clean :
	$(RM) done.done
	
%.cleant :
	@mv ./examples/$*.example ./$(BINDIR)/$*
	
mrproper : clean $(FTR)
	
	
%.ftr :
	$(RM) $(BINDIR)/$*
	
-include $(DFILES) $(DHMFILES)

%.echo :
	$(ECHO) "\033[31;01m$*\033[00m"
