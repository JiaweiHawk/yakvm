PWD		:= $(shell pwd)
KDIR	?= ${PWD}/../kernel

default:
	make -C ${KDIR} M=${PWD} modules

clean:
	make -C ${KDIR} M=${PWD} clean
