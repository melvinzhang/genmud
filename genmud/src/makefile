OBJS =  aff.o alias.o areagen.o areagen2.o auction.o bank.o battle.o \
	boot.o \
	chan.o clan.o consid.o const.o color.o  combatai.o create.o \
	dam.o detailgen.o edit.o event.o fight.o fileio.o follow.o \
	guild.o help.o historygen.o implant.o info.o \
	int.o kde.o look.o magic.o magicitem.o map.o mapgen.o \
	mobgen.o money.o move.o myrand.o note.o objectgen.o \
	parse.o pcio.o pkdata.o prac.o proc.o provisiongen.o \
	questgen.o questgenold.o quit.o race.o roomdescgen.o rumor.o \
	script.o scriptio.o scriptedit.o scriptline.o security.o serv.o \
	shop.o socials.o socibuild.o sociedit.o society.o societyio.o \
	socifight.o socirts.o socigen.o sociname.o socineed.o sociraid.o \
	spell.o stredit.o stringparse.o thing.o thingio.o track.o \
	upd.o wear.o weather.o worldgen.o \
#	wilderness.o wildalife.o \
#	THE PRECEEDING LINE IS NOT ADDED IN UNLESS YOU REMOVE THE #
#	ONLY USE THE PRECEEDING LINE IF YOU WANT THE WILDERNESS MAP!!!

.c.o: serv.h
	gcc -c -g -Wall -pedantic $<

mud: $(OBJS)
	rm -f mud
	gcc -lpthread -lcrypt -o mud $(OBJS)

clean:
	\rm -rf *.o mud
