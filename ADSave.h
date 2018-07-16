#ifndef __3072_AD__
#define __3072_AD__


#define MCASP_MAGIC 't'
#define MCASP_START 		_IOW(MCASP_MAGIC, 1, int)
#define MCASP_STOP  		_IOW(MCASP_MAGIC, 2, int)
#define MCASP_REASUM  		_IOW(MCASP_MAGIC, 3, int)
#define MCASP_EDMA_BUFSIZE  _IOW(MCASP_MAGIC, 4, int)
#define MCASP_FIFO  		_IOW(MCASP_MAGIC, 5, int)
#define MCASP_RXNUMEVT  	_IOW(MCASP_MAGIC, 6, int)
#define ENABLE_TACHO		_IOW(MCASP_MAGIC, 7, int)
#define ENABLE_DIGIT		_IOW(MCASP_MAGIC, 8, int)
#define INQUIRE_INTERRUPT_FLAG  _IOW(MCASP_MAGIC, 17, int) 

#define LEN  				196608 //4096*16*3
#define PAGE_SIZE 			4096
#define sec_size 			2*1024*1024

int *ad_process_data(void);
int init_AD_Device();
int AD_NAME_RESET(void);
int AD_STOP(void);
int AD_START(void);
int Get_FILE_NAME_LIST(char ** name);
int Get_ALL_FILE_NUM(void);
int Get_NOW_FILE(char * file);
int DA_NUM(int num);
int SD_FORMAT_CMD();
//int GET_AD_FLAG(int * flag);
int Get_NEW_FILE_NAME(void);
void CLOSE_AD_FILE();
int Get_AD_State(void);


#endif //__3072_AD__