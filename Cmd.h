#ifndef __3072_CMD__
#define __3072_CMD__
#include <pthread.h>

#define OMAPL138_GPIO_MAGIC 'k'
#define SET_GPIO_OUTPUT_HIGH 	_IOW(OMAPL138_GPIO_MAGIC, 1, int)
#define SET_GPIO_OUTPUT_LOW 	_IOW(OMAPL138_GPIO_MAGIC, 2, int)
#define SET_GPIO_07_OUTPUT_LOW 	_IOW(OMAPL138_GPIO_MAGIC, 3, int)
#define ENABLE_FPGACHARGE 		_IOW(OMAPL138_GPIO_MAGIC, 4, int)
#define DISABLE_FPGACHARGE 		_IOW(OMAPL138_GPIO_MAGIC, 5, int)
#define ENABLE_WIFI 			_IOW(OMAPL138_GPIO_MAGIC, 6, int)
#define DISABLE_WIFI 			_IOW(OMAPL138_GPIO_MAGIC, 7, int)
#define DATA_UART 				_IOW(OMAPL138_GPIO_MAGIC, 8, int)
#define DATA_MCBSP 				_IOW(OMAPL138_GPIO_MAGIC, 9, int)
#define STORAGE_STATUS 			_IOW(OMAPL138_GPIO_MAGIC, 10, int)
#define STORAGE_CMD 			_IOW(OMAPL138_GPIO_MAGIC, 11, int)
#define SYNC_CODE_LOST_CNT 		_IOW(OMAPL138_GPIO_MAGIC, 12, int)

#define CMD_CHECKSELF 		0x1001	//自检命令
#define CMD_STARTAD 		0x2001	//开始记录数据
#define CMD_STOPSTORE 		0x2002	//停止记录数据
#define CMD_STOPRECALL 		0x3001	//停止回读数据
#define CMD_RECALL 			0x3002	//回读指定文件
#define CMD_FORMATCARD 		0x4001	//存储卡格式化
#define CMD_RESET	 		0x5002	//存储卡复位

#define STATE_CHECKSELF 	0x10F1	//正在自检
#define STATE_STOREDATA 	0x20F1	//正在存储 
#define STATE_WAITTING		0x20F2	//空闲，等待存储或回读
#define STATE_RECALL 		0x30F2	//正在回放数据
#define STATE_FORMATCARD	0x40F1	//正在格式化
#define STATE_APPERR 		0xF001	//存储卡异常
#define STATE_CMDERR 		0xF002	//接收的命令异常
#define STATE_CHECKERR 		0xF003	//自检异常
#define STATE_GETCRCERR 	0xF004	//CRC异常
#define STATE_DRIVERERR 	0xF005	//mcasp没有中断

//int GET_ACTUAL_DATA = 0;

void *CMD_process(void);
int *CheckCardSelf(void);
int AD_STORE_FILE(void);
//unsigned int ForceRecord(void);
//unsigned int FormatCard(void);
void *FormatCard(void);
int init_CMD_Device();
int DA_READ_FILE_OVER(void);
int AD_STATE_FREE(void);

#pragma pack(push, 1)
struct sendState{	//16*8 = 128byte

	unsigned char  start;
	unsigned short get_cmd;
	unsigned short state;
	unsigned short file_num;
	unsigned short left_card_size;
	unsigned short cur_file_size;
	unsigned short reserved;
	unsigned short crc_cal;
	unsigned char  stop;

}Send_to_PC;

struct receiveCmd{	//10*8 = 80byte

	unsigned char  start;
	unsigned short get_cmd;
	unsigned short file_num;
	unsigned short reserved;
	unsigned short crc_num;
	unsigned char  stop;

}Recv_from_PC;
#pragma pack(pop)

pthread_mutex_t State_lock;
#define STATE_MUTEX(value)\
		pthread_mutex_lock(&State_lock);\
		Send_to_PC.state = value;\
		pthread_mutex_unlock(&State_lock);


#endif //__3072_CMD__
