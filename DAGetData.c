#include <assert.h>

#include "DAGetData.h"
#include "Cmd.h"
#include "ADSave.h"
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
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/mman.h>
#include <stddef.h>
#include <dirent.h>
#include <signal.h>

//#define LEN_DA  				393216 //4096*32*3 					  old
#define LEN_DA  				393216//	491520 	//4096*40*3, 	send 		,393216
#define LEN_DA_PACKAGE			262144//	327680  //=LEN_DA*2/3, 	Read 		,262144
#define LEN_DA_POINTS  			32768 //	40960 	//4096*40*3 /3 /4, points/ch 	,32768

static int DaHandle=0;
static int framePoint;  
static int clkdiv = 1;
static int value = 0x00;
static unsigned int * da_buf[512];
static int da_sum_cnt, da_pag_size;
static int da_buf_cnt;
static int ad_da_sync_da = 0,da_mmap_enable = 0;
static unsigned int Card_readPos=0, Card_writePos=0;
static int Data_Get_Flag=0;
static unsigned long long filesize=0;
static struct stat statbuff;
static int file_NUM;
pthread_mutex_t Card_lock;
pthread_cond_t	Card_notEmpty;
pthread_cond_t	Card_notFull;
pthread_t 		thread_ReadSD_Num, thread_ReadAD_ALL;
static char file_name_list[512][35];
static char file_to_read[100];
char Readfile_num[100];
static unsigned int sd_read_buffer[sd_read_buffer_cnt][LEN_DA/4];
//static int * start_da;
FILE * fr = NULL;
int fd=-1;
int SleepTime=0;

void DA_buff_init(void){

	pthread_mutex_init(&Card_lock, NULL);
	Card_readPos = 0;
	Card_writePos = 0;
	da_buf_cnt = 0;
	CLOSE_DA_FILE();
}

void DA_buff_reset(void){
	Card_readPos = 0;
	Card_writePos = 0;
	da_buf_cnt = 0;
}

void DA_buff_destroy(void){

	pthread_mutex_destroy(&Card_lock);

}

ssize_t Read_N(int ffd, void *buf, size_t n){
	ssize_t nread=0;
	size_t nleft=n;
	char*bp=(char*)buf;
	while(nleft>0){
		nread=read(ffd, bp, nleft);
		if(nread>0){
			nleft 	-= nread;
			bp		+= nread;
		}else if(nread==0){
			printf("Read over,n=%d, nleft=%d, %s, %d\n",n, nleft, __func__, __LINE__);
			break;
		}else{
			printf("_______________________Read over,n=%d, nleft=%d, %s, %d\n",n, nleft, __func__, __LINE__);
			if(errno==EINTR){
				nread=0;
			}else{
				return -1;
			}
		}
	}
	return (n-nleft);
}

int DA_SLEEP_COM(){ //32秒一次300kB，平均 10kB/s, 波特率115200bps
	SleepTime=32;
	return 0;
}

int DA_SLEEP_LAN(){
	SleepTime=0;
	return 0;
}

unsigned short Set_da_mmap(int DaHandle,int p_pag_size){
	int i,pag_cnt;
	da_pag_size = (p_pag_size/PAGE_SIZE+1)*PAGE_SIZE;
	//printf("da_pag_size:%d\n",da_pag_size);
	if(ad_da_sync_da == 1){
		pag_cnt = sec_size / da_pag_size;
	}else{
		pag_cnt = 6;//lxj_modify_20150730
	}
	da_sum_cnt = pag_cnt *4;

	//MSGD("______pag_size %d,sec_size %d,pag_cnt %d\n",da_pag_size,sec_size,pag_cnt);

	for(i=0; i<da_sum_cnt; i++){
		da_buf[i] = mmap(NULL, da_pag_size, PROT_READ | PROT_WRITE, MAP_SHARED, DaHandle, da_pag_size * i);
		if(da_buf[i] == MAP_FAILED){
			printf("da mmap error\n");
			return 1;
		}
		//printf("---da mmap addr is %d %x\n",i, buf[i]);
	}
	da_mmap_enable = 1;
	return 0;
}

void CLOSE_DA_FILE(){
	if(fd>=0)
		close(fd);
}

int DA_STATE(void){
	return Data_Get_Flag;
}

int DA_START(void){
	DA_buff_reset();
	Data_Get_Flag = 0x3;
	return 0x3;
}

int DA_STOP(void){
	printf("DA_STOP, LINE=%d\n", __LINE__);
	Data_Get_Flag = 0xF;
	da_buf_cnt = 0;
	return 0xF;
}

int init_DA_Device(){ 	//init, open, caculate oriential num

	DaHandle=open("/dev/OMAPL138-MCASP-DA",O_RDWR);
	if (DaHandle <= 0){
		perror("open DaHandle Err");
		return -8;
	}

	ioctl(DaHandle, MCASP_DA_STOP,NULL);
	ioctl(DaHandle, SET_DAAD_SYNC, &value); 
	framePoint = LEN_DA;
	ioctl(DaHandle, MCASP_DA_EDMA_BUFSIZE, &framePoint); 
	ioctl(DaHandle, MCASP_DA_SAMPLE_RATE, &clkdiv);
	if(0 != Set_da_mmap(DaHandle, framePoint)){
		perror("Set_da_mmap Err");
		return -64;
	}
	DA_buff_init();
	return 0;
}

unsigned long long File_Size(char * name){
	filesize = 0;
	stat(name, &statbuff);
	filesize = statbuff.st_size;
	printf("--++filesize=%llu Bytes\n", filesize);

	return filesize;
}

int *sd_read_once(void){ //生产者
	//FILE * fr = NULL;
	unsigned int i=0;
	unsigned int *buf_first=malloc(LEN_DA_PACKAGE);  //LEN_DA_PACKAGE
	int num; //, package_size, package_offset;
	memset(buf_first, 0, LEN_DA_PACKAGE);
	sleep(3);

	fd=open(file_to_read, O_RDWR);
	if(fd<0){
		perror("open file Once Err-------------------\n");
		printf("---------------++++++$$$$########, %d\n", fd);
		free(buf_first);
		buf_first=NULL;
		return NULL;
	}

	if(File_Size(file_to_read) <= 0){
		printf("---------------++++++$$$$########,the file size is %llu\n", filesize);
		free(buf_first);
		close(fd);
		fd=-1;
		buf_first=NULL;
		return NULL;
	}
	while(filesize>=LEN_DA_PACKAGE){

		pthread_mutex_lock(&Card_lock);
		if((Card_readPos-Card_writePos)>10){ //full
			//printf("Read too fast, waitting.....................%s\n", "hahah");
			pthread_mutex_unlock(&Card_lock);
			usleep(1000000);
		}else{
			num = Card_readPos%sd_read_buffer_cnt;
			pthread_mutex_unlock(&Card_lock);

			if(LEN_DA_PACKAGE==Read_N(fd, buf_first, LEN_DA_PACKAGE)){
				for(i=0;i<LEN_DA_POINTS;i++){ 
					sd_read_buffer[num][3*i+0] = buf_first[2*i+0];
					sd_read_buffer[num][3*i+1] = buf_first[2*i+1];
					sd_read_buffer[num][3*i+2] = 0xffffffff;
				}
				pthread_mutex_lock(&Card_lock);
				Card_readPos++;
				pthread_mutex_unlock(&Card_lock);
				filesize = filesize-LEN_DA_PACKAGE;
				system("date");

			}else{
				// printf("-----ERRERRERR---##################package_offset=%d,package_size=%d, Card_readPos=%d,i=%d\n",
				// 		package_offset,
				// 		package_size,
				// 		Card_readPos,
				// 		i);
				printf("there must be something wrong here\n");
				close(fd);
				fd=-1;
				DA_STOP();
				if(buf_first!=NULL){
					free(buf_first);
					buf_first=NULL;
				}
				return NULL;
			}
		}
		//if(SleepTime>1){
			int xnum=32;
			while(xnum--){
				sleep(1);
				if(Data_Get_Flag==0xF){ //DA stop
					close(fd);
					fd=-1;
					if(buf_first!=NULL){
						free(buf_first);
						buf_first=NULL;
					}
					return NULL;
				}
			}
		// }else{
		// 	sleep(SleepTime);
		// 	if(Data_Get_Flag==0xF){ //DA stop
		// 		close(fd);
		// 		fd=-1;
		// 		if(buf_first!=NULL){
		// 			free(buf_first);
		// 			buf_first=NULL;
		// 		}
		// 		return NULL;
		// 	}
		// }
		// if(Data_Get_Flag==0xF){ //DA stop
		// 	close(fd);
		// 	fd=-1;
		// 	if(buf_first!=NULL){
		// 		free(buf_first);
		// 		buf_first=NULL;
		// 	}
		// 	return NULL;
		// }
		// sleep(SleepTime);
		//usleep(1000000);
	}
	printf("Read file OVER, Card_readPos=%d\n", Card_readPos);
	close(fd);
	fd=-1;
	i=10;
	while(1){
		if(Card_readPos==Card_writePos){
			break;
		}
		usleep(200000);
	}
	DA_STOP();
	if(buf_first!=NULL){
		free(buf_first);
		buf_first=NULL;
	}
	return NULL;
}

int *sd_read_num(void){ //生产者
	unsigned int i=0;
	unsigned int *buf_first=malloc(LEN_DA_PACKAGE);  //LEN_DA_PACKAGE
	int num; //, package_size, package_offset;
	memset(buf_first, 0, LEN_DA_PACKAGE);
	sleep(0);

	fd=open(Readfile_num, O_RDWR);
	if(fd<=0){
		perror("open file Once Err-------------------\n");
		printf("---------------++++++$$$$########, %d, %s, %d\n", fd, __FILE__, __LINE__);
		free(buf_first);
		buf_first=NULL;
		DA_STOP();
		return NULL;
	}

	if(File_Size(Readfile_num) <= 0){
		printf("---------------++++++$$$$########,the file size is %llu\n", filesize);
		free(buf_first);
		close(fd);
		fd=-1;
		buf_first=NULL;
		DA_STOP();
		return NULL;
	}
	while(filesize>=LEN_DA_PACKAGE){

		pthread_mutex_lock(&Card_lock);
		if((Card_readPos-Card_writePos)>10){ //full
			//printf("Read too fast, waitting.....................%s\n", "hahah");
			pthread_mutex_unlock(&Card_lock);
			usleep(100000);
		}else{
			num = Card_readPos%sd_read_buffer_cnt;
			pthread_mutex_unlock(&Card_lock);

			if(LEN_DA_PACKAGE==Read_N(fd, buf_first, LEN_DA_PACKAGE)){
				for(i=0;i<LEN_DA_POINTS;i++){ 
					sd_read_buffer[num][3*i+0] = buf_first[2*i+0];
					sd_read_buffer[num][3*i+1] = buf_first[2*i+1];
					sd_read_buffer[num][3*i+2] = 0xffffffff;
				}
				pthread_mutex_lock(&Card_lock);
				Card_readPos++;
				pthread_mutex_unlock(&Card_lock);
				filesize = filesize-LEN_DA_PACKAGE;

			}else{
				// printf("-----ERRERRERR---##################package_offset=%d,package_size=%d, Card_readPos=%d,i=%d\n",
				// 		package_offset,
				// 		package_size,
				// 		Card_readPos,
				// 		i);
				printf("there must be something wrong here\n");
				close(fd);
				fd=-1;
				DA_STOP();
				if(buf_first!=NULL){
					free(buf_first);
					buf_first=NULL;
				}
				return NULL;
			}
		}
		if(Data_Get_Flag==0xF){ //DA stop
			close(fd);
			fd=-1;
			if(buf_first!=NULL){
				free(buf_first);
				buf_first=NULL;
			}
			return NULL;
		}

		sleep(0);

	}
	printf("Read file OVER, Card_readPos=%d\n", Card_readPos);
	close(fd);
	fd=-1;
	i=10;
	while(1){
		if(Card_readPos==Card_writePos){
			break;
		}
		sleep(3);
	}
	DA_STOP();
	if(buf_first!=NULL){
		free(buf_first);
		buf_first=NULL;
	}
	return NULL;
}

int *sd_read_all(void){ //生产者
	int i,j;
	int num; //, package_size, package_offset;
	// unsigned int buf_all[LEN_DA/4*2/3];
	unsigned int *buf_all=malloc(LEN_DA_PACKAGE);
	memset(buf_all, 0, LEN_DA_PACKAGE);
	sleep(1);
	for(i=0; i<file_NUM; i++){
		filesize = 0;
		j=0;
		fd=open(file_name_list[i], O_RDWR);
		if(fd<0){
			perror("open file all Err-------------------\n");
			printf("---------------++++++$$$$########, %d\n", fd);
			free(buf_all);
			buf_all=NULL;
			return NULL;
		}

		if(File_Size(file_name_list[i]) <= 0){
			printf("the file is NULL, name=%s\n", file_name_list[i]);
			close(fd);
			fd=-1;
			continue;
		}
		while(filesize >= LEN_DA_PACKAGE){ //(Card_readPos==(filesize/(LEN_DA*2/3))
			pthread_mutex_lock(&Card_lock);
			if(Card_readPos - Card_writePos > 10 ){
				pthread_mutex_unlock(&Card_lock);
				//printf("Read too fast, waitting.....................%s\n", "hahah");
				usleep(100000);
			}else{
			
				num = Card_readPos%sd_read_buffer_cnt;
				pthread_mutex_unlock(&Card_lock);
				if(LEN_DA_PACKAGE==Read_N(fd, buf_all, LEN_DA_PACKAGE)){
					for(j=0;j<LEN_DA_POINTS;j++){ 
						sd_read_buffer[num][3*j+0] = buf_all[2*j+0];
						sd_read_buffer[num][3*j+1] = buf_all[2*j+1];
						sd_read_buffer[num][3*j+2] = 0xffffffff;
					}

					pthread_mutex_lock(&Card_lock);
					Card_readPos++;
					pthread_mutex_unlock(&Card_lock);
				
					filesize = filesize - LEN_DA_PACKAGE;
				}else{
					// printf("-----ERRERRERR---##################package_offset=%d,package_size=%d,j=%d, Card_readPos=%d,i=%d\n",
					// 	package_offset,
					// 	package_size,
					// 	j,
					// 	Card_readPos,
					// 	i);
					close(fd);
					fd=-1;
					fr = NULL;
					if(buf_all!=NULL){
						free(buf_all);
						buf_all=NULL;
					}
					return NULL;
				}
			}
			if(Data_Get_Flag==0xF){ //DA stop
				close(fd);
				fd=-1;
				fr = NULL;
				if(buf_all!=NULL){
					free(buf_all);
					buf_all=NULL;
				}
				return NULL;
			}
			sleep(SleepTime);
			//usleep(1000000);
		}
		close(fd);
		fd=-1;
		if(Data_Get_Flag==0xF){ //DA stop
			printf("some all stop me, %d\n", __LINE__);
			if(buf_all!=NULL){
				free(buf_all);
				buf_all=NULL;
			}
			return NULL;
		}

	}
	
	i=10;
	while(1){
		if(Card_readPos==Card_writePos){
			break;
		}
		usleep(200000);
	}

	DA_STOP();
	if(buf_all!=NULL){
		free(buf_all);
		buf_all=NULL;
	}
	return NULL;
}

fd_set wfds;
int SendDaData(int DaHandle, int timeouts){ //消费者
	int ret;
	struct timeval *timeout;
	if(timeouts==-1){
		timeout = NULL;
	}else{
		struct timeval timeout1 = {timeouts, 0 };
		timeout= &timeout1;
	}
	FD_ZERO(&wfds);
	FD_SET(DaHandle, &wfds);
	ret = select(DaHandle+1, NULL, &wfds, NULL, timeout);
	if(ret < 0){
		printf("ret < 0\n");
		return -1;
	}
	else if(ret == 0){
		printf("ret == 0\n");
		return -1;
	}
	else{
		ioctl(DaHandle, CHECK_DA_FIFO, &ret);
		if(ret == 1){
			printf("------------+++++++++dainfo->Da_flag.dafifo_empty:%d\n",ret);
			//ioctl(DaHandle, MCASP_DA_STOP,NULL);
			DA_STOP();
			return 0xF;
		}

		pthread_mutex_lock(&Card_lock);
		if (Card_readPos > Card_writePos){
			pthread_mutex_unlock(&Card_lock);
				memcpy(da_buf[da_buf_cnt], sd_read_buffer[Card_writePos%sd_read_buffer_cnt], LEN_DA);
				
				// printf("                                        da_buf = %#08X, %#08X, %#08X, da_buf_cnt=%d\n",
				// 	da_buf[da_buf_cnt][0],
				// 	da_buf[da_buf_cnt][1],
				// 	da_buf[da_buf_cnt][2],
				// 	da_buf_cnt
				// );

				pthread_mutex_lock(&Card_lock);
				Card_writePos++;
				pthread_mutex_unlock(&Card_lock);
				// da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
				// FD_ZERO(&wfds);
				// FD_SET(DaHandle, &wfds);
				// ret = select(DaHandle+1, NULL, &wfds, NULL, NULL);
				// if(ret > 0){
				// 	memset(da_buf[da_buf_cnt], 0, LEN_DA);
				// }
				// da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
				// FD_ZERO(&wfds);
				// FD_SET(DaHandle, &wfds);
				// ret = select(DaHandle+1, NULL, &wfds, NULL, NULL);
				// if(ret > 0){
				// 	memset(da_buf[da_buf_cnt], 0, LEN_DA);
				// }
			//}
		}else{
			pthread_mutex_unlock(&Card_lock);
			memset(da_buf[da_buf_cnt], 0, LEN_DA);
		}
		da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
		// FD_ZERO(&wfds);
		// FD_SET(DaHandle, &wfds);
		// ret = select(DaHandle+1, NULL, &wfds, NULL, NULL);
		// if(ret > 0){
		// 	memset(da_buf[da_buf_cnt], 0, LEN_DA);
		// 	da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
		// }
		if(da_buf_cnt==11){
			
			FD_ZERO(&wfds);
			FD_SET(DaHandle, &wfds);
			ret = select(DaHandle+1, NULL, &wfds, NULL, NULL);
			if(ret > 0){
				memset(da_buf[da_buf_cnt], 0, LEN_DA);
				da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
			}
			FD_ZERO(&wfds);
			FD_SET(DaHandle, &wfds);
			ret = select(DaHandle+1, NULL, &wfds, NULL, NULL);
			if(ret > 0){
				memset(da_buf[da_buf_cnt], 0, LEN_DA);
				da_buf_cnt = (da_buf_cnt + 1)%da_sum_cnt;
			}
		}
	}

	return 0;

}


void *da_process_Num_Data(void){
	CLOSE_DA_FILE();

	while(1){
		if(Data_Get_Flag==3){
			break;
		}else{
			Data_Get_Flag=0xF;
			sleep(1);
		}
	}

	printf("--------------------the file name is %s\n", Readfile_num);
	int da_thread = pthread_create(&thread_ReadSD_Num, NULL, (void *)sd_read_num, NULL);
	if(da_thread != 0){
		printf("create da-read all data thread failed");
		pthread_exit(0);
	}
	int value = 0;
	int framePoint = LEN_DA;
	int clkdiv = 1;
	ioctl(DaHandle, SET_DAAD_SYNC, &value); 
	ioctl(DaHandle, MCASP_DA_EDMA_BUFSIZE, &framePoint); 
	ioctl(DaHandle, MCASP_DA_SAMPLE_RATE, &clkdiv);
	ioctl(DaHandle, MCASP_DA_START,NULL);
	da_buf_cnt = 0;
	while(1){
		if(Data_Get_Flag==0xF){ //DA stop
			printf("some one stop me %d\n", __LINE__);
			break;
		}
		SendDaData(DaHandle, -1);
	}

	ioctl(DaHandle, MCASP_DA_STOP,NULL);
	CLOSE_DA_FILE();
	STATE_MUTEX(STATE_WAITTING);
	return NULL;
}

int *da_process_ALL_Data(void){
	//sleep(1);
	CLOSE_DA_FILE();
	file_NUM = Get_FILE_NAME_LIST((char **)file_name_list);
	if(-1==file_NUM){
		printf("there is no file in the card,please,check.\n");
		DA_STOP();
		DA_READ_FILE_OVER();
		return NULL;
	}

	int da_thread = pthread_create(&thread_ReadAD_ALL, NULL, (void *)sd_read_all, NULL);
	if(da_thread != 0){
		printf("create da-read all data thread failed");
		pthread_exit(NULL);
	}
	int value = 0;
	int framePoint = LEN_DA;
	int clkdiv = 1;
	ioctl(DaHandle, SET_DAAD_SYNC, &value); 
	ioctl(DaHandle, MCASP_DA_EDMA_BUFSIZE, &framePoint); 
	ioctl(DaHandle, MCASP_DA_SAMPLE_RATE, &clkdiv);
	ioctl(DaHandle, MCASP_DA_START,NULL);
	da_buf_cnt = 0;
	while(1){
		if(Data_Get_Flag==0xF){ //DA stop
			printf("some one stop me %d\n", __LINE__);
			break;
		}
		SendDaData(DaHandle, -1);
	}

	ioctl(DaHandle, MCASP_DA_STOP,NULL);
	//DA_buff_destroy();
	CLOSE_DA_FILE();
	DA_READ_FILE_OVER();

	return NULL;

}


