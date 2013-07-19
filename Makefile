# Makefile for src/mod/gseen.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.$(MOD_EXT) *~

static: ../gseen.o

modules: ../../../gseen.$(MOD_EXT)

../gseen.o: ../module.h ../modvals.h ../../eggdrop.h datahandling.c \
 gseen.c sensors.c gseencmds.c gseencmds.c do_seen.c ai.c tclcmds.c \
 misc.c seentree.c generic_binary_tree.c slang_gseen_commands.c \
 slang.c slang_text.c slang_ids.c slang_chanlang.c seenlang.h \
 slang_multitext.c gseen.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c gseen.c
	rm -f ../gseen.o
	mv gseen.o ../

../../../gseen.$(MOD_EXT): ../gseen.o
	$(LD) -o ../../../gseen.$(MOD_EXT) ../gseen.o $(XLIBS)

#safety hash
