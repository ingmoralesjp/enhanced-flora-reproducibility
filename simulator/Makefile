# INET lives beside this FLoRa project in the OMNeT++ samples directory.
# Keep this path consistent with src/Makefile and the command-line runners.
INET_DIR = ../inet

all: checkmakefiles
	@cd src && $(MAKE)

clean: checkmakefiles
	@cd src && $(MAKE) clean

cleanall: checkmakefiles
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
	@rm -f src/Makefile

makefiles:
	@cd src && opp_makemake --make-so -o flora -O out -f --deep -KINET_PROJ=../$(INET_DIR) -DINET_IMPORT -I. -I$$\(INET_PROJ\)/src -L$$\(INET_PROJ\)/src -lINET$$\(D\)

check-config:
	@../../bin/opp_run -n src:simulations:../inet/src -l src/flora -l ../inet/src/INET \
		-q numruns -c LoRaWanReferenceStaticS1 simulations/examples/three-adr.ini

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
