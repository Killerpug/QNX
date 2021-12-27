
#
#	Makefile for images project
#

# rule to build an ifs from a build file
%.ifs : %.build
	mkifs -v $< $@

IMAGES = x86_64-vmware_sol.ifs x86_64-vbox_sol.ifs

all:	$(BINS) $(IMAGES)

$(IMAGES): $(BINS)

clean_solutions:
	rm -f *.o $(BINS) $(IMAGES) *.sym

