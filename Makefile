CC=g++
HIKSDKPATH=../EN-HCNetSDKV6.1.6.3_build20200925_Linux64

hikutil:
	$(CC) -L$(HIKSDKPATH)/lib -I$(HIKSDKPATH)/incEn/ -lhcnetsdk hikutil.cpp -o hikutil

run: hikutil
	LD_LIBRARY_PATH=$(HIKSDKPATH)/lib ./hikutil

clean:
ifneq (,$(wildcard ./hikutil))
	rm hikutil
endif

ifneq (,$(wildcard ./sdkLog))
	rm -rf ./sdkLog
endif
