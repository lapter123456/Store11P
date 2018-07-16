#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

/**
* @brief 判断文件是否存在
* @param file_path 文件名称，包含路径
* return 文件路径下不存在 返回 -1
					存在  返回 0

	accsee(const char *path_name, int mode);
	mode: 	F_OK 测试文件是否存在
			R_OK 测试读取权限
			W_OK 测试写权限
			X_OK 测试执行权限
*/
int is_file_exist(const char * file_path)
{
	if(file_path == NULL){
		return -2;
	}
	if(access(file_path, F_OK)==0){
		return 0;
	}
	return -1;
}

/**
* @brief 判断目录是否存在
* @param dir_path 目录名称，包含路径
* @return 存在返回值0， 不存在返回-1
*/
int is_dir_exist(const char *dir_path)
{	DIR *dirptr=NULL;
	if(dir_path==NULL){
		return -2;
	}
	if((dirptr=opendir(dir_path)) == NULL){
		return -1;
	}
	return 0;
}

