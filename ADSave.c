#define _GNU_SOURCE
#include "DAGetData.h"
#include "Cmd.h"
#include "ADSave.h"
#include "exist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/mman.h>
#include <stddef.h>
#include <dirent.h>
#include <signal.h>
#include <linux/sem.h>
#include <linux/sched.h>

static int AdHandle=0;
static int pag_size, sum_cnt;
static unsigned int * buf[2 * 1024];
static char * buf_sd[320];
static int mmap_enable = 0;
static char Data_Stop=0;
static int package_head_size = 0;
static int package_tail_size = 0;
static int buf_cnt;
static char SD_Format=0;
static int fp = -1;
static char file_name[100]={0};
static char file_name_now[100]={0}, file_name_old[100]={0};
static int file_times_now=0, file_times_old=0;

static char save_file_flag=0;
extern char Readfile_num[100];
char cur_file_name[100];

pthread_mutex_t Save_lock;
pthread_cond_t	Save_notEmpty;
pthread_cond_t	Save_notFull;
pthread_t 		Save_thread;
static int 	save_ret;
static unsigned int Save_readPos, Save_writePos;
static int AD_Start_Flag=0;
int cmd_to_data_times=0;



void buff_init(void){
	pthread_mutex_init(&Save_lock, NULL);
	pthread_cond_init(&Save_notEmpty, NULL);
	pthread_cond_init(&Save_notFull, NULL);
	Save_readPos = 0;
	Save_writePos = 0;
	buf_cnt=0;
	AD_Start_Flag=1;

}

void buff_reset(void){
	Save_readPos = 0;
	Save_writePos = 0;
}

void buff_destroy(void){
	pthread_mutex_destroy(&Save_lock);
	pthread_cond_destroy(&Save_notEmpty);
	pthread_cond_destroy(&Save_notFull);
	Save_readPos = 0;
	Save_writePos = 0;
	buf_cnt=0;
	//ioctl(AdHandle, MCASP_STOP, NULL);
	//AD_Start_Flag=0;
}


unsigned short Set_mmap(int AdHandle, int CurChan,int uAiFrameSize) 
{
	int i, pag_cnt;
	pag_size = (uAiFrameSize * CurChan + package_head_size+ package_tail_size) * 4;
	pag_size = (pag_size / PAGE_SIZE + 1) * PAGE_SIZE; //every package points NUM
	pag_cnt = sec_size / pag_size; //every package points NUM
	sum_cnt = pag_cnt * 32; //
//	MSGD("pag_size %d,sec_size %d,pag_cnt %d\n",pag_size,sec_size,pag_cnt);

	for (i = 0; i < sum_cnt; i++) {
		buf[i] = mmap(NULL, pag_size, PROT_READ | PROT_WRITE, MAP_SHARED/*|MAP_ANONYMOUS*/,
				AdHandle, pag_size * i);
		if(buf[i] == MAP_FAILED){
			printf("da mmap error\n");
			return 1;
		}
		//MSGD("---mmap addr is %d %x\n",i, buf[i]);
	}
	mmap_enable = 1;
	return 0;
}

ssize_t Write_N(int ffd, void *buf, size_t n){
	ssize_t nwrite=0;
	size_t nleft=n;
	char*bp=(char*)buf;
	while(nleft>0){
		nwrite=write(ffd, bp, nleft);
		if(nwrite>0){
			nleft 	-= nwrite;
			bp		+= nwrite;
		}else if(nwrite==0){
			printf("___________________Read over,n=%d, nleft=%d, %s, %d\n",n, nleft, __func__, __LINE__);
			break;
		}else{
			printf("___________________Read over,n=%d, nleft=%d, nwrite=%d, %s, %d\n",n, nleft, nwrite, __func__, __LINE__);
			if(errno==EINTR){
				nwrite=0;
			}else{
				return -1;
			}
		}
	}
	return (n-nleft);
}

int AD_NAME_RESET(void){

	file_times_now = 0;
	file_times_old = 0;
	memset(file_name, 0, 100);
	sprintf(file_name, "/mnt/udisk/Sample#0.dat");
	memcpy(file_name_now, file_name, 100);
	memcpy(file_name_old, file_name, 100);
	return 0;
}

int Get_NEW_FILE_NAME(void){
	unsigned int i;
	int j;
	//printf("aho+++++++++++++++\n");
	for(i=0; ;i++){
		memset(file_name, 0, 100);
		sprintf(file_name, "/mnt/udisk/Sample#%d.dat", i);
		j =is_file_exist(file_name);
		if(0 == j){
			file_times_old = i;
			memcpy(file_name_old, file_name, 100);
			printf("file_name_exist: %s\n", file_name);
		}else if(-1 == j){
			file_times_now = i;
			memcpy(file_name_now, file_name, 100);
			printf("get a new file name=%s\n", file_name_now);
			break;			
		}else{
			printf("Warnning, file path is not exist\n");
			break;
		}
	}
	return 0;
}

int Get_ALL_FILE_NUM(void){
	unsigned int i;
	int j;
	char all_file[100];
	//printf("aho+++++++++++++++\n");
	for(i=0; ;i++){
		memset(all_file, 0, 100);
		sprintf(all_file, "/mnt/udisk/Sample#%d.dat", i);
		j =is_file_exist(all_file);
		if(0 == j){
			continue;
			//file_times_old = i;
			//memcpy(file_name_old, file_name, 100);
			//printf("file_is_exist: %s\n", all_file);
		}else if(-1 == j){
			//file_times_now = i;
			//memcpy(file_name_now, file_name, 100);
			if(i==0){
				printf("there is no file in card\n");
				return 0;  //没有文件存在
			}else{
				printf("all file num is =%d, %s\n", i, all_file);
				return i;  //文件存在的个数
			}
			//break;
		}else{
			printf("Warnning, file path is not exist\n");
			break;
		}
	}
	return 0;
}

int DA_NUM(int num){
	memset(Readfile_num, 0, 100);
	sprintf(Readfile_num, "/mnt/udisk/Sample#%d.dat", num-1);
	int j =is_file_exist(Readfile_num);
	if(0 == j){
		printf("Readfile_NUM+++++++++= %s \n", Readfile_num);
		memcpy(cur_file_name, Readfile_num, 100);
		return 0;
	}else{
		printf("Readfile_num not exist， %s\n", Readfile_num);
		return -1;
	}
	
}

//获得所有文件的名字列表,为了读取全部函数，后来不用了
int Get_FILE_NAME_LIST(char ** name){
	int i=0;
	char file_name_list[512][35];
	//file_name_list = name;
	memset(name, 0, 512*35);
	for(i=0; i<512; i++){
		memset(file_name, 0, 100);
		memset(file_name_list[i], 0, 35);
		sprintf(file_name, "/mnt/udisk/Sample#%d.dat", i);
		if(0 == is_file_exist(file_name)){
			memcpy(file_name_list[i], file_name, 35);
			printf("file_name_exist[%d]: %s\n", i, file_name);
			
		}else{
			if(i==0){
				printf("there is no file in the card.\n");
				return -1;
			}
			else{
				printf("ALL file_NUM= %d\n", i);
			}
			break;
		}
	}
	memcpy(name, file_name_list, i*35);
	return i;
}

//获取最新的存储的文件名
int Get_NOW_FILE(char * file){
	int i=0;
	for(i=0; i<10000; i++){
		memset(file_name, 0, 100);
		sprintf(file_name, "/mnt/udisk/Sample#%d.dat", i);
		if(0 == is_file_exist(file_name)){
			memset(file, 0, 100);
			memcpy(file, file_name, 100);
			continue;
			//memcpy(file, file_name, 100);
			//return 0;
			
		}else{
			if(i==0){
				printf("there is no file in the card.\n");
				return -1;
			}else{
				//memcpy(file, file_name, 100);
				printf("Get DA Read file OK, name is %s\n", file);
				return 0;
			}
		}
	}
	return -1;
}

int init_AD_Device(){ 	//init, open, caculate oriential num
	unsigned int i;
	for(i=0; ;i++){
		memset(file_name, 0, 100);
		sprintf(file_name, "/mnt/udisk/Sample#%d.dat", i);
		if(-1 == is_file_exist(file_name)){
			file_times_now = i;
			memcpy(file_name_now, file_name, 100);
			break;
		}else{
			file_times_old = i;
			memcpy(file_name_old, file_name, 100);
		}
	}
	AdHandle = open("/dev/OMAPL138-MCASP", O_RDWR);
	if (AdHandle <= 0){
		perror("open OMAPL138-MCASP err");
		return -2;
	}

	for(i=0;i<320;i++){ //内存对齐，整存整取，速度快
		int ntmp = posix_memalign((void**)&buf_sd[i], getpagesize(), LEN);
		if(ntmp!=0){
			printf("error\n");
			return -16;
		}
	}

	printf("sum_cnt1:%d\n", sum_cnt);
	ioctl(AdHandle, MCASP_STOP, NULL);
	AD_Start_Flag=0;
	ioctl(AdHandle, MCASP_EDMA_BUFSIZE, LEN);
	if(0 != Set_mmap(AdHandle, 1, LEN/4)){
		perror("Set_mmap Err");
		return -32;
	}
	printf("sum_cnt:%d\n", sum_cnt);
	buff_init();
	cmd_to_data_times = 0;

	return 0;
}

int	AD_START(void){
	Data_Stop = 0;
	cmd_to_data_times=0;
	buff_reset();
	ioctl(AdHandle, MCASP_STOP, NULL);
	ioctl(AdHandle, MCASP_EDMA_BUFSIZE, LEN);
	ioctl(AdHandle, MCASP_START, NULL);
	AD_Start_Flag=1;
	return 0;
}

int SD_FORMAT_CMD(void){
	SD_Format=1;
	ioctl(AdHandle, MCASP_STOP, NULL);
	AD_Start_Flag=0;

	return 0;
}

int AD_STOP(void){
	Data_Stop = 1;
	//ioctl(AdHandle, MCASP_STOP, NULL);
	//close(fp);
	return 0;
}

void *SaveThread(void){ //消费者
	while(1){
		pthread_mutex_lock(&Save_lock);
		if (Save_readPos <= Save_writePos){
			//printf("write data empty\n");
			pthread_cond_wait(&Save_notEmpty, &Save_lock);
		}
		pthread_mutex_unlock(&Save_lock);

		//AD_STORE_FILE();
		if(LEN==Write_N(fp, buf_sd[Save_writePos%320], LEN)){
		
			pthread_mutex_lock(&Save_lock);
			Save_writePos++;
			pthread_cond_signal(&Save_notFull);
			pthread_mutex_unlock(&Save_lock);
		}else{
			//lseek(fp, Save_writePos*LEN, SEEK_SET);
			printf("write file err once$$$$$$$$$$$$$$$$$$$$^^^^^^^^^^^^^^^--------\n");
			break;
		}

		if(Data_Stop==1){
			//pthread_exit(NULL);
			return NULL; 			//收到停止命令后，立即停止，是否合理
		}
	}
	//如果存储空间不足，会进入下面
	CLOSE_AD_FILE();
	AD_STOP();
	return NULL;
}

void CLOSE_AD_FILE(){
	if(fp>=0)
		close(fp);
}

int Get_AD_State(void){
	return AD_Start_Flag;
}

fd_set rfds;
int SendDatafloat(int AdHandle, int timeouts){
	/************************************************
	    1、确定取数的超时时间timeout 生产者
	************************************************/	
	struct timeval timeout1 = { timeouts, 0 };
	struct timeval *timeout = &timeout1;
	int ret,value;

	FD_ZERO(&rfds);
	FD_SET(AdHandle, &rfds);
	ret = select(AdHandle + 1, &rfds, NULL, NULL, timeout);
	// usleep(10000);
	// ret = 1;

	/****************************************************
	    3、取数结束后，有三种情况--出错、超时和得到了数据
	****************************************************/
	if (ret < 0){
		printf("----------------AD get Err\n");
		return -1;/**取数出错，失败了*/
	}
	else if (ret == 0){
		ioctl(AdHandle, INQUIRE_INTERRUPT_FLAG, &value);
		printf("++++++++++++++++AD get timeout, interrupu=%d\n", value);
		if(Send_to_PC.get_cmd==CMD_STARTAD){
			cmd_to_data_times++;
			if(cmd_to_data_times==2 && value==1){
				STATE_MUTEX(STATE_DRIVERERR);
			}
		}
		if(Save_writePos>0 && Save_readPos>0 && Save_writePos==Save_readPos){
			AD_STOP();
		}

		//AD_STATE_FREE();
		//AD_START();
		return -1;	/**取数超时了*/
	}
	else{
		if(	(Send_to_PC.state != STATE_STOREDATA) &&
		  	(Send_to_PC.get_cmd == CMD_STARTAD)
		){
			STATE_MUTEX(STATE_STOREDATA);
		}
		pthread_mutex_lock(&Save_lock);
		if(Save_readPos-Save_writePos >= 319){ 		//full
			printf("Get data, FIFO full, Save_readPos=%d, Save_writePos=%d\n",Save_readPos, Save_writePos);
			pthread_cond_wait(&Save_notFull, &Save_lock);
		}
		pthread_mutex_unlock(&Save_lock);

		memcpy(buf_sd[Save_readPos%320], buf[buf_cnt], LEN);

		if(Save_readPos%10==0){
			 printf("GET DATA[%d]: %#X,%#X,%#X,%#X,%#X,%#X,%#X,%#X\n", 
			 	Save_readPos,
			 	buf[buf_cnt][0], buf[buf_cnt][1], buf[buf_cnt][2], buf[buf_cnt][3],
			 	buf[buf_cnt][4], buf[buf_cnt][5], buf[buf_cnt][6], buf[buf_cnt][7]
			 );
		}

		pthread_mutex_lock(&Save_lock);
		Save_readPos++;
		pthread_cond_signal(&Save_notEmpty);
		pthread_mutex_unlock(&Save_lock);

		buf_cnt = (buf_cnt + 1) % sum_cnt;
		
	}

	return 0;
}

int *ad_process_data(void){

START:
	AD_START();
	while(1)
	{
		AD_Start_Flag=1;
		SendDatafloat(AdHandle, 3);
		if(Save_readPos>=1 && Save_writePos==0){
			if(save_file_flag==0){
				save_file_flag=1;
				Get_NEW_FILE_NAME();
				fp = open(file_name_now, O_RDWR | O_TRUNC | O_CREAT | O_DIRECT);
				printf("open file %s, fp=%d\n", file_name_now, fp);
				if (fp < 0){
					perror("open sd err");
					return 0;
				}
				memcpy(cur_file_name, file_name_now, 100);
				AD_STORE_FILE();
				save_ret = pthread_create(&Save_thread,NULL,(void *)SaveThread,NULL);
				if(save_ret != 0){
					printf("create smd thread failed:%d\n",save_ret);
					close(fp);
					pthread_exit(NULL);
				}
				printf("create a file to store, %s\n", "haha");
			}
		}

		if(Data_Stop==1){
			printf("exit AD thread. For SD_Format Prepare\n");
			//buff_destroy();
			AD_STOP();
			close(fp);
			save_file_flag = 0;
			buf_cnt=0;
			AD_Start_Flag=0;
			// return 0;
			STATE_MUTEX(STATE_WAITTING);
			break;
		}
		
	}
	sleep(1);
	goto START;
	AD_Start_Flag=0;
	//usleep(1000000);
	//DA_READ_FILE_OVER();
	return 0;

}


 