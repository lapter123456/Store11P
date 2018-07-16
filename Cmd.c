#include "exist.h"
#include "Cmd.h"
#include "ADSave.h"
#include "DAGetData.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
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


static int omapl138_gpio_handle=0;
static unsigned int disksize=0; //MB
static unsigned int freesize=0; //MB
static unsigned int Card_percent=0;
static char Card_Flag=1;
		// 1, 空闲，等待记录
		// 2, 格式化完成
		// 3, 正在格式化
		// 4, 正在记录 
		// 5, 回放全部（部分）
		// 6, 正在存数
static int ad_thread=-1, da_thread=-1, card_state_thread=-1, format_thread=-1, check_thread=-1;
pthread_t thread_AD, thread_DA, thread_DA_once, thread_State, thread_format, thread_check;
static char CardState=1;
extern char cur_file_name[100];

static unsigned short crc16_table[] = {
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

static unsigned short crc16_byte(unsigned short crc, const char data)
{
	return (crc<<8)^crc16_table[((crc>>8)^data)&0xff];
}

static unsigned short CalCRC16(void *pData, int dwNumOfBytes)
{
	unsigned short	wCRC = 0;	
	char *pbDataBuf = (char *)pData;
	while ( 0 != (dwNumOfBytes--) ){
		wCRC = crc16_byte(wCRC, *pbDataBuf++);
	}
	return	wCRC;
}

unsigned short CHECK_CMD_CRC(char *pdata, int num){
	return CalCRC16(pdata, num);
}

unsigned long get_system_tf_info(void){
    struct statfs dinfo;
    memset(&dinfo, 0, sizeof(dinfo));
    if(-1==statfs("/mnt/udisk", &dinfo)){
    	return 0;
    }
    unsigned long long bsize=dinfo.f_bsize;
    disksize=(bsize*dinfo.f_blocks)>>20; //MB
    freesize=(bsize*dinfo.f_bavail)>>20; //MB
    double all = dinfo.f_blocks-dinfo.f_bfree+dinfo.f_bavail;
    Card_percent = (dinfo.f_blocks-dinfo.f_bfree)*100/all + 1;
    // if((disksize)<1024){
    //     printf("&usbdisk=0&disksize=%ld",disksize);
    // }else{
   // printf("&usbdisk=1&disksize=%ld",disksize);
    // }
 //    printf("Card info: All[%.2f GB], Free[%d MB], Card_percent=%d%\n", 
	// 	(double)disksize/1024.0, freesize, Card_percent
	// );
    return disksize;
}

int init_CMD_Device(){ 	//init, open, caculate oriential num

	if(get_system_tf_info()<512){
		printf("card is missiing, memery aviable is %d MB\n", disksize);
	}
	if(freesize<512){
		printf("We found that the aviable memery is %d MB\n", freesize);
		printf("we will not run the program\n");
	}
	printf("Card info: All[%.2f GB], Free[%d MB], Card_percent=%d\n", 
		(float)disksize/1024.0, freesize, Card_percent
	);

	omapl138_gpio_handle = open("/dev/OMAPL138-GPIO", O_RDWR);
	if (omapl138_gpio_handle <= 0){
		perror("open omapl138_gpio_handle Err");
		return -4;
	}
	pthread_mutex_init(&State_lock, NULL);
	memset(&Send_to_PC, 0, sizeof(Send_to_PC));
	memset(&Recv_from_PC, 0, sizeof(Recv_from_PC));
 
	return 0;
}

static unsigned short MB_Size(char * name){
	unsigned long long f_size=0;
	struct stat filestat;
	if(0==stat(name, &filestat)){
		f_size = filestat.st_size;
	}
	
	//printf("--++f_size=%llu Bytes\n", f_size);
	unsigned short size_MB = (unsigned short)(f_size/1024/1024);
	return size_MB;
}

int AD_STORE_FILE(void){

	if(Card_Flag != 4)
		Card_Flag = 6;
	return 0;
}

int AD_STATE_FREE(void){

	if(Card_Flag != 4 && Card_Flag != 3)
		Card_Flag = 1;
	return 0;
}

int DA_READ_FILE_OVER(void){
	if(Card_Flag==3 || Card_Flag==2){
		return 0;
	}
	if(Card_Flag!=1){
		
		Get_NEW_FILE_NAME();
		ad_thread = pthread_create(&thread_AD, NULL, (void *)ad_process_data, NULL);
		if(ad_thread != 0){
			printf("create ad thread failed:%d\n",ad_thread);
			pthread_exit(NULL);
			printf("AD init failed\n");
		}
		AD_START();
		Card_Flag = 1;
	}else{
		printf("there is one AD thread running already.\n");
	}
	return 0;
}

void Check_AD_State(void){
	 if((Card_Flag==1 || Card_Flag==6)  && Get_AD_State()==0){

	 	sleep(5);
	 	if(Get_AD_State()==0 && (Card_Flag==1|| Card_Flag==6)){
	 		sleep(2);
	 		if(Get_AD_State()==0 && (Card_Flag==1 || Card_Flag==6)){
				Get_NEW_FILE_NAME();
				ad_thread = pthread_create(&thread_AD, NULL, (void *)ad_process_data, NULL);
				if(ad_thread != 0){
					printf("create ad thread failed:%d\n",ad_thread);
					pthread_exit(NULL);
					printf("AD init failed\n");
				}
				AD_START();
				Card_Flag = 1;
			}
		}
	}
}

void *Card_State_process(void){
	//int NUM;
	char check[16]={0};

	STATE_MUTEX(STATE_WAITTING);
	while(1){
		get_system_tf_info();
		Send_to_PC.start = 0;
		Send_to_PC.stop  = 0xFF;
		Send_to_PC.reserved = 0;
		Send_to_PC.left_card_size = freesize;
		Send_to_PC.cur_file_size  = MB_Size(cur_file_name);
		Send_to_PC.file_num = Get_ALL_FILE_NUM() & 0xFFFF;
		memcpy(check, &Send_to_PC, 16);
		Send_to_PC.crc_cal = CHECK_CMD_CRC(check+1, 12);

		ioctl(omapl138_gpio_handle, STORAGE_STATUS, &Send_to_PC);
		
		printf("[Card_Stat]:Free[%d MB],CurFile[%d MB],", 
			Send_to_PC.left_card_size, Send_to_PC.cur_file_size);
		printf("STATE: %#X, fileNum=%d\n", 
			Send_to_PC.state, Send_to_PC.file_num);
		sleep(1);
	}
}

void *CMD_process(void)
{
	STATE_MUTEX(STATE_WAITTING);
	Send_to_PC.get_cmd = 0;
	ad_thread = pthread_create(&thread_AD, NULL, (void *)ad_process_data, NULL);
	if(ad_thread != 0){
		printf("create ad thread failed:%d\n",ad_thread);
		printf("AD init failed\n");
		pthread_exit(NULL);
	}else{
		printf("++++++++++++creat ad thread OK\n");
	}

	da_thread = pthread_create(&thread_DA, NULL, (void *)da_process_Num_Data,NULL);
	if(da_thread != 0){
		printf("create da-read all data thread failed");
		pthread_exit(NULL);
	}else{
		printf("++++++++++++creat da thread OK\n");
	}
	
	sleep(2);

	card_state_thread = pthread_create(&thread_State, NULL, (void *)Card_State_process, NULL);
	if(card_state_thread != 0){
		printf("create ad thread failed:%d\n",card_state_thread);
		pthread_exit(NULL);
		printf("AD init failed\n");
		exit(0);
	}

	//AD_SAVE_FALG=1;
	char storage_cmd[10];

	while(1){ //循环获取命令
		READCMD:
		read(omapl138_gpio_handle, storage_cmd, 10);
		memcpy(&Recv_from_PC, storage_cmd, 10);
		unsigned short check_crc_num = CHECK_CMD_CRC(storage_cmd+1, 6);
		// &Recv_from_PC = (struct receiveCmd *)storage_cmd
		if(Recv_from_PC.crc_num!=check_crc_num){
			STATE_MUTEX(STATE_GETCRCERR);
			printf("get_crc_num=%#X, check_crc_num=%#X\n", Recv_from_PC.crc_num, check_crc_num);
			goto READCMD;
		}

		printf("get_cmd:%#X, CARD_STATE: %d\n",Recv_from_PC.get_cmd, Card_Flag);
		Send_to_PC.get_cmd = Recv_from_PC.get_cmd;
		switch(Recv_from_PC.get_cmd){
			case CMD_CHECKSELF: //自检命令
				if(Send_to_PC.state != STATE_WAITTING){
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: CMD_CHECKSELF\n");

				check_thread = pthread_create(&thread_check, NULL, (void *)CheckCardSelf, NULL);
				if(check_thread != 0){
					printf("create ad thread failed:%d\n",check_thread);
					pthread_exit(NULL);
					printf("thread_check init failed\n");
				}
				break;
					
			case (CMD_FORMATCARD): //擦除，格式化卡, 3
				if(Send_to_PC.state != STATE_WAITTING){
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: CMD_FORMATCARD\n");

				format_thread = pthread_create(&thread_format, NULL, (void *)FormatCard, NULL);
				if(format_thread != 0){
					printf("create ad thread failed:%d\n",format_thread);
					pthread_exit(NULL);
					printf("format_thread init failed\n");
				}
				break;		

			case CMD_STARTAD: //开始记录数据
				if(Send_to_PC.state!=STATE_WAITTING){
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: \n");
				
				
				break;

			case CMD_STOPSTORE: //停止记录数据
				if(Send_to_PC.state != STATE_STOREDATA){ //???
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: \n");
				AD_STOP();
				break;

			case CMD_RECALL: //回读指定文件
				if(Send_to_PC.state!=STATE_WAITTING){
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: \n");

				if(-1 == DA_NUM(Recv_from_PC.file_num)){
					break;
				}
				DA_START();
				// da_thread = pthread_create(&thread_DA, NULL, (void *)da_process_Num_Data,NULL);
				// if(da_thread != 0){
				// 	printf("create da-read all data thread failed");
				// 	pthread_exit(NULL);
				// }else{
				// 	printf("++++++++++++creat da thread OK\n");
				// }
				break;
				
			case CMD_STOPRECALL: //停止回读数据
				if(Send_to_PC.state!=STATE_RECALL){
					printf("Cur_state is not aviable to exec this function\n");
					break;
				}
				printf("recv the function cmd: \n");
				DA_STOP();
				break;	

			case CMD_RESET:	//存储卡复位
				system("cp -f /mnt/yaffs/umnt /mnt/sd_card/umnt");
				system("/mnt/sd_card/umnt");
				sleep(3);
				system("/mnt/yaffs/reset.sh");
				break;

			default:
				printf("not support CMD: %#X\n", Send_to_PC.state);	
		}
	}

	return 0;
}

void *FormatCard(void){
	int i = 0;
	STATE_MUTEX(STATE_FORMATCARD);
	system("umount /mnt/udisk");
	while(get_system_tf_info()>1024){ //存储卡是否卸载完毕
		sleep(1);
		CLOSE_AD_FILE();
		CLOSE_DA_FILE();
		sync();
		system("umount /mnt/udisk");
		if(++i>30){
			printf("We can't umount the Card, we will use 'rm -rf *' to delete all files in the card\n");
			system("rm -rf /mnt/sd_card/* &");
			sleep(3);
			AD_NAME_RESET();
			STATE_MUTEX(STATE_WAITTING);
			return NULL;
		}
	}
	printf("card format is running\n");
	system("mkfs.ext4 /dev/mmcblk0p1 && mount /dev/mmcblk0p1 /mnt/sd_card");
	i=0;
	while(i<300){
		sleep(1); 
		i++;
		printf("we wait the card mount...%ds\n", i);
		if(get_system_tf_info()>1024){ //格式化是否完成
			AD_NAME_RESET();
			STATE_MUTEX(STATE_WAITTING);
			return NULL;
		}else{
			system("mount /dev/mmcblk0p1 /mnt/sd_card");
		}
	}
	return 0;
}

int *CheckCardSelf(void){
	STATE_MUTEX(STATE_CHECKSELF);
	system("rm -rf /mnt/sd_card/check_self.bin");
	int i = 0, f_check=-1;
	unsigned char tmp[1024]={0};
	struct stat statbuff;
	unsigned long long filesize_check=0;
	for(i=0;i<1024;i++){
		tmp[i] = i%256;
	}

	get_system_tf_info();
	int Card_size = freesize;

	if(Card_size>150){ //大于 150 MB, 24秒
		f_check = open("/mnt/sd_card/check_self.bin", O_RDWR | O_CREAT);
		for(i=0;i<1024*150;i++){
			write(f_check, tmp, 1024);
		}
		close(f_check);
		stat("/mnt/sd_card/check_self.bin", &statbuff);
		filesize_check = statbuff.st_size;
		if(filesize_check==150*1024*1024){
			system("rm -rf /mnt/sd_card/check_self.bin");
			get_system_tf_info();
			STATE_MUTEX(STATE_WAITTING);
			return NULL;
		}
	}else if(Card_size>1){//大于 1 MB， 8秒
		f_check = open("/mnt/sd_card/check_self.bin", O_RDWR | O_CREAT);
		for(i=0;i<1024*1;i++){
			write(f_check, tmp, 1024);
		}
		close(f_check);

		stat("/mnt/sd_card/check_self.bin", &statbuff);
		filesize_check = statbuff.st_size;
		if(filesize_check==1*1024*1024){
			system("rm -rf /mnt/sd_card/check_self.bin");
			get_system_tf_info();
			sleep(8);
			STATE_MUTEX(STATE_WAITTING);
			return NULL;
		}
	}else{ // 1个字节， 5秒
		if(disksize>1024){
			f_check = open("/mnt/sd_card/check_self.bin", O_RDWR | O_CREAT);
			for(i=0;i<1;i++){
				write(f_check, tmp, 1);
			}
			close(f_check);

			stat("/mnt/sd_card/check_self.bin", &statbuff);
			filesize_check = statbuff.st_size;
			if(filesize_check==1){
				system("rm -rf /mnt/sd_card/check_self.bin");
				get_system_tf_info();
				sleep(5);
				STATE_MUTEX(STATE_WAITTING);
				return NULL;
			}
		}
	}

	STATE_MUTEX(STATE_CHECKERR);

	return NULL;
}

