#!/bin/sh
#
#  change file version
#    Autor winxxp
#    2010-01-29
#  Find DASPNET_BUILD in file "filename",
#  and change "x" to "x+1"
#  update 2011-05-25 winxxp add version and build
#  update 2011-05-26 winxxp add more version control
#---------------------------------------------------
help() {
	printf 'Usage:\n'
	printf '\t%s [help] \t\tfor help\n' $0
	printf '\t%s [Ver.h file]\tchange version\n' $0
	printf '\t%s [show] [Ver.h]\tshow version\n' $0
}

FormatVersion() {
	ver_main=$(grep API_VER_YEAR $1 | awk '{print $3}')
	ver_comm=$(grep API_VER_MONTH $1 | awk '{print $3}')
	ver_ad=$(grep API_VER_DAY $1 | awk '{print $3}')
	ver_func=$(grep API_VER_TIME $1 | awk '{print $3}')
	ver_build=$(grep API_BUILD $1 | awk '{print $3}')
    
	printf "build version:%s-%s-%s:%s(%s)\n" $ver_main $ver_comm $ver_ad $ver_func $ver_build
}

BuildVersionFile() {
	printf "build version:%s(%s-%s-%s_%s)\n" $6 $2 $3 $4 $5 

	echo "#define API_VER_YEAR $2.$5" > $1
	#echo "#define API_BUILD $3" >> $1
	echo "#define API_VER_MONTH $3" >> $1
	echo "#define API_VER_DAY $4" >> $1
	echo "#define API_VER_TIME $5" >> $1
	echo "#define API_BUILD $6" >> $1
}

if [ "$1" = "help" ];then
	help
	exit 0
fi

if [ -z $1 ];then
	echo please input resource.h file
	help
	exit 0
fi

if [ "$1" = "show" ];then
	if [ -z $2 ];then
		echo Error Usange
		help
		exit 0
	fi

	if [ ! -e $2 ]; then
		echo Not find $2
		exit 0
	fi
	
	FormatVersion $2
	exit 0
fi

if [ ! -e $1 ]; then
	BuildVersionFile $1 0 0 0 0 0
fi

ver_main=$(date +%Y%m%d)
ver_comm=$(date +%m)
ver_ad=$(date +%d)
ver_func=$(date +%H%M%S)
ver_build=$(grep API_BUILD $1 | awk '{print $3 + 1}')
FILE_MAKE=Makefile
BUILD_NAME="R$(grep API_BUILD $1 | awk '{print $3}'))"

BuildVersionFile $1 $ver_main $ver_comm $ver_ad $ver_func $ver_build

echo "$ver_build" > ./dev.ver
#sed 24c 'mv ./libadjcy.so /mnt/hgfs/3062/libadjcy.so.1.0.0-$(ver_build)'
#sed -i "24c mv libadjcy.so /mnt/hgfs/3062/libadjcy.so.1.0.0.$(ver_build)" $FILE_MAKE
#sed -i "s/^mv*$/mv libadjcy.so /mnt/hgfs/3062/libadjcy.1.0.0/g" $FILE_MAKE
#BuildVersionFile $1 $ver_main $ver_build
