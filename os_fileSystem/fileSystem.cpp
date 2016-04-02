#include "fileSystem.h"
//TODO�������ļ����Ƿ�ϸ�
//TODO:��α�֤��Щ�ļ����ܱ�ɾ��
//TODO���޸ı����� eg:lastmodtime
//TODO:����disk\\D��
//TODO��ʵ�鱨�������Ծ�����
//TODO:���ļ��ĸ���ʱ��

FileSystem::FileSystem()
{
	fileIndex.indexCnt = 0;
	curLevel = 0;
	curIndex = 0;
	tmpFileCnt = 0;

	if (load() == 0)	//������ʧ�ܣ������³�ʼ��
	{
		memset(bitmap, 0xff, sizeof(bitmap));
		createFile(writable, "root", dir, 0);
		curIndex = 0;
		createFile(writable, "C:", dir, 0);
		createFile(writable, "D:", dir, 0);

		//TODO:�ϵ��ӽ�����
		curIndex = 1;
		createFile(writable, "catalog.ini", file, 0);
		createFile(writable, "Users", dir, 0);
		curIndex = 4;
		createFile(writable, "susliks", dir, 0);
		curIndex = 0;
		setCurDirReadOnly();	//��root�ļ�������Ϊֻ��
	}
}

void FileSystem::setCurDirReadOnly()
{
	fileIndex.index[curIndex].fcb->facc = readOnly;
}

void getCurTime(char* currtime) //��ȡ��ǰʱ��
{
	char dbuffer[9];
	char tbuffer[9];
	_strdate(dbuffer);
	_strtime(tbuffer);
	strcpy(currtime, dbuffer);
	strcat(currtime, " ");
	strcat(currtime, tbuffer);
}

int FileSystem::findBlankBlockId()  //Ѱ�ҿ����ļ���
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
	if (mode == 1)	//��Ӧλ ��1
	{
		int loc = blockId / 8;
		int offset = blockId % 8;
		bitmap[loc] |= (1 << offset);
	}
	else if (mode == 0)	//��Ӧλ ��0
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

		if (found == -1 && i == 0)	//���ǰ�����ַ�ʽ���Ҳ��� �򵽸�Ŀ¼��
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
		printf("ϵͳ�Ҳ���ָ��·��\n");
		return 0;
	}
		

}

//�涨�ļ��������ɴ�СдӢ����ĸ�����֡��»��ߺ�ð�����
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
	//Ӧ���ȴ����ļ� ����ӵ�Ŀ¼�� ��Ӧ�Դ���ʧ�ܵ����
	int block[50];  //������ŷ�����ļ����б�
	FCB* filecb;
	memset(block, 0, sizeof(block));  //��ʼ��Ϊ0

	if (fileNameIsLegal(fileName) == false)
	{
		printf("�ļ������Ϸ�\n");
		return 0;
	}


	//��֤ͬһ·�����Ƿ��������ļ�
	if (fileName != "root")
	{
		vector<int> tmp = fileIndex.index[curIndex].child;
		int size = tmp.size();
		for (int i = 0; i < size; i++)
		{
			if (stringAreEqual(fileIndex.index[tmp[i]].fileName, fileName))
			{
				printf("ͬһ·�����������ļ�\n");
				return 0;
			}
		}
	}

	//TODO:ʲô��˼
	if (fileName == "root" || fileIndex.index[curIndex].fcb->facc != readOnly)
	{

		filecb = new FCB;
		filecb->fname = fileName;

		if (fileName == "root")  //����Ǹ�Ŀ¼
		{
			filecb->flevel = 0;
		}
		else   //��ͨ�ļ�
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
			int blockCnt = (filesize + 1023) / 1024;   //�����ļ����������

			for (int i = 0; i < blockCnt; i++)  //����Ϊ�ļ����������
			{
				//TODO:����ʧ�ܵĴ�����
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

		//TODO:���ʧ�ܵĴ�����
		addIndex(fileName, (FileType)type, filecb);
		
	}
	else
	{
		printf("��Ŀ¼Ϊֻ��Ŀ¼�������ڴ˴����ļ���\n");
	}

	//todo
	return 1;
}

int FileSystem::addIndex(string fileName, FileType type, FCB* fcb)
{
	if (!(fileIndex.indexCnt < MaxIndexLen))
		return 0;

	//���û�����Ϣ
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

	//���¸�ĸ�ڵ�
	if (fileName != "root")
		//fileIndex.index[curIndex].child[fileIndex.index[curIndex].childnum++] = fileIndex.indexCnt;
		fileIndex.index[curIndex].child.push_back(fileIndex.indexCnt);
	fileIndex.indexCnt++;
	return 1;
}

vector<string> split(string str, string pattern)	//	��string���ض��ָ������з�
{
	string::size_type pos;
	vector<string> result;
	str += pattern;//��չ�ַ����Է������  
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
	while (true)	//��������
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
		printf("ϵͳ�Ҳ���ָ��·��\n");
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
		else if (child.fileType == 1)	//Ҳ����dir
		{
			printf(" <dir>           ");
			dirCnt++;
		}
		printf("%s\n", child.fileName.c_str());
	}
	printf("%d���ļ�\n", fileCnt);
	printf("%d��Ŀ¼\n", dirCnt);
	return 1;
}

int FileSystem::op_mkdir()	//·����ѡ ģʽ��ѡ
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
				printf("ģʽѡ�����");
				return 0;
			}	
		}
		createFileWithPath(acc, fileName, dir, 0, path);
		return 1;
	}
	else	//�ڵ�ǰ·���½��ļ���
	{
		fileName = path;
		createFile(acc, fileName, dir, 0);
		return 1;
	}
}

int FileSystem::op_mkfile()	//·����ѡ ģʽ��ѡ
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
				printf("ģʽѡ�����");
				return 0;
			}
			createFileWithPath(acc, fileName, file, 0, path);
			return 1;
		}
	}
	else	//�ڵ�ǰ·���½��ļ���
	{
		fileName = path;
		return createFile(acc, fileName, file, 0);	
	}
	return 0;
}

//TODO:�������ѱ�ɾ�����ļ� ��effect = 0;
int FileSystem::getFileIndex(string path)	//����ʧ�ܷ���-1
{
	
	int tmp_loc = 0;
	while (tmp_loc < path.length() && (path[tmp_loc] == ' ' || path[tmp_loc] == '\t'))	//���Ե���ǰ��whitespace
		tmp_loc++;
	path = path.substr(tmp_loc, path.length());

	//����·��Ϊ�յ����
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

		if (found == -1 && i == 0)	//���ǰ�����ַ�ʽ���Ҳ��� �򵽸�Ŀ¼��
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

		//�ж���������Ŀ¼�Ƿ��Ѿ���ɾ���� ��effect�Ƿ�Ϊ0
		if (fileIndex.index[newCurIndex].effect == 0)
			flag = false;

		//�ж�·���ϵ�ÿһ�������սڵ��Ƿ��Ϊ�ļ���dir����
		if (fileIndex.index[newCurIndex].fileType != dir && i != pathLen-1)
			flag = false;
	}

	if (flag)
		return newCurIndex;
	else
		return -1;
}

int FileSystem::rm(int dirIndex)	//����0��ʾĿ¼���ǿյ�
{
	if (fileIndex.index[dirIndex].fileType == dir)
	{
		if (fileIndex.index[dirIndex].child.size() > 0)	//��Ŀ¼���ǿյ��򷵻�
			return 0;

		//ɾ����ĸ�ڵ����Լ�����Ϣ
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

		//���Լ���Ϊ��Ч
		fileIndex.index[dirIndex].effect = 0;
		return 1;
	}

	else if (fileIndex.index[dirIndex].fileType == file)
	{
		//ɾ����ĸ�ڵ����Լ�����Ϣ
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

		//����λͼ
		vector<int> tmpBlockId = fileIndex.index[dirIndex].fcb->blockId;
		size = tmpBlockId.size();
		for (int i = 0; i < size; i++)
		{
			updateBitmap(1, tmpBlockId[i]);	//�Ѹ��ļ�ռ�ÿ���λͼ����1 �������Ա�����ʹ��
		}

		//���Լ���Ϊ��Ч
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

		//������Ŀ¼ ���ȵݹ�ɾ����Ŀ¼
		if (childSize > 0)
			for (int i = 0; i < childSize; i++)
				recursiveRm(fileIndex.index[dirIndex].child[i]);
		
		//ɾ���Լ�
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
	if (path == "/s")	//�ݹ�ɾ���ļ���
	{
		string mode = path;
		ss >> path;
		int dirIndex = getFileIndex(path);
		if (dirIndex == -1)
		{
			printf("·��������\n");
			return 0;
		}
		if (fileIndex.index[dirIndex].fileType != dir)	//�ж�Ŀ�������Ƿ�Ϊdir
		{
			printf("Ŀ�����Ͳ�ƥ��\n");
			return 0;
		}
		if (dirIndex < DiskCnt + 1)	//��Ŀ¼�͸���Ӳ�̲���ɾ��
		{
			printf("����ɾ����Ŀ��\n");
			return 0;
		}

		if (!recursiveRm(dirIndex))
		{
			printf("�ݹ�ɾ���ļ���ʧ��\n");
			return 0;
		}

		return 1;
	}
	
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("·��������\n");
		return 0;
	}
	if (fileIndex.index[dirIndex].fileType != dir)	//�ж�Ŀ�������Ƿ�Ϊdir
	{
		printf("Ŀ�����Ͳ�ƥ��\n");
		return 0;
	}
	if (dirIndex < DiskCnt + 1)	//��Ŀ¼�͸���Ӳ�̲���ɾ��
	{
		printf("����ɾ����Ŀ��\n");
		return 0;
	}
	if (rm(dirIndex) == 0)
	{
		printf("Ŀ¼���ǿյ�\n");
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
		printf("·��������\n");
		return 0;
	}
	if (fileIndex.index[dirIndex].fileType != file)	//�ж�Ŀ�������Ƿ�Ϊfile
	{
		printf("Ŀ�����Ͳ�ƥ��\n");
		return 0;
	}
	rm(dirIndex);
	return 1;
}

int FileSystem::save()
{
	//�洢boost��Ϣ
	//�ȴ洢����ϵͳ������Ϣ
	FILE *fout = fopen("disk\\c\\0.txt", "w");
	updateBitmap(0, 0);
	int bitmapSize = (TotalBlockCnt + 7) / 8;
	for (int i = 0; i < bitmapSize; i++)
		fprintf(fout, "%c", bitmap[i]);
	fprintf(fout, "%d ", fileIndex.indexCnt);
	for (int i = 0; i < DiskCnt+2; i++)
	{
		//��indexͷ
		fprintf(fout, "%d %s %d ", fileIndex.index[i].id,
			fileIndex.index[i].fileName.c_str(), fileIndex.index[i].parentId);
		fprintf(fout, "%d ", fileIndex.index[i].child.size());
		for (int j = 0; j < fileIndex.index[i].child.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].child[j]);
		fprintf(fout, "%d %d ", fileIndex.index[i].effect, fileIndex.index[i].fileType);

		//��fcb
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

	
	//�ٰ�������Ϣ�洢����ʼ�������ļ�
	string path = "c:\\catalog.ini";
	int tmpFileId = openFile(path);
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	fout = fopen(filenameBuf, "w");
	
	for (int i = DiskCnt + 2; i < fileIndex.indexCnt; i++)
	{
		//��indexͷ
		fprintf(fout, "%d %s %d ", fileIndex.index[i].id, 
			fileIndex.index[i].fileName.c_str(), fileIndex.index[i].parentId);
		fprintf(fout, "%d ", fileIndex.index[i].child.size());
		for (int j = 0; j < fileIndex.index[i].child.size(); j++)
			fprintf(fout, "%d ", fileIndex.index[i].child[j]);
		fprintf(fout, "%d %d ", fileIndex.index[i].effect, fileIndex.index[i].fileType);
	
		//��fcb
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

	//�ȶ�ȡ����ϵͳ������Ϣ
	int bitmapSize = (TotalBlockCnt + 7) / 8;
	for (int i = 0; i < bitmapSize; i++)
		if (fscanf(fin, "%c", &bitmap[i]) == 0)
			return 0;

	//��ȡ����ϵͳ�����ṹ
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

	//���Ŀ¼�ļ����Ծ�����
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
	//�ȼ��ش��������飻
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

int FileSystem::openFile(string path)	//��ʧ�ܷ���-1
										//�򿪳ɹ�ʱ������ʱ�ļ����
{
	//��á�Ŀ���ļ�����Ŀ¼���
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("·��������\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("Ŀ�겻��һ���ļ�\n");
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
	//��á�Ŀ���ļ�����Ŀ¼���
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("Ŀ��·��������\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("Ŀ�겻��һ���ļ�\n");
		return -1;
	}

	//�򿪴洢����д�����ݡ����ļ�
	int tmpFileId = fileIndex.index[dirIndex].tmpFileId;
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	FILE *fin = fopen(filenameBuf, "r");
	if (fin == NULL)
	{
		printf("��Ч��д������\n");
		return -1;
	}

	//ͳ�ơ���д�����ݡ��Ĵ�С
	int contentLen = 0;
	char tmpChar;
	while (fscanf(fin, "%c", &tmpChar) == 1)
		contentLen++;
	fclose(fin);
	fin = fopen(filenameBuf, "r");

	//���ԭ�ļ���ռ�õ�block
	int blockCnt = fileIndex.index[dirIndex].fcb->blockId.size();
	for (int i = 0; i < blockCnt; i++)
	{
		updateBitmap(1, fileIndex.index[dirIndex].fcb->blockId[i]);
	}
	fileIndex.index[dirIndex].fcb->blockId.clear();

	//�޸ġ�Ŀ���ļ���״̬
	fileIndex.index[dirIndex].fcb->fstate = writing;
	fileIndex.index[dirIndex].fcb->flen = contentLen;
	getCurTime(fileIndex.index[dirIndex].fcb->lastmodtime);

	//������
	blockCnt = (contentLen + BlockSize - 1) / BlockSize;
	int blockLoc = 0;
	FILE *fout = NULL;
	for (int i = 0; i < contentLen; i++)
	{
		char *c_dir = "disk\\c";
		if (i % BlockSize == 0)
		{	
			//TODO:�������ʧ��
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

	//���Ŀ¼�ļ����Ծ�����
	if (dirIndex == 3)
	{
		FILE *fout2 = fopen("disk\\c\\0.txt", "a");
		int blockCnt = fileIndex.index[dirIndex].fcb->blockId.size();
		fprintf(fout2, "%d %d ", contentLen, blockCnt);
		for (int i = 0; i < blockCnt; i++)
			fprintf(fout2, "%d ", fileIndex.index[dirIndex].fcb->blockId[i]);
		fclose(fout2);
	}


	//��ԭ��Ŀ���ļ���״̬
	fileIndex.index[dirIndex].fcb->fstate = opened;
	return 1;
}

int FileSystem::closeFile(string path)
{
	//��á�Ŀ���ļ�����Ŀ¼���
	int dirIndex = getFileIndex(path);
	if (dirIndex == -1)
	{
		printf("Ŀ��·��������\n");
		return -1;
	}
	if (fileIndex.index[dirIndex].fileType != file)
	{
		printf("Ŀ�겻��һ���ļ�\n");
		return -1;
	}

	if (fileIndex.index[dirIndex].fcb->fstate == closed)
	{
		printf("Ŀ���ļ�δ��\n");
		return -1;
	}

	int tmpFileId = fileIndex.index[dirIndex].tmpFileId;
	char filenameBuf[MaxCharLen];
	sprintf(filenameBuf, "tmp%d.txt", tmpFileId);
	remove(filenameBuf);	//�Ƴ���ʱ�ļ�
	fileIndex.index[dirIndex].tmpFileId = -1;
	fileIndex.index[dirIndex].fcb->fstate = closed;
	return 1;
}