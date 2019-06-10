#pragma once
#define B		10			//�洢�鳤��
#define L		500			//�洢������
#define K		100			//��������С
#define BUSY	1
#define FREE	0
#define OK		1
#define ERROR	0
#define FILE_BLOCK_LENGTH		(B-3)				//�ļ�������̿�����鳤��
#define FILE_NAME_LENGTH		(B-1)				//��ļ�������
#define FILE_SIGN_AREA			((L-1-K)/B+1)		//���������ļ���ʶ����ʼ���(λͼ֮��)
#define FILE_NUM				FILE_BLOCK_LENGTH	//Ŀ¼������ļ���Ŀ
#define BUFFER_LENGTH			25					//���ļ���Ŀ�еĻ���������
#define INPUT_LENGTH			100					//д�ļ�ʱ������볤��
#define OUTPUT_LENGTH			100					//���ļ�ʱ����������



struct filesign {							//�ļ�������
	int file_length;						//�ļ�����
	int filesign_flag;						//ռ�ñ�ʶλ
	int file_block;							//�ļ�������̿������ʵ�ʳ���
	int file_block_ary[FILE_BLOCK_LENGTH];	//�ļ�������̿������
};

struct contents {							//Ŀ¼��
	char filename[FILE_NAME_LENGTH];		//�ļ���
	int	 filesignnum;						//�ļ����������
};

struct openfilelist {						//���ļ����Ŀ
	char buffer[BUFFER_LENGTH];				//��д������
	int pointer[2];							//��дָ��(�ļ����ݵ�λ��)
	int filesignnum;						//�ļ�������
	int flag;								//ռ�÷�
};

char ldisk[L][B];						//���ַ�����ģ�����

openfilelist open_list[FILE_NUM];		//���ļ���

int show_openlist();				//��ʾ���ļ���,���ش��ļ�����
void directory();					//��ʾĿ¼�ļ���ϸ��Ϣ
void show_help();					//���ļ�ϵͳ�İ���
void show_ldisk();					//��ʾ�������� 
void read_block(int, char *);		//�ļ�ϵͳ��IO�豸�Ľӿں�������ȡ��
void write_block(int, char *);		//�ļ�ϵͳ��IO�豸�Ľӿں�����д���
void Init();						//��ʼ���ļ�ϵͳ
int create(char *);					//�����ļ�
int destroy(char *);				//ɾ���ļ�
int open(char *);					//���ļ�
int close(int);						//�ر��ļ�
int read(int, int, int);				//���ļ�
int write(int, int, int);				//д�ļ�
int write_buffer(int, int);			//�ѻ���������д���ļ�
int lseek(int, int);					//��λ�ļ�ָ��
void Init_block(char, int);			//��ʼ���ַ������