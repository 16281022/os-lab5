#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#define _CRT_SECURE_NO_WARNINGS

using namespace std;

#include "file.h"

#define DIR		0
#define	CREATE	1
#define	DELETE	2
#define	OPEN	3
#define	CLOSE	4
#define	READ	5	
#define	WRITE	6
#define HELP	7
#define LSEEK	8
#define EXIT	9
#define	OPLIST	10
#define LDISK	11
#define CH_LENGTH	20

void read_block(int i, char *p)
{
	char * temp = (char *)malloc(sizeof(char));
	temp = p;
	for (int a = 0; a < B;)
	{
		*temp = ldisk[i][a];
		a++;
		temp++;
	}
}

void write_block(int i, char *p)
{
	char * temp = (char *)malloc(sizeof(char));
	temp = p;
	for (int a = 0; a < B;)
	{
		ldisk[i][a] = *temp;
		a++;
		temp++;
	}
}

void Init_block(char *temp, int length)       /*初始化字符数组内容为\0*/
{
	int i;
	for (i = 0; i < length; i++)
	{
		temp[i] = '\0';
	}
}

int write_buffer(int index, int list)
{

	int i;
	int j;
	int freed;
	char temp[B];

	int buffer_length = BUFFER_LENGTH;
	for (i = 0; i < BUFFER_LENGTH; i++)
	{
		if (open_list[list].buffer[i] == '\0')
		{
			buffer_length = i;					//缓冲区有效长度
			break;
		}
	}

	int x = open_list[list].pointer[0];
	int y = open_list[list].pointer[1];
	int z = B - y;									//当前块空闲容量

	if (buffer_length < z)					//块容量可写入缓冲区不需要再分配
	{
		read_block(x, temp);
		strncat(temp + y, open_list[list].buffer, buffer_length);			//缓冲区接入
		write_block(x, temp);

		read_block(index + FILE_SIGN_AREA, temp);	//更改文件长度
		temp[1] += buffer_length;
		//printf("temp[1] = %d\n",temp[1]);
		write_block(index + FILE_SIGN_AREA, temp);

		open_list[list].pointer[0] = x;
		open_list[list].pointer[1] = y + buffer_length;					//更新文件读写指针
	}
	else									//大于需要分配新块
	{
		read_block(index + FILE_SIGN_AREA, temp);
		if (temp[2] + (buffer_length - z) / B + 1 > FILE_BLOCK_LENGTH)
		{
			cout << "文件分配数组空间不足\n" << endl;
			return ERROR;
		}

		//填满
		read_block(x, temp);
		strncat(temp + y, open_list[list].buffer, z);			//缓冲区接入的长度填满当前块
		write_block(x, temp);

		//分配(buffer_length - z)/B+1块 

		//寻找文件区（目录项之后）的空闲块分配新块
		for (i = 0; i < (buffer_length - z) / B; i++)
		{
			for (j = K + FILE_NUM; j < L; j++)
			{
				read_block((j - K) / B, temp);
				if (temp[(j - K) % B] == FREE)
				{
					freed = j;
					break;
				}
			}
			if (j == L)
			{
				cout << "分配失败！" << endl;
				return ERROR;
			}

			Init_block(temp, B);
			strncpy(temp, (open_list[list].buffer + z + (i*B)), B);
			write_block(freed, temp);				//写入内容

			read_block((freed - K) / B, temp);			//更改位图状态
			temp[(freed - K) % B] = BUSY;
			write_block((freed - K) / B, temp);			//写入磁盘位图

			read_block(index + FILE_SIGN_AREA, temp);
			temp[2] ++;								//增加文件分配数组长度
			temp[2 + temp[2]] = freed;
			write_block(index + FILE_SIGN_AREA, temp);
		}

		//(buffer_length - z)%B 不满一块在尾部的部分
		for (j = K + FILE_NUM; j < L; j++)
		{
			read_block((j - K) / B, temp);
			if (temp[(j - K) % B] == FREE)
			{
				freed = j;
				break;
			}
		}
		if (j == L)
		{
			cout << "分配失败！" << endl;
			return ERROR;
		}
		Init_block(temp, B);
		strncpy(temp, (open_list[list].buffer + z + (i*B)), (buffer_length - z) % B);
		write_block(freed, temp);				//写入内容

		read_block((freed - K) / B, temp);			//更改位图状态
		temp[(freed - K) % B] = BUSY;
		write_block((freed - K) / B, temp);			//写入磁盘位图

		read_block(index + FILE_SIGN_AREA, temp);
		temp[2] ++;								//增加文件分配数组长度
		temp[2 + temp[2]] = freed;
		write_block(index + FILE_SIGN_AREA, temp);


		read_block(index + FILE_SIGN_AREA, temp);
		temp[1] += buffer_length;				//更改文件长度
		write_block(index + FILE_SIGN_AREA, temp);

		open_list[list].pointer[0] = freed;
		open_list[list].pointer[1] = (buffer_length - z) % B;
	}
	//	printf("X = %d, Y = %d\n",open_list[list].pointer[0],open_list[list].pointer[1]);
}
int lseek(int index, int pos)
{
	int i;
	int list = -1;
	char temp[B];
	int pos_i = pos / B;				//在文件所有块数中的第几块
	int pos_j = pos % B;				//在第某块中的某个位置
	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == index)
		{
			list = i;
			break;
		}
	}

	if (list == -1)				
	{
		cout << "未找到索引号" << endl;
		return ERROR;
	}
	if (open_list[list].flag != BUSY)		
	{
		cout << "索引号有误" << endl;
		return ERROR;
	}
	read_block(open_list[list].filesignnum + FILE_SIGN_AREA, temp);
	if (pos_i > temp[2] - 1)											//大于文件实际的块数时
	{
		cout << "越界" << endl;
		return ERROR;
	}

	open_list[list].pointer[0] = temp[3 + pos_i];				//该文件某块实际磁盘地址
	open_list[list].pointer[1] = pos_j;


	return OK;


}


void Init()                /*初始化磁盘将磁盘全部置空，然后创建0号文件描述符为根目录的文件描述符初始化位图区*/
{

	int i;
	char temp[B];


	for (i = 0; i < L; i++)
	{
		Init_block(temp, B);
		write_block(i, temp);			//初始化磁盘
	}

	for (i = K; i < L; i++)						//初始化保留区中的位图
	{
		read_block((i - K) / B, temp);
		temp[(i - K) % B] = FREE;
		write_block((i - K) % B, temp);
	}

	//初始化目录的文件标识符
	filesign temp_cnt_sign;
	temp_cnt_sign.filesign_flag = 1;
	temp_cnt_sign.file_length = 0;					//文件长度初始为0					
	temp_cnt_sign.file_block = FILE_BLOCK_LENGTH;	//分配FILE_BLOCK_LENGTH*B的空间（在数据区前段）用于储存目录项

	Init_block(temp, B);
	temp[0] = temp_cnt_sign.filesign_flag;
	temp[1] = temp_cnt_sign.file_length;
	temp[2] = temp_cnt_sign.file_block;

	for (i = 0; i < FILE_BLOCK_LENGTH; i++)
	{
		temp[i + 3] = K + i;									//默认数据区的FILE_BLOCK_LENGTH被根目录占用
	}
	write_block(FILE_SIGN_AREA, temp);

	//更改位图区状态
	read_block(0, temp);
	for (i = 0; i < FILE_NUM; i++)
	{
		temp[i] = FREE;					
	}
	write_block(0, temp);
}


int create(char filename[])
{
	int i;
	int frees;							
	int	freed;							
	int freed2;
	char temps[B];
	char tempc[B];
	char temp[B];
	//查看文件名是否存在
	for (i = K; i < K + FILE_NUM; i++)
	{
		read_block((i - K) / B, temp);
		if (temp[(i - K) % B] == BUSY)
		{
			read_block(i, temp);
			if (strncmp(temp + 1, filename, FILE_NAME_LENGTH) == 0)
			{
				cout << "该目录已经存在文件名:" << filename << endl;
				return ERROR;
			}
		}
	}
	//寻找保留区中空闲的文件描述符
	for (i = FILE_SIGN_AREA; i < K; i++)
	{
		read_block(i, temp);
		if (temp[0] == FREE)
		{
			frees = i;
			break;
		}
	}
	if (i == K)
	{
		cout << "没有空闲的文件描述符" << endl;
		return ERROR;
	}
	for (i = K; i < K + FILE_NUM; i++)
	{
		read_block((i - K) / B, temp);
		if (temp[(i - K) % B] == FREE)
		{
			freed = i;
			break;
		}
	}
	if (i == K + FILE_NUM)
	{
		cout << "文件个数达到上限" << endl;
		return ERROR;
	}

	for (i = K + FILE_NUM; i < L; i++)
	{
		read_block((i - K) / B, temp);
		if (temp[(i - K) % B] == FREE)
		{
			freed2 = i;
			break;
		}
	}
	if (i == L)
	{
		cout << "磁盘已满，分配失败" << endl;
		return ERROR;
	}


	filesign temp_filesign;						
	contents temp_contents;					

	//构建文件描述符
	temp_filesign.filesign_flag = 1;
	temp_filesign.file_length = 0;
	temp_filesign.file_block = 1;


	Init_block(temps, B);
	temps[0] = temp_filesign.filesign_flag;
	temps[1] = temp_filesign.file_length;
	temps[2] = temp_filesign.file_block;
	temps[3] = freed2;
	for (i = 4; i < FILE_BLOCK_LENGTH; i++)
	{
		temps[i] = '\0';
	}
	write_block(frees, temps);				

											
	temp_contents.filesignnum = frees - FILE_SIGN_AREA;
	strncpy(temp_contents.filename, filename, FILE_NAME_LENGTH);

	Init_block(tempc, B);
	tempc[0] = temp_contents.filesignnum;
	tempc[1] = '\0';
	strcat(tempc, temp_contents.filename);
	write_block(freed, tempc);				

	//更改位图状态
	read_block((freed - K) / B, temp);			
	temp[(freed - K) % B] = BUSY;
	write_block((freed - K) / B, temp);			

	read_block((freed2 - K) / B, temp);			
	temp[(freed2 - K) % B] = BUSY;
	write_block((freed2 - K) / B, temp);			

	read_block(FILE_SIGN_AREA, temp);
	temp[1]++;
	write_block(FILE_SIGN_AREA, temp);

	return OK;


}

int destroy(char * filename)
{
	int i;
	int dtys;										//将要删除的文件的目录项的文件描述符位置
	int dtyd;										//将要删除的文件的目录项位置
	int use_block;									//该文件实际使用的块数
	int index;
	char temp[B];
	char tempd[B];


	//依据文件名寻找文件的目录项和文件描述符
	for (i = K; i < K + FILE_NUM; i++)
	{
		read_block((i - K) / B, temp);
		if (temp[(i - K) % B] == BUSY)
		{
			read_block(i, temp);
			if (strncmp(temp + 1, filename, FILE_NAME_LENGTH) == 0)
			{
				dtyd = i;								
				dtys = temp[0] + FILE_SIGN_AREA;		
				index = temp[0];
				break;
			}
		}
	}
	if (i == K + FILE_NUM)
	{
		cout << "没有找到文件" << endl;
		return ERROR;
	}

	//根据文件描述符查看该文件是否打开
	int list = -1;
	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == index)
		{
			list = i;
			break;
		}
	}
	if (open_list[list].flag == BUSY && list != -1)
	{
		cout << "文件已经达科，无法删除" << endl;
		return ERROR;
	}

	read_block(dtys, temp);
	use_block = temp[2];
	for (i = 0; i < use_block; i++)
	{
		read_block((temp[i + 3] - K) / B, tempd);
		tempd[(temp[i + 3] - K) % B] = FREE;
		write_block((temp[i + 3] - K) / B, tempd);
	}
	//删除该目录项
	Init_block(temp, B);
	write_block(dtys, temp);




	//删除文件描述符
	Init_block(temp, B);
	write_block(dtyd, temp);

	//更改位图区
	read_block((dtyd - K) / B, temp);
	temp[(dtyd - K) % B] = FREE;
	write_block((dtyd - K) / B, temp);
	//目录文件描述符中的长度减1
	read_block(FILE_SIGN_AREA, temp);
	temp[1]--;
	write_block(FILE_SIGN_AREA, temp);


	return OK;

}
int open(char * filename)
{
	int i;
	int opd;
	int ops;
	int list;
	char temp[B];
	int index;
	//依据文件名寻找文件的目录项和文件描述符
	for (i = K; i < K + FILE_NUM; i++)
	{
		read_block((i - K) / B, temp);
		if (temp[(i - K) % B] == BUSY)
		{
			read_block(i, temp);
			if (strncmp(temp + 1, filename, FILE_NAME_LENGTH) == 0)
			{
				opd = i;						
				ops = temp[0];							
													
				break;
			}
		}
	}
	if (i == K + FILE_NUM)
	{
		cout << "没有找到文件" << endl;
		return ERROR;
	}

	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == ops && open_list[i].flag == BUSY)
		{
			cout << "文件已经被打开" << endl;
			return ERROR;
		}
	}

	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].flag != BUSY)
		{
			list = i;
			break;
		}
	}

	//对表目进行操作

	open_list[list].filesignnum = ops;								//写入文件描述符序号

	open_list[list].flag = BUSY;									//置标志位为占用

	index = open_list[list].filesignnum;									
	lseek(index, 0);													

	Init_block(open_list[list].buffer, BUFFER_LENGTH);				
	read_block(open_list[list].pointer[0], temp);						
	strncpy(open_list[list].buffer, temp, BUFFER_LENGTH);				

	return OK;

}

int close(int index)
{
	int i;
	int list = -1;
	char temp[B];
	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == index)
		{
			list = i;
			break;
		}
	}
	if (list == -1)					
	{
		cout << "未找到索引号，操作失败" << endl;
		return ERROR;
	}
	if (open_list[list].flag != BUSY)	
	{
		cout << "索引号有误，操作失败" << endl;
		return ERROR;
	}
	
	write_buffer(index, list);	

    //释放表目
	Init_block(open_list[list].buffer, BUFFER_LENGTH);				
	open_list[list].filesignnum = FREE;								
	open_list[list].flag = FREE;									
	open_list[list].pointer[0] = NULL;								
	open_list[list].pointer[1] = NULL;
	return OK;
}
int read(int index, int mem_area, int count)
{
	int i;
	int list = -1;
	char temp[B];

	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == index)
		{
			list = i;
			break;
		}
	}
	if (list == -1)			
	{
		cout << "未找到索引号，操作失败" << endl;
		return ERROR;
	}
	if (open_list[list].flag != BUSY)		
	{
		cout << "索引号有误，操作失败" << endl;
		return ERROR;
	}


	char temp_output[OUTPUT_LENGTH];
	Init_block(temp_output, OUTPUT_LENGTH);
	char output[OUTPUT_LENGTH];
	Init_block(output, OUTPUT_LENGTH);

	read_block(FILE_SIGN_AREA + index, temp);
	int file_length = temp[1];					
	int file_block = temp[2];					
	int file_area;

	//拷贝文件内容
	for (i = 0; i < file_block - 1; i++)
	{
		read_block(FILE_SIGN_AREA + index, temp);
		read_block(temp[3 + i], temp);
		strncpy(temp_output + i*B, temp, B);
	}
	read_block(FILE_SIGN_AREA + index, temp);
	read_block(temp[3 + i], temp);
	strncpy(temp_output + i*B, temp, B);

    //读写坐标
	int x = open_list[list].pointer[0];
	int y = open_list[list].pointer[1];

	for (i = 0; i < file_block; i++)
	{
		read_block(FILE_SIGN_AREA + index, temp);
		if (temp[3 + i] == x)
		{
			break;
		}
	}
	file_area = i * B + y;							//转换为文件内相对位置								

	for (i = 0; i < count; i++)
	{
		output[i + mem_area] = temp_output[i + file_area];
	}

	cout<< output + mem_area << endl;
	return OK;
}
int write(int index, int mem_area, int count)
{
	int i;
	int list = -1;
	int input_length;
	char temp[B];

	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].filesignnum == index)
		{
			list = i;
			break;
		}
	}
	if (list == -1)				
	{
		cout << "未找到索引号，操作失败" << endl;
		return ERROR;
	}
	if (open_list[list].flag != BUSY)	
	{
		cout << "索引号有误，操作失败" << endl;
		return ERROR;
	}

	char input[INPUT_LENGTH];
	Init_block(input, INPUT_LENGTH);
	i = 0;
	fflush(stdin);
	while (scanf("%c", &input[i]))
	{
		if (input[i] == '\n')											
		{
			input[i] == '\0';
			break;
		}
		i++;
	}
	input_length = i;

	if (count <= BUFFER_LENGTH)
	{
		strncat(open_list[list].buffer, input + mem_area, count);		
	}
	else
	{
		int rest;						
		for (i = 0; i < BUFFER_LENGTH; i++)
		{
			if (open_list[list].buffer[i] == FREE)
			{
				rest = BUFFER_LENGTH - i;
				break;
			}
		}
			
		strncat(open_list[list].buffer + BUFFER_LENGTH - rest, input + mem_area, rest);
		write_buffer(index, list);
		Init_block(open_list[list].buffer, BUFFER_LENGTH);
		
		for (i = 0; i < (count / BUFFER_LENGTH) - 1; i++)
		{
			strncpy(open_list[list].buffer, (input + mem_area) + rest + i*BUFFER_LENGTH, BUFFER_LENGTH);
			
			write_buffer(index, list);
			Init_block(open_list[list].buffer, BUFFER_LENGTH);
		}
		
		Init_block(open_list[list].buffer, BUFFER_LENGTH);
		strncpy(open_list[list].buffer, (input + mem_area) + rest + i*BUFFER_LENGTH, count%BUFFER_LENGTH);
		int buffer_start;
		
	}
	return OK;
}

void directory()
{
	int i;
	int filenum;
	int filelength;
	char filename[FILE_NAME_LENGTH];
	char temp[B];
	char tempd[B];
	char temps[B];
	read_block(FILE_SIGN_AREA, temp);
	filenum = temp[1];						//实际存在的文件个数
	cout << endl;
	if (filenum == 0)
	{
		cout << "没有文件" << endl;
	}

	for (i = 0; i < FILE_NUM; i++)
	{
		read_block(temp[3 + i], tempd);					//读取目录项
		if (tempd[0] != 0)
		{
			read_block(tempd[0] + FILE_SIGN_AREA, temps);		//读取文件描述符
			if (temps[0] == BUSY && tempd[0] != 0)
			{
				filelength = temps[1];
				strcpy(filename, tempd + 1);
				printf_s("%-10s\t\t%-2d字节\n", filename, filelength);
			}
		}
	}

	if (filenum != 0)
	{
		cout << "\t\t\t\t" << filenum << "个文件" << endl;
	}
}

int show_openlist()
{

	int i, j;
	int openfile = 0;
	char temp[B];
	int index;

	for (i = 0; i < FILE_NUM; i++)
	{
		if (open_list[i].flag == BUSY)
		{
			index = open_list[i].filesignnum;
			printf_s("  %-2d", index);
			openfile++;
			read_block(FILE_SIGN_AREA + index, temp);
			printf_s("\t\t %-2d", temp[1]);
			for (j = K; j < K + FILE_NUM; j++)
			{
				read_block(j, temp);
				if (temp[0] == index)
				{
					printf("\t\t%-10s\n", temp + 1);
				}
			}
		}
	}
	return openfile;

}
void show_help()
{
	printf("**----------------------------------------------------------------------**\n");
	printf("** dir                  显示目录内容                                    **\n");
	printf("**                                                                      **\n");
	printf("** create+filename      新建以filename为文件名的文件                    **\n");
	printf("**                                                                      **\n");
	printf("** delete+filename      删除以filename为文件名的文件                    **\n");
	printf("**                                                                      **\n");
	printf("** open+filename        打开以filename为文件名的文件                    **\n");
	printf("**                                                                      **\n");
	printf("** close                关闭index为索引号的文件                         **\n");
	printf("**                                                                      **\n");
	printf("** read                 请根据提示，接着要求输入索引号以及读取长度进行读**\n");
	printf("**                      取文件操作                                      **\n");
	printf("**                                                                      **\n");
	printf("** write                请根据提示，接着要求输入索引号以及写入长度进行写**\n");
	printf("**                      读文件操作                                      **\n");
	printf("**                                                                      **\n");
	printf("** lseek                请根据提示，接着要求输入一个不大于文件长度的数字**\n");
	printf("**                      用于定位读写指针                                **\n");
	printf("**                                                                      **\n");
	printf("** help                 帮助                                            **\n");
	printf("**                                                                      **\n");
	printf("** exit                 退出文件系统                                    **\n");
	printf("**----------------------------------------------------------------------**\n");
}



void show_ldisk()
{
	int a, b;
	for (a = 0; a < K + 30; a++)
	{
		printf_s("%-3d :", a);
		for (b = 0; b< B; b++)
		{
			printf_s("%-3d ", ldisk[a][b]);
		}
		cout << endl;
	}
}

void main()
{
	cout << "\t\t\t 16281022文件系统" << endl;
	show_help();
	Init();
	create("zhy1");
	create("zhy2");
	create("zhy3");
	open("zhy1");
	open("zhy3");

	char ch[CH_LENGTH];
	Init_block(ch, CH_LENGTH);
	while (gets_s(ch))
	{

		int cmd;
		char filename[FILE_NAME_LENGTH];
		cmd = -1;
		Init_block(filename, FILE_NAME_LENGTH);
		if (strncmp("dir", ch, 3) == 0)			
		{
			cmd = DIR;
		}
		if (strncmp("create", ch, 6) == 0)		
		{
			cmd = CREATE;
			strcat(filename, ch + 7);
		}
		if (strncmp("delete", ch, 6) == 0)			
		{
			cmd = DELETE;
			strcat(filename, ch + 7);
		}
		if (strncmp("open", ch, 4) == 0)		
		{
			cmd = OPEN;
			strcat(filename, ch + 5);
		}
		if (strncmp("close", ch, 5) == 0)			
		{
			cmd = CLOSE;
		}
		if (strncmp("read", ch, 4) == 0)			
		{
			cmd = READ;
		}
		if (strncmp("write", ch, 5) == 0)			
		{
			cmd = WRITE;
		}
		if (strncmp("lseek", ch, 5) == 0)			
		{
			cmd = LSEEK;
		}
		if (strncmp("oplist", ch, 6) == 0)			
		{
			cmd = OPLIST;
		}
		if (strncmp("exit", ch, 4) == 0)			
		{
			cmd = EXIT;
			break;
		}
		if (strncmp("ldisk", ch, 5) == 0)			
		{
			cmd = LDISK;
		}
		if (strncmp("help", ch, 4) == 0)			
		{
			cmd = HELP;
		}
		int index, count, pos;
		switch (cmd)
		{
		case DIR:
			directory();
			cout<< "----------------------------------------------" << endl;
			break;
		case CREATE:
			if (create(filename) == OK)
				cout << "创建文件成功" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		case DELETE:
			if (destroy(filename) == OK)
				cout << "删除文件成功" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		case OPEN:
			if (open(filename) == OK)
				cout << "打开文件成功" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		case CLOSE:
			if (show_openlist() == 0)
			{
				cout << "没有文件被打开" << endl;
				cout << "----------------------------------------------" << endl;
				break;
			}
			cout << "请输入想要关闭文件的索引号" << endl;
			scanf("%d", &index);
			if (close(index) == OK)
				cout << "关闭操作成功" << endl;
			cout << "----------------------------------------------" << endl;
			getchar();
			break;
		case READ:
			if (show_openlist() == 0)
			{
				cout << "没有文件被打开" << endl;
				cout << "----------------------------------------------" << endl;
				break;
			}
			cout << "请输入想要读取文件的索引号" << endl;
			scanf("%d", &index);
			cout << "请输入想要读取文件的长度" << endl;
			scanf("%d", &count);
			if (read(index, 0, count) == OK)
				cout << "读入操作成功" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		case WRITE:
			if (show_openlist() == 0)
			{
				cout << "没有文件被打开" << endl;
				cout << "----------------------------------------------" << endl;
				break;
			}
			cout << "请输入想要写入文件的索引号" << endl;
			scanf("%d", &index);
			cout << "请输入想要写入文件的长度" << endl;
			scanf("%d", &count);
			if (write(index, 0, count) == OK)
				cout << "写入操作成功" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		case LSEEK:
			if (show_openlist() == 0)
			{
				cout << "没有文件被打开" << endl;
				cout << "----------------------------------------------" << endl;
				break;
			}
			cout << "请输入想要写入文件的索引号" << endl;
			scanf("%d", &index);
			cout << "请输入想要设置的文件的相对位置" << endl;
			scanf("%d", &pos);
			lseek(index, pos);
			cout << "----------------------------------------------" << endl;
			break;
		case OPLIST:
			if (show_openlist() == 0)
			{
				cout << "没有文件被打开" << endl;
				cout << "----------------------------------------------" << endl;
				break;
			}
			cout << "----------------------------------------------" << endl;
			break;
		case HELP:
			show_help();
			break;
		case LDISK:
			show_ldisk();
			break;
		default:
			cout << "指令错误" << endl;
			cout << "----------------------------------------------" << endl;
			break;
		}
		fflush(stdin);
		Init_block(ch, CH_LENGTH);
	}

}