CC       	= /root/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc
RULE		= -lpthread -O2 -pipe -Wall -W -mlittle-endian -march=armv5t -mabi=aapcs-linux -mtune=arm9tdmi -D_REENTRANT -D_FILE_OFFSET_BITS=64
RULEAAA		= -lpthread
DIR			= /mnt/hgfs/3062/
APPNAME		= Store3072a-12.0.1
OBJDIR 		= $./release/


	

$(APPNAME):ADSave.c Cmd.c DAGetData.c main.c exist.c VER
	@echo Linking...... '$@'
	@echo "bbbbbb"
	@$(CC)  ADSave.c Cmd.c DAGetData.c main.c exist.c -o $@  $(RULE)
	@echo "aaaaaa"
	chmod +x $@
	@echo -e "+++Compling [$@] successed.\n"
	cp -f $(APPNAME) "$(DIR)Store3072A2.Release"
	mv -f $(APPNAME) "$(DIR)$(APPNAME)-$(shell cat ./dev.ver)_$(shell date +%Y%m%d.%H%M%S).Release"

VER: ver.sh ver.h
	./ver.sh ver.h

clean:
	rm *.o 

