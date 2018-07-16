#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dirent.h>
#include <linux/sem.h>
#include <linux/sched.h>
#include <assert.h>
#include <signal.h>

#include "Cmd.h"
#include "DAGetData.h"
#include "ADSave.h"
#include "ver.h"

//#define APPNAME APP_No.11Pro

char lib_ad_ver_str[100];

const char *libADVersion(){
    char aa[]   = "Store_No1.11Pro";
    int  bb     = API_BUILD;
    memset(lib_ad_ver_str, 0, 100);
    sprintf(lib_ad_ver_str, "%s #%d, %s %s\n", 
        aa, bb, __DATE__, __TIME__);
    return lib_ad_ver_str;
}

int initDevice(){ 	//init, open, caculate oriential num

	if(0 != init_AD_Device()){
		perror("Set_ad_mmap Err");
		return -1;
	}
	if(0 != init_CMD_Device()){
		perror("Set_cmd_mmap Err");
		return -2;
	}
	if(0 != init_DA_Device()){
		perror("Set_da_mmap Err");
		return -4;
	}
	return 0;
}

static int LockFile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return fcntl(fd, F_SETLK, &fl); //成功返回设置的锁值， 如果文件锁被其他进程占用返回-1
}

/* Check app is already run
 *
 * Return:
 *      1 running
 *    0 not runing
 * Remark:
 *
 * */
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
static int AlreadyRun(const char *appName){
    int fd = -1;
    char lockfilename[256] = "/mnt/yaffs/";
    char buf[128];

    strcat(lockfilename, appName); //字符串接龙,自动去掉字符串尾的'\0'
    fd = open(lockfilename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        printf("open:%s\n", lockfilename); //打印警告信息
        exit (1);
    }

    if (LockFile(fd) < 0) {     //程序开始会锁定一个文件，如果锁不住则说明程序已经运行
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return 1;
        }
        exit (1);
    }

    if (ftruncate(fd, 0) == -1) {   //改变文件大小，将文件大小置为0
        printf("ftruncate");
        exit(0);
    }

    if (write(fd, buf, strlen(buf) + 1) == -1) {
        printf("ftruncate");
        exit(0);
    }

    printf("Run Instance file:%s\n", lockfilename);
    return 0;
}


int main(int argc, char *argv[]){
// system("echo 408000 >/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
// system("echo userspace >/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
 
	libADVersion();
	if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0) {
            printf("%s\n", lib_ad_ver_str);
            exit(0);
        }
    }
     if (AlreadyRun("APP_No1.11Pro")) {    //判断进程是否占用
        printf("A instance has running.\n");
        exit(0);
    }

	printf("STORE--------------------------%s\n", lib_ad_ver_str);
	initDevice();
	printf("--------------init OVER\n");
	Get_NEW_FILE_NAME();

	pthread_t thread_CMD;
	void * thrd_ret;

	int cmd_thread = pthread_create(&thread_CMD, NULL, (void *)CMD_process, NULL);
	if(cmd_thread != 0){
		printf("create smd thread failed:%d\n",cmd_thread);
		pthread_exit(NULL);
	}
     pthread_join(thread_CMD,&thrd_ret);


    return 0;
}

