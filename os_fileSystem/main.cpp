#include "fileSystem.h"



void init()
{
	char *dir = "disk";
	_mkdir(dir);
	for (int i = 0; i < TotalBlockCnt; i++)
	{
		char path[MaxCharLen];
		sprintf(path, "%s\\%d.txt", dir, i);
		FILE *fp = fopen(path, "r");
		if (!fp)
			fp = fopen(path, "w");
		fclose(fp);
	}
}

int main()
{
	init();//��ʼ������
	FileSystem *fs = new FileSystem;
	char inputBuf[MaxCharLen];

	while (true)
	{
		string command;
		printf("%s>", fs->getCurPath().c_str());
		cin >> inputBuf;
		command = inputBuf;
		if (command == "cd")
			fs->op_cd();
		else if (command == "dir")
			fs->op_dir();
		else if (command == "md" || command == "mkdir")
			fs->op_mkdir();
		else if (command == "mkfile")
			fs->op_mkfile();
		else if (command == "rd" || command == "rmdir")
			fs->op_rmdir();
		else if (command == "del")
			fs->op_delfile();
		else if (command == "exit")
			break;
		else
			fgets(inputBuf, MaxCharLen, stdin);	//���������
		printf("\n");
	}

	fs->save();
	delete fs;
	return 0;
}