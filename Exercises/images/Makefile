
#
#	Makefile for images project
#



# rule to build an ifs from a build file
%.ifs : %.build
	mkifs -v $< $@

IMAGES = x86_64-vmware.ifs x86_64-vbox.ifs


all:  $(IMAGES)



clean:
	rm -f *.o $(BINS) $(IMAGES) *.sym
	cd solutions; make clean_solutions


 	