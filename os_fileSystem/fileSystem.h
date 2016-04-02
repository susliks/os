#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <sstream>

using namespace std;

const int TotalBlockCnt = 64;
const int BlockSize = 1024;
const int MaxCharLen = 200;
const int MaxChildNum = 20;
const int MaxIndexLen = 1000;
const int DiskCnt = 2;

typedef enum
{
	readOnly,   //ֻ��
	writable    //��д
}FileAccess;

typedef enum
{
	file,   //�ļ�
	dir   //Ŀ¼
}FileType;

typedef enum
{
	closed,
	opened,
	writing
}FileState;

typedef struct fse
{
	int flevel;  //�ļ����ڵĲ�Σ���+�ļ�Ԫ����Ϊһ���ļ����߼�λ��
	string fname;  //�ļ���
				   //char parent[20];  //���ļ���
	vector<int> blockId;  //�ļ����ڵ��������
	int flen;  //�ļ�����
	FileType ftype;  //�ļ�����
	FileAccess facc;  //�ļ�Ȩ��
	int feffect;  //�����ļ��Ƿ�ɾ����0��ʾ��Ч�ļ���1��ʾ��Ч
	char createtime[20]; //����ʱ��
	char lastmodtime[20]; //���һ���޸�ʱ��
	FileState fstate;  //�ļ���ǰ״̬
					   //char child[10][20];  //������ļ�ΪĿ¼���������д�����̵����ļ����������ͨ�ļ����������Ϊ��

}FCB;

//һ���ļ������Ľṹ
typedef struct
{
	int id;  //�������
	string fileName;   //�ļ���
	int parentId;
	vector<int> child;
	//int childnum;  //���ļ���Ŀ
	//char parentfilename[20];   //���ڵ��ļ���
	//int filelevel;   //�ļ����ڲ㼶
	int effect;  //�ļ���Ч��
	int fileType;   //�ļ�����
	int tmpFileId;
	FCB* fcb;   //ָ�������FCB
} FileIndexElement;

typedef struct
{
	FileIndexElement index[MaxIndexLen];  //��������
	int indexCnt;   //���е���������
} FileIndex;

//�ļ�ϵͳ�е�Ԫ�ؽṹ�������ļ���Ŀ¼


class FileSystem
{
private:
	FileIndex fileIndex;
	int curIndex;	//��ǰĿ¼
	int curLevel;
	unsigned char bitmap[(TotalBlockCnt + 7) / 8]; //1��ʾ���� 0��ʾ��ռ��
	int tmpFileCnt;
private:
	int updateBitmap(int mode, int blockId);
	int findBlankBlockId();
	string getPath(int index);
	void setCurDirReadOnly();
	int rm(int dirIndex);
	int recursiveRm(int dirIndex);
	int getFileIndex(string path);
	int load();
	bool fileNameIsLegal(string fileName);
	int openFile(string path);
	int writeFile(string path);
	int closeFile(string path);
	//TODO����ʵ�鱨���н���Ϊʲô��Ҫboost 
	//		Ҫ�㣺boost�鳤�ȹ̶���ϵͳ��Ҫ�ȼ��ػ�����Ϣ��������Ϣ�洢���ļ��С�������Ҫ���������齨��������Ϣ�洢�ļ���FCB
	int boost();
	

public:
	FileSystem();
	int createFile(FileAccess acc, string filename, FileType type, int filesize);
	int createFileWithPath(FileAccess acc, string filename, FileType type, int filesize, string path);
	int addIndex(string fileName, FileType type, FCB* fcb);
	string getCurPath();
	int op_cd();
	int op_dir();
	int op_mkdir();
	int op_mkfile();
	int op_rmdir();
	int op_delfile();
	/*int op_open();
	int op_close();
	int op_write();*/
	int save();
	
};

vector<string> split(string str, string pattern);
bool stringAreEqual(string a, string b);

