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
	readOnly,   //只读
	writable    //可写
}FileAccess;

typedef enum
{
	file,   //文件
	dir   //目录
}FileType;

typedef enum
{
	closed,
	opened,
	writing
}FileState;

typedef struct fse
{
	int flevel;  //文件所在的层次，层+文件元素名为一个文件的逻辑位置
	string fname;  //文件名
				   //char parent[20];  //父文件名
	vector<int> blockId;  //文件所在的物理块编号
	int flen;  //文件长度
	FileType ftype;  //文件类型
	FileAccess facc;  //文件权限
	int feffect;  //表征文件是否被删除，0表示无效文件，1表示有效
	char createtime[20]; //创建时间
	char lastmodtime[20]; //最后一次修改时间
	FileState fstate;  //文件当前状态
					   //char child[10][20];  //如果该文件为目录，则数组中存放其后继的子文件；如果是普通文件，则该数组为空

}FCB;

//一个文件索引的结构
typedef struct
{
	int id;  //索引序号
	string fileName;   //文件名
	int parentId;
	vector<int> child;
	//int childnum;  //子文件数目
	//char parentfilename[20];   //父节点文件名
	//int filelevel;   //文件所在层级
	int effect;  //文件有效性
	int fileType;   //文件属性
	int tmpFileId;
	FCB* fcb;   //指向自身的FCB
} FileIndexElement;

typedef struct
{
	FileIndexElement index[MaxIndexLen];  //索引数组
	int indexCnt;   //现有的索引数量
} FileIndex;

//文件系统中的元素结构，包括文件和目录


class FileSystem
{
private:
	FileIndex fileIndex;
	int curIndex;	//当前目录
	int curLevel;
	unsigned char bitmap[(TotalBlockCnt + 7) / 8]; //1表示空闲 0表示被占用
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
	//TODO：在实验报告中解释为什么需要boost 
	//		要点：boost块长度固定、系统需要先加载基本信息、基本信息存储于文件中、所以需要先由引导块建立基本信息存储文件的FCB
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

