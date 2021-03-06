#include "fileSystem.h"
//TODO：检验文件名是否合格
//TODO:如何保证有些文件不能被删除
//TODO：修改变量名 eg:lastmodtime
//TODO:处理disk\\D盘
//TODO：实验报告体现自举难题
//TODO:父文件的更新时间

FileSystem::FileSystem()
{
	fileIndex.indexCnt = 0;
	curLevel = 0;
	curIndex = 0;
	tmpFileCnt = 0;

	if (load() == 0)	//若加载失败，则重新初始化
	{
		memset(bitmap, 0xff, sizeof(bitmap));
		createFile(writable, "root", dir, 0);
		curIndex = 0;
		createFile(writable, "C:", dir, 0);
		createFile(writable, "D:", dir, 0);

		//TODO:上帝视角嫌疑
		curIndex = 1;
		createFile(writable, "catalog.ini", file, 0);
		createFile(writable, "Users", dir, 0);
		curIndex = 4;
		createFile(writable, "susliks", dir, 0);
		curIndex = 0;
		setCurDirReadOnly();	//把root文件夹设置为只读
	}
}

void FileSystem::setCurDirReadOnly()
{
	fileIndex.index[curIndex].fcb->facc = readOnly;
}

void getCurTime(char* currtime) //获取当前时间
{
	char dbuffer[9];
	char tbuffer[9];
	_strdate(dbuffer);
	_strtime(tbuffer);
	strcpy(currtime, dbuffer);
	strcat(currtime, " ");
	strcat(currtime, tbuffer);
}

int FileSystem::findBlankBlockId()  //寻找空闲文件块
{
	int bitmapSize = (TotalBlockCnt + 7) / 8;
	for (int i = 0; i < bitmapSize; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if ((bitmap[i] & (1 << j)) != 0 && (i * 8 + j < TotalBlockCnt))
				return i * 8 + j;
		}
	}
	return -1;
}

int FileSystem::updateBitmap(int mode, int blockId)
{
	if (blockId >= TotalBlockCnt)
		return 0;
	if (mode == 1)	//对应位 置1
	{
		int loc = blockId / 8;
		int offset = blockId % 8;
		bitmap[loc] |= (1 << offset);
	}
	else if (mode == 0)	//对应位 置0
	{
		int loc = blockId / 8;
		int offset = blockId % 8;
		bitmap[loc] &= ~(1 << offset);
	}
	return 1;
}

string FileSystem::getCurPath()
{
	return getPath(curIndex);
}

string FileSystem::getPath(int index)
{
	string path = "";
	int id = index;
	while (true)
	{
		if(path.length() == 0)
			path = fileIndex.index[id].fileName;
		else
			path = fileIndex.index[id].fileName + '\\' + path;
		
		if (fileIndex.index[id].fileName != "root")
			id = fileIndex.index[id].parentId;
		else
			break;
	}
	return path;
}

int FileSystem::createFileWithPath(FileAccess acc, string fileName, FileType type, int filesize, string path)
{
	string pattern = "\\";
	vector<string> cdPath = split(path, pattern);
	int pathLen = cdPath.size();
	bool flag = true;
	int newCurIndex = curIndex;
	for (int i = 0; i < pathLen && flag; i++)
	{
		if (i == pathLen - 1 && cdPath[i].length() == 0)
			break;
			

		if (cdPath[i] == "..")
		{
			if (fileIndex.index[newCurIndex].fileName == "root")
			{
				flag = false;
			}
			else
			{
				newCurIndex = fileIndex.index[newCurIndex].parentId;
			}
			continue;
		}

		int found = -1;
		int childCnt = fileIndex.index[newCurIndex].child.size();
		for (int j = 0; j < childCnt && found == -1; j++)
		{
			int childId = fileIndex.index[newCurIndex].child[j];
			if (stringAreEqual(fileIndex.index[childId].fileName, cdPath[i]))
				found = j;
		}

		if (found == -1 && i == 0)	//如果前面两种方式都找不到 则到根目录找
		{
			newCurIndex = 0;

			int childCnt = fileIndex.index[newCurIndex].child.size();
			for (int j = 0; j < childCnt && found == -1; j++)
			{
				int childId = fileIndex.index[newCurIndex].child[j];
				cout << fileIndex.index[childId].fileName.c_str() << endl;
				cout << cdPath[i].c_str() << endl;
				if (stringAreEqual(fileIndex.index[childId].fileName, cdPath[i]))
					found = j;
			}
		}

		if (found == -1)
			flag = false;
		else
			newCurIndex = fileIndex.index[newCurIndex].child[found];
	}

	if (flag)
	{
		swap(curIndex, newCurIndex);
		if (createFile(acc, fileName, type, filesize))
		{
			swap(curIndex, newCurIndex);
			return 1;
		}
		swap(curIndex, newCurIndex);
		return 0;
	}
		
	else
	{
		printf("系统找不到指定路径\n");
		return 0;
	}
		

}

//规定文件名仅可由大小写英文字母、数字、下划线和冒号组成
bool FileSystem::fileNameIsLegal(string fileName)
{
	int len = fileName.length();
	bool flag = true;
	for (int i = 0; i < len && flag; i++)
	{
		char tmp = fileName[i];
		if (!(('0' <= tmp && tmp <= '9') || ('a' <= tmp && tmp <= 'z')
			|| ('A' <= tmp && tmp <= 'Z') || tmp == '_' || tmp == ':'
			|| tmp == '.'))
			flag = false;
	}
	return flag;
}

int FileSystem::createFile(FileAccess acc, string fileName, FileType type, int filesize)
{
	//应该先创建文件 再添加到目录中 以应对创建失败的情况
	int block[50];  //用来存放分配的文件块列表
	FCB* filecb;
	memset(block, 0, sizeof(block));  //初始化为0

	if (fileNameIsLegal(fileName) == false)
	{
		printf("文件名不合法\n");
		return 0;
	}


	//验证同一路径下是否有重名文件
	if (fileName != "root")
	{
		vector<int> tmp = fileIndex.index[curIndex].child;
		int size = tmp.size();
		for (int i = 0; i < size; i++)
		{
			if (stringAreEqual(fileIndex.index[tmp[i]].fileName, fileName))
			{
				printf("同一路径下有重名文件\n");
				return 0;
			}
		}
	}

	//TODO:什么意思
	if (fileName == "root" || fileIndex.index[curIndex].fcb->facc != readOnly)
	{

		filecb = new FCB;
		filecb->fname = fileName;

		if (fileName == "root")  //如果是根目录
		{
			filecb->flevel = 0;
		}
		else   //普通文件
		{
			filecb->flevel = curLevel+1;
		}

		filecb->ftype = type;
		filecb->facc = acc;
		filecb->fstate = closed;
		filecb->feffect = 1;
		

		getCurTime(filecb->createtime);
		strcpy(filecb->lastmodtime, filecb->createtime);

		if (filecb->ftype == file)
		{

			filecb->flen = filesize;
			int blockCnt = (filesize + 1023) / 1024;   //计算文件所需物理块

			for (int i = 0; i < blockCnt; i++)  //依次为文件分配物理块
			{
				//TODO:分配失败的错误处理
				block[i] = findBlankBlockId();
				updateBitmap(0, block[i]);
				filecb->blockId.push_back(block[i]);
			}
		}
		else if(filecb->ftype == dir)
		{
			filecb->flen = 0;
			//memset(filecb->blockId, 0, sizeof(filecb->blockId));
			//CS.filelevel++;
		}

		//TODO:添加失败的错误处理
		addIndex(fileName, (FileType)type, filecb);
		
	}
	else
	{
		printf("该目录为只读目录，不可在此创建文件！\n");
	}

	//todo
	return 1;
}

int FileSystem::addIndex(string fileName, FileType type, FCB* fcb)
{
	if (!(fileIndex.indexCnt < MaxIndexLen))
		return 0;

	//设置基础信息
	fileIndex.index[fileIndex.indexCnt].id = fileIndex.indexCnt;
	fileIndex.index[fileIndex.indexCnt].fileName = fileName;
	if (fileName != "root")
		fileIndex.index[fileIndex.indexCnt].parentId = curIndex;
	else
		fileIndex.index[fileIndex.indexCnt].parentId = -1;
	fileIndex.index[fileIndex.indexCnt].effect = 1;
	fileIndex.index[fileIndex.indexCnt].fileType = type;
	fileIndex.index[fileIndex.indexCnt].tmpFileId = -1;
	fileIndex.index[fileIndex.indexCnt].fcb = fcb;
	//memset(fileIndex.index[fileIndex.indexCnt].child, 0, sizeof(fileIndex.index[fileIndex.indexCnt].child));
	//fileIndex.index[fileIndex.indexCnt].childnum = 0;

	//更新父母节点
	if (fileName != "root")
		//fileIndex.index[curIndex].child[fileIndex.index[curIndex].childnum++] = fileIndex.indexCnt;
		fileIndex.index[curIndex].child.push_back(fileIndex.indexCnt);
	fileIndex.indexCnt++;
	return 1;
}

vector<string> split(string str, string pattern)	//	把string以特定分隔符作切分
{
	string::size_type pos;
	vector<string> result;
	str += pattern;//扩展字符串以方便操作  
	int size = str.size();

	for (int i = 0; i<size; i++)
	{
		pos = str.find(pattern, i);
		if (pos<size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

bool stringAreEqual(string a, string b)
{
	int alen = a.length();
	int blen = b.length();
	if (alen != blen)
		return false;
	for (int i = 0; i < alen; i++)
	{
		char atmp = tolower(a[i]);
		char btmp = tolower(b[i]);
		if (atmp != btmp)
			return false;
	}
	return true;
}


int FileSystem::op_cd()
{
	char inputBuf[MaxCharLen];
	int len = 0;
	while (true)	//跳过空行
	{
		fgets(inputBuf, MaxCharLen, stdin);
		len = strlen(inputBuf);
		inputBuf[len - 1] = '\0';
		if (len > 0)
			break;
	}
	string tmpCdPath = inputBuf;

	int newCurIndex = getFileIndex(tmpCdPath);
	if (newCurIndex != -1 && fileIndex.index[newCurIndex].fileType == dir)
	{
		curIndex = newCurIndex;
		return 1;
	}
	else
	{
		printf("系统找不到指定路径\n");
		return 0;
	}
		
	
}

int FileSystem::op_dir()
{
	FileIndexElement cur = fileIndex.index[curIndex];
	int dirCnt = 0;
	int fileCnt = 0;
	int childCnt = cur.child.size();
	for (int i = 0; i < childCnt; i++)
	{
		FileIndexElement child = fileIndex.index[cur.child[i]];
		printf("%-20s", child.fcb->lastmodtime);
		if (child.fileType == file)
		{
			printf("<file>%10d ", child.fcb->flen);
			fileCnt++;
		}
		else if (child.fileType == 1)	//也就是dir
		{
			printf(" <dir>           ");
			dirCnt++;
		}
		printf("%s\n", child.fileName.c_str());
	}
	printf("%d个文件\n", fileCnt);
	printf("%d个目录\n", dirCnt);
	return 1;
}

int FileSystem::op_mkdir()	//路径可选 模式可选
{
	char inputBuf[MaxCharLen];
	fgets(inputBuf, MaxCharLen, stdin);
	stringstream ss = stringstream(inputBuf);
	string path;
	string fileName;
	string mode;
	ss >> path;
	if (path.length() == 0)
		return 0;

	FileAccess acc = writable;
	if (ss >> fileName)
	{
		if (fileName == "-r" || fileName == "-w")
		{
			mode == fileName;
			fileName = path;
			if (mode == "-r")
				acc = readOnly;
			createFile(acc, fileName, dir, 0);
		}

		else if (ss >> mode)
		{
			if (mode == "-r")
				acc = readOnly;
			else if (mode == "-w")
				acc = writable;
			else
			{
				printf("模式选择错误");
				return 0;
			}	
		}
		createFileWithPath(acc, fileName, dir, 0, path);
		return 1;
	}
	else	//在当前路径下建文件夹
	{
		fileName = path;
		createFile(acc, fileName, dir, 0);
		return 1;
	}
}

int FileSystem::op_mkfile()	//路径可选 模式可选
{
	char inputBuf[MaxCharLen];
	fgets(inputBuf, MaxCharLen, stdin);
	stringstream ss = stringstream(inputBuf);
	string path;
	string fileName;
	string mode;
	ss >> path;
	if (path.length() == 0)
		return 0;

	FileAccess acc = writable;
	if (ss >> fileName)
	{
		if (fileName == "-r" || fileName == "-w")
		{
			mode == fileName;
			fileName = path;
			if (mode == "-r")
				acc = readOnly;
			createFile(acc, fileName, file, 0);
		}

		else if (ss >> mode)
		{
			if (mode == "-r")
				acc = readOnly;
			else if (mode == "-w")
				acc = writable;
			else
			{
				printf("模式选择错误");
				return 0;
			}
			createFileWithPath(acc, fileName, file, 0, path);
			return 1;
		}
	}
	else	//在当前路径下建文件夹
	{
		fileName = path;
		return createFile(acc, fileName, file, 0);	
	}
	return 0;
}

//TODO:不搜索已被删除的文件 即effect = 0;
int FileSystem::getFileIndex(string path)	//搜索失败返回-1
{
	
	int tmp_loc = 0;
	while (tmp_loc < path.length() && (path[tmp_loc] == ' ' || path[tmp_loc] == '\t'))	//忽略掉最前的whitespace
		tmp_loc++;
	path = path.substr(tmp_loc, path.length());

	//特判路径为空的情况
	if (path.length() == 0)
		return -1;

	string pattern = "\\";
	vector<string> cdPath = split(path, pattern);
	int pathLen = cdPath.size();
	bool flag = true;
	int newCurIndex = curIndex;

	for (int i = 0; i < pathLen && flag; i++)
	{
		if (i == pathLen - 1 && cdPath[i].length() == 0)
			break;

		if (cdPath[i] == "..")
		{
			if (fileIndex.index[newCurIndex].fileName == "root")
			{
				flag = false;
			}
			else
			{
				newCurIndex = fileIndex.index[newCurIndex].parentId;
			}
			continue;
		}

		int found = -1;
		int childCnt = fileIndex.index[newCurIndex].child.size();
		for (int j = 0; j < childCnt && found == -1; j++)
		{
			int childId = fileIndex.index[newCurIndex].child[j];
			if (stringAreEqual(fileIndex.index[childId].fileName, cdPath[i]))
				found = j;
		}

		if (found == -1 && i == 0)	//如果前面两种方式都找不到 则到根目录找
		{
			newCurIndex = 0;

			int childCnt = fileIndex.index[newCurIndex].child.size();
			for (int j = 0; j < childCnt && found == -1; j++)
			{
				int childId = fileIndex.index[newCurIndex].child[j];
				if (stringAreEqual(fileIndex.index[childId].fileName, cdPath[i]))
					found = j;
			}
		}

		if (found == -1)
			flag = false;
		else
			newCurIndex = fileIndex.index[newCurIndex].child[found];

		//判断搜索到的目录是否已经被删除过 即effect是否为0
		if (fileIndex.index[newCurIndex].effect == 0)
			flag = false;

		//判断路径上的每一个非最终节点是否均为文件夹dir类型
		if (fileIndex.index[newCurIndex].fileType != dir && i != pathLen-1)
			flag = false;
	}

	if (flag)
		return newCurIndex;
	else
		return -1;
}

int FileSystem::rm(int dirIndex)	//返回0表示目录不是空的
{
	if (fileIndex.index[dirIndex].fileType == dir)
	{
		if (fileIndex.index[dirIndex].child.size() > 0)	//若目录不是空的则返回
			return 0;

		//删除父母节点中自己的信息
		int parentId = fileIndex.index[dirIndex].parentId;
		int size = fileIndex.index[parentId].child.size();
		for (int i = 0; i < size; i++)
		{
			int tmpId = fileIndex.index[parentId].child[i];
			if (tmpId == dirIndex)
			{
				vector<int>::iterator it = fileIndex.index[parentId].child.begin() + i;
				fileIndex.index[parentId].child.erase(it);
				break;
			}
		}

		//把自己置为无效
		fileIndex.index[dirIndex].effect = 0;
		return 1;
	}

	else if (fileIndex.index[dirIndex].fileType == file)
	{
		//删除父母节点中自己的信息
		int parentId = fileIndex.index[dirIndex].parentId;
		int size = fileIndex.index[parentId].child.size();
		for (int i = 0; i < size; i++)
		{
			int tmpId = fileIndex.index[parentId].child[i];
			if (tmpId = dirIndex)
			{
				vector<int>::iterator it = fileIndex.index[parentId].child.begin() + i;
				fileIndex.index[parentId].child.erase(it);
				break;
			}
		}

		//更新位图
		vector<int> tmpBlockId = fileIndex.index[dirIndex].fcb->blockId;
		size = tmpBlockId.size();
		for (int i = 0; i < size; i++)
		{
			updateBitmap(1, tmpBlockId[i]);	//把该文件占用块在位图中置1 表明可以被重新使用
		}

		//把自己置为无效
		fileIndex.index[dirIndex].effect = 0;
		return 1;
	}
	return 0;
}

int FileSystem::recursiveRm(int dirIndex)	
{
	if (fileIndex.index[dirIndex].fileType == dir)
	{
		int childSize = fileIndex.index[dirIndex].child.size();

		//若有子目录 则先递归删除子目录
		if (childSize > 0)
			for (int i = 0; i < childSize; i++)
				recursiveRm(fileIndex.index[dirIndex].child[i]);
		
		//删除自己
		rm(dirIndex);
		return 1;
	}

	else if (fileIndex.index[dirIndex].fileType == file)
	{
		return rm(dirIndex);
	}
	return 0;
}

int FileSystem::op_rmdir()
{
	char inputBuf[MaxCharLen];
	fgets(inputBuf, MaxCharLen, stdin);
	stringstream ss = stringstream(inputBuf);
	string path;
	ss >> path;
	if (path == "/s")	//递归删除文件夹
	{
		string mode = path;
		ss >> path;
		int dirIndex = getFileIndex(path);
		if (dirIndex == -1)
		{
			printf("路径不存在\n");
			return 0;
		}
		if (fileIndex.index[dirIndex].fileType != dir)	//判断目标类型是否为dir
		{
			printf("目标类型不匹配\n");
			return 0;
		}
		if (dirIndex < DiskCnt + 1)	//根目录和各大硬盘不可删除
		{
			printf("不可删除的目标\n");
			return 0;
		}

		if (!recursiveRm(dirIndex))
		{
			printf("递归删除文件夹失败\n");
			return 0;
		}

		return 1;
	}
	
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("路径不存在\n");
		return 0;
	}
	if (fileIndex.index[dirIndex].fileType != dir)	//判断目标类型是否为dir
	{
		printf("目标类型不匹配\n");
		return 0;
	}
	if (dirIndex < DiskCnt + 1)	//根目录和各大硬盘不可删除
	{
		printf("不可删除的目标\n");
		return 0;
	}
	if (rm(dirIndex) == 0)
	{
		printf("目录不是空的\n");
		return 0;
	}
	return 1;
}

int FileSystem::op_delfile()
{
	char inputBuf[MaxCharLen];
	fgets(inputBuf, MaxCharLen, stdin);
	stringstream ss = stringstream(inputBuf);
	string path;
	ss >> path;

	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("路径不存在\n");
		return 0;
	}
	if (fileIndex.index[dirIndex].fileType != file)	//判断目标类型是否为file
	{
		printf("目标类型不匹配\n");
		return 0;
	}
	rm(dirIndex);
	return 1;
}

int FileSystem::save()
{
	//存储boost信息
	//先存储操作系统基本信息
	FILE *fout = fopen("disk\\c\\0.txt", "w");
	updateBitmap(0, 0);
	int bitmapSize = (TotalBlockCnt + 7) / 8;
	for (int i = 0; i < bitmapSize; i++)
		fprintf(fout, "%c", bitmap[i]);
	fprintf(fout, "%d ", fileIndex.indexCnt);
	for (int i = 0; i < DiskCnt+2; i++)
	{
		//存index头
		fprintf(fout, "%d %s %d ", fileIndex.index[i].id,
			fileIndex.index[i].fileName.c_str(), fileIndex.index[i].parentId);
		fprintf(fout, "%d ", fileIndex.index[i].child.size());
		for (int j = 0; j < fileIndex.index[i].child.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].child[j]);
		fprintf(fout, "%d %d ", fileIndex.index[i].effect, fileIndex.index[i].fileType);

		//存fcb
		fprintf(fout, "%d %s ", fileIndex.index[i].fcb->flevel, fileIndex.index[i].fcb->fname.c_str());
		fprintf(fout, "%d ", fileIndex.index[i].fcb->blockId.size());
		for (int j = 0; j < fileIndex.index[i].fcb->blockId.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].fcb->blockId[j]);
		fprintf(fout, "%d %d %d %d ", fileIndex.index[i].fcb->flen, fileIndex.index[i].fcb->ftype,
			fileIndex.index[i].fcb->facc, fileIndex.index[i].fcb->feffect);
		fprintf(fout, "%-20s %-20s ", fileIndex.index[i].fcb->createtime, fileIndex.index[i].fcb->lastmodtime);
		fprintf(fout, "%d ", fileIndex.index[i].fcb->fstate);
	}
	fclose(fout);

	
	//再把其他信息存储到初始化配置文件
	string path = "c:\\catalog.ini";
	int tmpFileId = openFile(path);
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	fout = fopen(filenameBuf, "w");
	
	for (int i = DiskCnt + 2; i < fileIndex.indexCnt; i++)
	{
		//存index头
		fprintf(fout, "%d %s %d ", fileIndex.index[i].id, 
			fileIndex.index[i].fileName.c_str(), fileIndex.index[i].parentId);
		fprintf(fout, "%d ", fileIndex.index[i].child.size());
		for (int j = 0; j < fileIndex.index[i].child.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].child[j]);
		fprintf(fout, "%d %d ", fileIndex.index[i].effect, fileIndex.index[i].fileType);
	
		//存fcb
		fprintf(fout, "%d %s ", fileIndex.index[i].fcb->flevel, fileIndex.index[i].fcb->fname.c_str());
		fprintf(fout, "%d ", fileIndex.index[i].fcb->blockId.size());
		for (int j = 0; j < fileIndex.index[i].fcb->blockId.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].fcb->blockId[j]);
		fprintf(fout, "%d %d %d %d ", fileIndex.index[i].fcb->flen, fileIndex.index[i].fcb->ftype, 
			fileIndex.index[i].fcb->facc, fileIndex.index[i].fcb->feffect);
		fprintf(fout, "%-20s %-20s ", fileIndex.index[i].fcb->createtime, fileIndex.index[i].fcb->lastmodtime);
		fprintf(fout, "%d ", fileIndex.index[i].fcb->fstate);
	}

	fclose(fout);
	path = "c:\\catalog.ini";
	writeFile(path);
	closeFile(path);

	return 1;
}

int FileSystem::boost()
{
	FILE *fin = fopen("disk\\c\\0.txt", "r");
	if (fin == NULL)
		return 0;

	//先读取操作系统基本信息
	int bitmapSize = (TotalBlockCnt + 7) / 8;
	for (int i = 0; i < bitmapSize; i++)
		if (fscanf(fin, "%c", &bitmap[i]) == 0)
			return 0;

	//读取操作系统索引结构
	char inputBuf[MaxCharLen];
	int tmpSize, intBuf;
	fscanf(fin, "%d", &fileIndex.indexCnt);
	for (int i = 0; i < DiskCnt+2; i++)
	{
		fscanf(fin, "%d%s%d", &fileIndex.index[i].id,
			inputBuf, &fileIndex.index[i].parentId);
		fileIndex.index[i].fileName = inputBuf;
		fscanf(fin, "%d", &tmpSize);
		for (int j = 0; j < tmpSize; j++)
		{
			fscanf(fin, "%d", &intBuf);
			fileIndex.index[i].child.push_back(intBuf);
		}
		fscanf(fin, "%d%d", &fileIndex.index[i].effect, &fileIndex.index[i].fileType);

		fileIndex.index[i].fcb = new FCB;
		fscanf(fin, "%d%s", &fileIndex.index[i].fcb->flevel, inputBuf);
		fileIndex.index[i].fcb->fname = inputBuf;
		fscanf(fin, "%d", &tmpSize);
		for (int j = 0; j < tmpSize; j++)
		{
			fscanf(fin, "%d", &intBuf);
			fileIndex.index[i].fcb->blockId.push_back(intBuf);
		}
		fscanf(fin, "%d%d%d%d", &fileIndex.index[i].fcb->flen, &fileIndex.index[i].fcb->ftype,
			&fileIndex.index[i].fcb->facc, &fileIndex.index[i].fcb->feffect);
		fscanf(fin, "%c", &inputBuf[0]);
		for (int j = 0; j < 20; j++)
			fscanf(fin, "%c", &fileIndex.index[i].fcb->createtime[j]);
		fileIndex.index[i].fcb->createtime[19] = '\0';
		fscanf(fin, "%c", &inputBuf[0]);
		for (int j = 0; j < 20; j++)
			fscanf(fin, "%c", &fileIndex.index[i].fcb->lastmodtime[j]);
		fileIndex.index[i].fcb->lastmodtime[19] = '\0';
		fscanf(fin, "%d", &fileIndex.index[i].fcb->fstate);
	}

	//解决目录文件的自举难题
	int blockCnt;
	fscanf(fin, "%d%d", &fileIndex.index[DiskCnt + 1].fcb->flen, &blockCnt);
	for (int i = 0; i < blockCnt; i++)
	{
		int tmp;
		fscanf(fin, "%d", &tmp);
		fileIndex.index[DiskCnt + 1].fcb->blockId.push_back(tmp);
	}


	fclose(fin);
	return 1;
}

int FileSystem::load()
{
	//先加载磁盘引导块；
	if (boost() == false)
		return 0;

	string path = "c:\\catalog.ini";
	int tmpFileId = openFile(path);
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	FILE *fin = fopen(filenameBuf, "r");

	if (fin == NULL)
		return 0;

	char inputBuf[MaxCharLen];
	int tmpSize, intBuf;
	
	for (int i = DiskCnt+2; i < fileIndex.indexCnt; i++)
	{
		fscanf(fin, "%d%s%d", &fileIndex.index[i].id,
			inputBuf, &fileIndex.index[i].parentId);
		fileIndex.index[i].fileName = inputBuf;
		fscanf(fin, "%d", &tmpSize);
		for (int j = 0; j < tmpSize; j++)
		{
			fscanf(fin, "%d", &intBuf);
			fileIndex.index[i].child.push_back(intBuf);
		}
		fscanf(fin, "%d%d", &fileIndex.index[i].effect, &fileIndex.index[i].fileType);

		fileIndex.index[i].fcb = new FCB;
		fscanf(fin, "%d%s", &fileIndex.index[i].fcb->flevel, inputBuf);
		fileIndex.index[i].fcb->fname = inputBuf;
		fscanf(fin, "%d", &tmpSize);
		for (int j = 0; j < tmpSize; j++)
		{
			fscanf(fin, "%d", &intBuf);
			fileIndex.index[i].fcb->blockId.push_back(intBuf);
		}
		fscanf(fin, "%d%d%d%d", &fileIndex.index[i].fcb->flen, &fileIndex.index[i].fcb->ftype,
			&fileIndex.index[i].fcb->facc, &fileIndex.index[i].fcb->feffect);
		fscanf(fin, "%c", &inputBuf[0]);
		for (int j = 0; j < 20; j++)
			fscanf(fin, "%c", &fileIndex.index[i].fcb->createtime[j]);
		fileIndex.index[i].fcb->createtime[19] = '\0';
		fscanf(fin, "%c", &inputBuf[0]);
		for (int j = 0; j < 20; j++)
			fscanf(fin, "%c", &fileIndex.index[i].fcb->lastmodtime[j]);
		fileIndex.index[i].fcb->lastmodtime[19] = '\0';
		fscanf(fin, "%d", &fileIndex.index[i].fcb->fstate);
	}	

	fclose(fin);
	path = "c:\\catalog.ini";
	closeFile(path);

	return 1;
}

int FileSystem::openFile(string path)	//打开失败返回-1
										//打开成功时返回临时文件编号
{
	//获得“目标文件”的目录序号
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("路径不存在\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("目标不是一个文件\n");
		return -1;
	}

	char filenameBuf[MaxCharLen];
	int tmpFileId = tmpFileCnt++;
	fileIndex.index[dirIndex].tmpFileId = tmpFileId;
	fileIndex.index[dirIndex].fcb->fstate = opened;
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	FILE* fout = fopen(filenameBuf, "w");
	int blockCnt = fileIndex.index[dirIndex].fcb->blockId.size();
	char *c_dir = "disk\\c";
	for (int i = 0; i < blockCnt; i++)
	{
		sprintf(filenameBuf, "%s\\%d.txt", c_dir, fileIndex.index[dirIndex].fcb->blockId[i]);
		FILE *fin = fopen(filenameBuf, "r");
		char tmpChar;
		while (fscanf(fin, "%c", &tmpChar) == 1)
		{
			fprintf(fout, "%c", tmpChar);
		}
			
		fclose(fin);
	}
	fclose(fout);
	return tmpFileId;
}

int FileSystem::writeFile(string path)
{
	//获得“目标文件”的目录序号
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("目标路径不存在\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("目标不是一个文件\n");
		return -1;
	}

	//打开存储“待写入内容”的文件
	int tmpFileId = fileIndex.index[dirIndex].tmpFileId;
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	FILE *fin = fopen(filenameBuf, "r");
	if (fin == NULL)
	{
		printf("无效的写入内容\n");
		return -1;
	}

	//统计“待写入内容”的大小
	int contentLen = 0;
	char tmpChar;
	while (fscanf(fin, "%c", &tmpChar) == 1)
		contentLen++;
	fclose(fin);
	fin = fopen(filenameBuf, "r");

	//清空原文件所占用的block
	int blockCnt = fileIndex.index[dirIndex].fcb->blockId.size();
	for (int i = 0; i < blockCnt; i++)
	{
		updateBitmap(1, fileIndex.index[dirIndex].fcb->blockId[i]);
	}
	fileIndex.index[dirIndex].fcb->blockId.clear();

	//修改“目标文件”状态
	fileIndex.index[dirIndex].fcb->fstate = writing;
	fileIndex.index[dirIndex].fcb->flen = contentLen;
	getCurTime(fileIndex.index[dirIndex].fcb->lastmodtime);

	//存内容
	blockCnt = (contentLen + BlockSize - 1) / BlockSize;
	int blockLoc = 0;
	FILE *fout = NULL;
	for (int i = 0; i < contentLen; i++)
	{
		char *c_dir = "disk\\c";
		if (i % BlockSize == 0)
		{	
			//TODO:如果分配失败
			fileIndex.index[dirIndex].fcb->blockId.push_back(findBlankBlockId());
			sprintf(filenameBuf, "%s\\%d.txt", c_dir, fileIndex.index[dirIndex].fcb->blockId[blockLoc]);
			blockLoc++;
			fout = fopen(filenameBuf, "w");
		}
		if (fout != NULL)
		{
			fscanf(fin, "%c", &tmpChar);
			fprintf(fout, "%c", tmpChar);
		}
		if ((i + 1) % BlockSize == 0 || (i + 1) == contentLen)
			fclose(fout);
	}
	fclose(fin);

	//解决目录文件的自举难题
	if (dirIndex == 3)
	{
		FILE *fout2 = fopen("disk\\c\\0.txt", "a");
		int blockCnt = fileIndex.index[dirIndex].fcb->blockId.size();
		fprintf(fout2, "%d %d ", contentLen, blockCnt);
		for (int i = 0; i < blockCnt; i++)
			fprintf(fout2, "%d ", fileIndex.index[dirIndex].fcb->blockId[i]);
		fclose(fout2);
	}


	//还原“目标文件”状态
	fileIndex.index[dirIndex].fcb->fstate = opened;
	return 1;
}

int FileSystem::closeFile(string path)
{
	//获得“目标文件”的目录序号
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("目标路径不存在\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("目标不是一个文件\n");
		return -1;
	}

	if (fileIndex.index[dirIndex].fcb->fstate == closed)
	{
		printf("目标文件未打开\n");
		return -1;
	}

	int tmpFileId = fileIndex.index[dirIndex].tmpFileId;
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	remove(filenameBuf);	//移除临时文件
	fileIndex.index[dirIndex].tmpFileId = -1;
	fileIndex.index[dirIndex].fcb->fstate = closed;
	return 1;
}