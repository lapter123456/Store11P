#ifndef __3072_DA__
#define __3072_DA__

#define MCASP_DA_MAGIC 'u'
#define MCASP_DA_START 			_IOW(MCASP_DA_MAGIC, 1, int)
#define MCASP_DA_STOP  			_IOW(MCASP_DA_MAGIC, 2, int)
#define MCASP_DA_EDMA_BUFSIZE  	_IOW(MCASP_DA_MAGIC, 3, int)
#define MCASP_DA_SAMPLE_RATE 	_IOW(MCASP_DA_MAGIC, 4, int)
#define CHECK_DA_FIFO 			_IOW(MCASP_DA_MAGIC, 5, int)
#define SET_DAAD_SYNC 			_IOW(MCASP_DA_MAGIC, 6, int)
#define MCASP_DA_PREPARE_START 	_IOW(MCASP_DA_MAGIC, 7, int)

#define sd_read_buffer_cnt 	15


//int *da_process_Once_Data(void * param);
void *da_process_Num_Data(void);

//int *da_process_ALL_Data(void * param);
int init_DA_Device();
int DA_START(void);
int DA_STOP(void);
void CLOSE_DA_FILE();
int DA_SLEEP_COM();
int DA_SLEEP_LAN();
int DA_STATE(void);


#endif //__3072_DA__


