#include <iostream>
#include <string.h>
#include <string>
#include <vector>
using namespace std;
extern "C"{
    void aprint(const char*,int);
}
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

static const int COLOR_RED = 4;
static const int COLOR_WHITE = 7;
static const int COLOR_GREEN = 2;

int  BytsPerSec;	        //每扇区字节数
int  SecPerClus;	        //每簇扇区数
int  RsvdSecCnt;	        //Boot记录占用的扇区数
int  NumFATs;	            //FAT表个数
int  RootEntCnt;	        //根目录最大文件数
int  FATSz;	                //FAT扇区数      
int  BytesPerClus;			//每簇的字节数
int  EntPerClus;			//每簇的条目数


#pragma pack (1)

struct BPB{
/** type mark               description;default **/
    u16 BPB_BytsPerSec;     //每个扇区的字节数;0x200
    u8 BPB_SecPerClus;      //每簇的扇区数;1
    u16 BPB_ResvdSecCnt;    //Boot记录占用的扇区数;1
    u8 BPB_NumFATs;         //FAT表的记录数;2
    u16 BPB_RootEntCnt;     //根目录最大文件数;0xE0
    u16 BPB_TotSec16;       //逻辑扇区总数;0xB40
    u8 BPB_Media;           //媒体描述符;0xF0
    u16 BPB_FATSz16;        //每个FAT占用扇区数;9
    u16 BPB_SecPerTrk;      //每个磁道扇区数;0x12
    u16 BPB_NumHeads;       //磁头数;2
    u32 BPB_HiddSec;        //隐藏扇区数;0
    u32 BPB_TotSec32;       //如果BPB_TotSec16是0，则在这里记录扇区数;0
};

struct RootEntry {
    char DIR_Name[11];      //文件名8字节，扩展名3字节
    u8 DIR_Attr;            //文件属性
    char reserved[10];      //保留位
    u16 DIR_WrtTime;        //最后一次写入时间
    u16 DIR_WrtDate;        //最后一次写入日期
    u16 DIR_FstClus;        //文件开始的簇号
    u32 DIR_FileSize;       //文件大小
};

#pragma pack ()

void fillBPB(FILE* FAT12,BPB* bpb_ptr);                             			//从fat12读取BPB信息。
void fillRootEntry(FILE* fat12, RootEntry* rep, int base, int offset);			//从fat12读取RootEntry信息。
void fillContent(FILE *fat12, void *p, int start, int size);					//从fat12读取信息。
int  getFATValue(FILE * fat12 , int num);	                        			//读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值，
void printRoot(FILE *fat12, RootEntry *rep, const char *spath, int isDetail); 	//打印fat12文件系统的根目录和其子目录
bool printSub(FILE *fat12, const char *fpath, int startClus, const char *spath, int isDetail); //打印一个子目录（非根目录）
void catRoot(FILE *fat12, RootEntry *rep, const char *spath, const char *fname);
bool catSub(FILE *fat12, int startClus, const char *fullName, const char *spath, const char *fname);
void catFile(FILE *fat12, int startClus, int fileSize);
void directoryOutPut(const char *fpath, vector<string> dlist, vector<string> flist);
void directoryOutPutDetail(const char *fpath, int dnum, int fnum, vector<string> dlist, vector<string> flist, vector<int> dnlist, vector<int> fnlist, vector<int> fszlist);
int getdfnum(FILE *fat12, int startClus);										//计算文件夹中的文件夹数和文件数，前16位存文件夹数，后16位存文件数。
bool isValidEntry(char *content,int offset);									//判断从content开始偏移offset的32个字节是否是一个Entry
vector<string> split(string str,string s);
void aprintnum(int num, int color);
bool isFatherPath(const char *fpath, const char *spath);						//判断fpath是否是spath的父目录。

static char PATH_IMG[] = "a.img";
static char MSG_TEST[] = "test succeed!\n";
static char MSG_QUIT[] = "quit successfully!\n";
static char MSG_INVALID_PATH[] = "Invalid path.\n";
static char MSG_INVALID_OPT[] = "Invalid command option.\n";
static char MSG_INVALID_DIRECTORY[] = "No such a directory.\n";
static char MSG_NO_FILE[] = "No such a file.\n";
static char MSG_UNRECOGNIZED_ORDER[] = "Unrecognized input. Usages:\n1. ls [-l [directory_path]]\n2. cat <file>\n3. quit\n";
static char MSG_IMG_WRONG[] = "Something wrong with img file.\n";
static char MSG_CIRCULAR_CLUSTER[] = "Read failure.Circular cluster.\n";

int main(){

	FILE *fat12;
	fat12 = fopen(PATH_IMG, "rb");

	BPB bpb;
	BPB *bpb_ptr = &bpb;
	RootEntry rootEntry;
	RootEntry *rep = &rootEntry;
	string input;

	//读取BPB信息
	fillBPB(fat12, bpb_ptr);

	BytsPerSec = bpb_ptr->BPB_BytsPerSec;
	SecPerClus = bpb_ptr->BPB_SecPerClus;
	RsvdSecCnt = bpb_ptr->BPB_ResvdSecCnt;
	NumFATs = bpb_ptr->BPB_NumFATs;
	RootEntCnt = bpb_ptr->BPB_RootEntCnt;
	FATSz = (bpb_ptr->BPB_FATSz16 == 0) ? bpb_ptr->BPB_TotSec32 : bpb_ptr->BPB_FATSz16;
	BytesPerClus = SecPerClus * BytsPerSec;	
	EntPerClus = BytesPerClus / 32;	

	while(true){
        getline(cin,input);

        //ls;
        if(input.compare("ls")==0){
			printRoot(fat12, rep, "/", 0);
			continue;
        }
		//quit;
		if(input.compare("quit")==0){
			break;
		}
        //ls **;
        if(input.substr(0,3).compare("ls ")==0){
			string path = "";
			bool optl = false;
			bool isOptValid = true;
			bool isPathValid = true;
			vector<string> args = split(input , " ");
			for (int i = 1; i < args.size(); i++){
				
				//指令参数
				if (args[i][0] == '-'){
					for (int j = 1; j < args[i].length(); j++){
						if (args[i][j] == 'l')
							optl = true;
						else{
							isOptValid = false;
							break;
						}
					}
				}
				//非指令参数
				else if (args[i].length() > 0){
					if(args[i][0]!='/'){
						isPathValid = false;
					}else{
						if (path.length() == 0)
							path = args[i];
						else
							isPathValid = false;	//多路径命令无效
					}
				}
			}

			if (optl && isPathValid && isOptValid){
				if (path.length() == 0)
					printRoot(fat12, rep, "/", 1);
				else
					printRoot(fat12, rep, path.c_str(), 1);
			}
			else if(!optl && isPathValid && isOptValid){
				if (path.length() == 0)
					printRoot(fat12, rep, "/", 0);
				else
					printRoot(fat12, rep, path.c_str(), 0);
			}
			else if(isPathValid && !isOptValid){
				aprint(MSG_INVALID_OPT, COLOR_WHITE);
			}
			else if (isOptValid && !isPathValid){
				aprint(MSG_INVALID_PATH, COLOR_WHITE);
			}
			else{
				aprint(MSG_UNRECOGNIZED_ORDER, COLOR_GREEN);
			}
		}
        //cat **;
        else if(input.substr(0,4).compare("cat ")==0){
			string path = "";
			vector<string> args = split(input," ");
			bool isPathValid = true;
			for (int i = 1; i < args.size(); i++){
				if(args[i].length()>0){
					if (path.length() == 0)
						path = args[i];
					else
						isPathValid = false;	//多路径命令无效
				}
			}
			if(isPathValid){
				if (path[0] != '/')
					path = "/" + path;
				int idx = path.find_last_of("/");
				string fname = path.substr(idx+1,path.length()-idx-1);
				string spath = path.substr(0,idx+1);
				catRoot(fat12, rep, spath.c_str(), fname.c_str());
			}	
			else aprint(MSG_INVALID_PATH, COLOR_WHITE);
		}
		else {
			aprint(MSG_UNRECOGNIZED_ORDER, COLOR_GREEN);
		}
    }

	fclose(fat12);
    return 0;
}

void catRoot(FILE *fat12, RootEntry *rep, const char *spath, const char *fname){
	int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz);
	char fullName[] = "/";
	bool isSearch = true;			//是否要在该目录下搜索文件
	bool isFound = false;			//是否搜索到了文件
	int offset = 0;
	char fileName[12];
	int tempi;
	vector<int> directoryIndexList = vector<int>();
	vector<string> directoryNameList = vector<string>();
	vector<int> fileIndexList = vector<int>();
	vector<string> fileNameList = vector<string>();

	if (strcmp(fullName, spath) != 0) {
		isSearch = false;
	}

	//遍历RootEntry
	for(int i=0;i<RootEntCnt;i++){
		//从img读取rootEntry信息
		offset = 32 * i;
		fillRootEntry(fat12, rep, base, offset);
		//跳过空条目
		if (rep->DIR_Name[0] == '\0')
			continue;

		//输出根目录下文件夹名和文件名
		if (isSearch && (rep->DIR_Attr & 0x10) == 0){			//文件且搜索目录为根目录
			tempi = 0;
			for (int j = 0; j < 8; j++){
				if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
				else break;
			}
			if (rep->DIR_Name[8] != ' '){
				fileName[tempi++] = '.';
				for (int j = 8; j < 11; j++){
					if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
					else break;
				}
			}
			fileName[tempi] = '\0';
			fileIndexList.push_back(i);
			fileNameList.push_back(fileName);
		}	
		else if(!isSearch && (rep->DIR_Attr & 0x10) != 0){			//目录并且搜索路径不是根目录
			tempi = 0;
			for (int j = 0; j < 11; j++){
				if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
				else break;
			}
			fileName[tempi] = '\0';
			directoryIndexList.push_back(i);
			directoryNameList.push_back(fileName);
		}	
	}

	if(isSearch){
		for (int i = 0; i < fileNameList.size(); i++){
			if (fileNameList[i].compare(fname) == 0){
				isFound = true;
				offset = fileIndexList[i] * 32;
				fillRootEntry(fat12, rep, base, offset);
				catFile(fat12, rep->DIR_FstClus, rep->DIR_FileSize);
				break;
			}
		}
	}
	else{
		for (int i = 0; i < directoryIndexList.size(); i++){
			offset = directoryIndexList[i] * 32;
			fillRootEntry(fat12, rep, base, offset);
			string directoryFullName = fullName + directoryNameList[i] + "/";
			isFound |= catSub(fat12, rep->DIR_FstClus, directoryFullName.c_str(), spath, fname);
		}
	}

	if(!isFound)aprint(MSG_NO_FILE,COLOR_WHITE);
}

bool catSub(FILE *fat12, int startClus, const char *fullName, const char *spath, const char *fname){
		int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

		bool isSearch = true;			//是否要在该目录下搜索文件
		bool isFound = false;			//是否搜索到了文件
		int offset = 0;
		char fileName[12];
		int tempi;
		vector<int> directoryIndexList = vector<int>();
		vector<string> directoryNameList = vector<string>();
		vector<int> directoryClusList = vector<int>();
		vector<int> fileIndexList = vector<int>();
		vector<string> fileNameList = vector<string>();
		vector<int> fileSizeList = vector<int>();
		vector<int> fileClusList = vector<int>();

		if (strcmp(fullName, spath) != 0)
			isSearch = false;
		
		int currentClus = startClus;
		int value = 0;

		while(value<0xFF8){
			value = getFATValue(fat12,currentClus);
			if (value == 0xFF7){													//环簇
				aprint(MSG_CIRCULAR_CLUSTER, COLOR_RED);
				break;
			}

			int base = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;		//簇开始的位置

			char *clusData = (char *)malloc(BytesPerClus);
			fillContent(fat12, clusData, base, BytesPerClus);
			char *content = clusData;

			for (int i = 0; i < EntPerClus; i++){

				memset(fileName, 0, strlen(fileName));
				offset = 32 * i;

				if (!isValidEntry(content, offset)) continue;

				if (isSearch && content[offset + 11] != 16){			//需要搜索并且是文件
					tempi = 0;
					for (int j = 0; j < 8; j++){
						if (content[offset + j] != ' '){
							fileName[tempi++] = content[offset + j];
						}else{
							break;
						}
					}
					if(content[offset+8]!=' '){
						fileName[tempi++] = '.';
						for (int j = 8; j < 11; j++){
							if (content[offset + j] != ' ')
								fileName[tempi++] = content[offset + j];
							else
								break;
						}
					}
					fileName[tempi] = '\0';
					fileNameList.push_back(fileName);
					int x = (int)content[offset+28];
					x &= 0x000000ff;
					fileSizeList.push_back(x);
					int clus = (int)content[offset + 26];
					clus += (int)content[offset + 27];
					fileClusList.push_back(clus);
				}
				else if(!isSearch && content[offset + 11] == 16){			//该目录不需要搜索且是文件夹
					tempi = 0;
					for (int j = 0; j < 11; j++){
						if (content[offset + j] != ' ') fileName[tempi++] = content[offset + j];
						else break;
					}
					fileName[tempi] = '\0';
					directoryIndexList.push_back(i);
					directoryNameList.push_back(fileName);
					int clus = (int)content[offset + 26];
					clus += (int)content[offset + 27];
					directoryClusList.push_back(clus);
				}
			}
			if(isSearch){
				for (int i = 0; i < fileNameList.size(); i++){
					if (fileNameList[i].compare(fname) == 0){
						isFound = true;
						catFile(fat12, fileClusList[i], fileSizeList[i]);
						break;
					}
				}
			}
			else{
				for (int i = 0; i < directoryIndexList.size(); i++){
					string directoryFullName = fullName + directoryNameList[i] + "/";
					isFound |= catSub(fat12, directoryClusList[i], directoryFullName.c_str(), spath, fname);
				}
			}
			free(clusData);
		}
	return isFound;

}

void catFile(FILE *fat12, int startClus, int fileSize){
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

	int resBytes = fileSize;	//还需要拷贝的字节数
	char *result = (char *)malloc(fileSize + 1);
	result[fileSize + 1] = '\0';
	char *respt = result;
	int currentClus = startClus;
	int value = 0;

	while(value<0xFF8){
		value = getFATValue(fat12,currentClus);
		if (value == 0xFF7){													//环簇
			aprint(MSG_CIRCULAR_CLUSTER, COLOR_RED);
			break;
		}
		int base = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;		//簇开始的位置

		char *clusData = (char *)malloc(BytesPerClus);
		fillContent(fat12, clusData, base, BytesPerClus);
		if(resBytes>BytesPerClus){
			for(int i=0;i<BytesPerClus;i++){
				respt[i] = clusData[i];
			}
			respt = respt + BytesPerClus;
			resBytes -= BytesPerClus;
		}
		else{
			for(int i=0;i<resBytes;i++){
				respt[i] = clusData[i];
			}
			respt = respt + resBytes;
			resBytes = 0;
		}

		currentClus = value;	
		free(clusData);	
	}	
	aprint(result, COLOR_WHITE);
	aprint("\n", COLOR_WHITE);
	free(result);
}


void printRoot(FILE *fat12, RootEntry *rep, const char *spath, int isDetail){

	int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz);
	int offset = 0;
	int check;
    char fileName[12];
	int tempi;
	int counts[2] = {0, 0};		//dcount,fcount
	char fullName[] = "/";
	bool isPrint = true;		//是否是搜索目录或搜索目录的子目录
	vector<int> directoryIndexList = vector<int>();
	vector<string> directoryNameList = vector<string>();
	vector<int> directoryDNumList = vector<int>();
	vector<int> directoryFNumList = vector<int>();
	vector<string> fileNameList = vector<string>();
	vector<int> fileSizeList = vector<int>();


	//fullName != spath && fullName != spath+"/"
	if ((strcmp(fullName, spath) != 0) && (string(fullName).compare(string(spath) + "/")) != 0)	{
		if (isFatherPath(fullName, spath)){
			isPrint = false;
		}		
		else{
			aprint(MSG_INVALID_DIRECTORY, COLOR_WHITE);
			return;
		}
	}

	for(int i=0;i<RootEntCnt;i++){
		//从img读取rootEntry信息
		offset = 32 * i;
		fillRootEntry(fat12, rep, base, offset);

		//跳过空条目
		if (rep->DIR_Name[0] == '\0')
			continue;

		//输出根目录下文件夹名和文件名
		if ((rep->DIR_Attr & 0x10) == 0){			//文件
			counts[1]++;
			tempi = 0;
			for (int j = 0; j < 8; j++){
				if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
				else break;
			}
			if (rep->DIR_Name[8] != ' '){
				fileName[tempi++] = '.';
				for (int j = 8; j < 11; j++){
					if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
					else break;
				}
			}
			fileName[tempi] = '\0';
			fileNameList.push_back(fileName);
			fileSizeList.push_back(rep->DIR_FileSize);
		}
		else{									//目录
			counts[0]++;
			tempi = 0;
			for (int j = 0; j < 11; j++){
				if (rep->DIR_Name[j] != ' ') fileName[tempi++] = rep->DIR_Name[j];
				else break;
			}
			fileName[tempi] = '\0';
			directoryIndexList.push_back(i);
			directoryNameList.push_back(fileName);
		}		
	}

	if (isDetail == 1){
		for (int i = 0; i < directoryIndexList.size(); i++){
			offset = directoryIndexList[i] * 32;
			fillRootEntry(fat12, rep, base, offset);
			int dfnum = getdfnum(fat12, rep->DIR_FstClus);
			directoryDNumList.push_back(dfnum & 0x00ff);
			directoryFNumList.push_back(dfnum >> 16);
		}	
	}

	//输出部分
	if(isPrint){
		if(isDetail==0){
			directoryOutPut(fullName, directoryNameList, fileNameList);
		}
		else{
			directoryOutPutDetail(fullName, counts[0], counts[1], directoryNameList, fileNameList, directoryDNumList, directoryFNumList, fileSizeList);
		}
	}
	

	//遍历根目录下的文件夹
	for (int i = 0; i < directoryIndexList.size(); i++){
		offset = directoryIndexList[i] * 32;
		fillRootEntry(fat12, rep, base, offset);
		string directoryFullName = fullName + directoryNameList[i] + "/";
		isPrint |= printSub(fat12, directoryFullName.c_str(), rep->DIR_FstClus, spath, isDetail);
	}

	if (!isPrint)
		aprint(MSG_INVALID_DIRECTORY, COLOR_WHITE);
}

bool printSub(FILE *fat12, const char *fullName, int startClus, const char *spath, int isDetail){
	//数据区的第一个簇（即2号簇）的偏移字节
	//公式之所以在分子上加上(BPB_BytsPerSec–1)，是为了保证根目录区没有填满整数个扇区时仍然适用。
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

	bool isPrint = true;
	int offset;
	int tempi;
	char fileName[12];
	vector<int> directoryIndexList = vector<int>();
	vector<string> directoryNameList = vector<string>();
	vector<int> directoryDNumList = vector<int>();
	vector<int> directoryFNumList = vector<int>();
	vector<int> directoryClusList = vector<int>();
	vector<string> fileNameList = vector<string>();
	vector<int> fileSizeList = vector<int>();
	int counts[] = {0, 0, 0};

	int currentClus = startClus;
	int value = 0;

	//fullName != spath && fullName != spath+"/"
	if ((strcmp(fullName, spath) != 0) && (string(fullName).compare(string(spath) + "/")) != 0)	{
		if (isFatherPath(fullName, spath)){
			isPrint = false;
		}
		else if(isFatherPath(spath,fullName)){
			isPrint = true;
		}
		else{
			return false;
		}
	}

	while(value<0xFF8){
		value = getFATValue(fat12,currentClus);
		if (value == 0xFF7){													//环簇
			aprint(MSG_CIRCULAR_CLUSTER, COLOR_RED);
			break;
		}

		int base = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;		//簇开始的位置

		char *clusData = (char *)malloc(BytesPerClus);
		fillContent(fat12, clusData, base, BytesPerClus);
		char *content = clusData;

		for (int i = 0; i < EntPerClus; i++){

			memset(fileName, 0, strlen(fileName));
			offset = 32 * i;

			if (!isValidEntry(content, offset)) continue;

			if (content[offset + 11] != 16){			//文件
				counts[1]++;
				tempi = 0;
				for (int j = 0; j < 8; j++){
					if (content[offset + j] != ' '){
						fileName[tempi++] = content[offset + j];
					}else{
						break;
					}
				}
				if(content[offset+8]!=' '){
					fileName[tempi++] = '.';
					for (int j = 8; j < 11; j++){
						if (content[offset + j] != ' ')
							fileName[tempi++] = content[offset + j];
						else
							break;
					}
				}
				fileName[tempi] = '\0';
				fileNameList.push_back(fileName);
				int x = (int)content[offset+28];
				x &= 0x000000ff;
				fileSizeList.push_back(x);
			}
			else{										//文件夹
				counts[0]++;
				tempi = 0;
				for (int j = 0; j < 11; j++){
					if (content[offset + j] != ' ') fileName[tempi++] = content[offset + j];
					else break;
				}
				fileName[tempi] = '\0';
				directoryIndexList.push_back(i);
				directoryNameList.push_back(fileName);
				counts[2] = (int)content[offset + 26];
				counts[2] += (int)content[offset + 27];
				directoryClusList.push_back(counts[2]);
			}
		}
		
		free(clusData);
	}

	if (isDetail == 1){
		for (int i = 0; i < directoryIndexList.size(); i++){
			offset = directoryIndexList[i] * 32;
			int dfnum = getdfnum(fat12, directoryClusList[i]);
			directoryDNumList.push_back(dfnum >> 16);
			directoryFNumList.push_back(dfnum & 0x00ff);			
		}	
	}

	//输出部分
	if(isPrint){
		if(isDetail==0){
			directoryOutPut(fullName, directoryNameList, fileNameList);
		}
		else{
			directoryOutPutDetail(fullName, counts[0], counts[1], directoryNameList, fileNameList, directoryDNumList, directoryFNumList, fileSizeList);
		}
	}

	currentClus = value;
	for (int i = 0; i < directoryIndexList.size(); i++){
		string dFullName = fullName + directoryNameList[i] + "/";
		isPrint |= printSub(fat12, dFullName.c_str(), directoryClusList[i], spath, isDetail);
	}
	
	return isPrint;
}

int  getFATValue(FILE * fat12 , int num) {
	//FAT1的偏移字节
	int fatBase = RsvdSecCnt * BytsPerSec;
	//FAT项的偏移字节
	int fatPos = fatBase + num*3/2;
	//奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
	int type = 0;
	if (num % 2 == 0) {
		type = 0;
	} else {
		type = 1;
	}
 
	//先读出FAT项所在的两个字节
	u16 bytes;
	u16 *bytes_ptr = &bytes;
	int check;
	check = fseek(fat12, fatPos, SEEK_SET);
	if (check == -1)
		aprint(MSG_IMG_WRONG, COLOR_RED);

	check = fread(bytes_ptr, 1, 2, fat12);
	if (check != 2)
		aprint(MSG_IMG_WRONG,COLOR_RED);

	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	if (type == 0) {
		return ((int)(bytes & 0X0FFF)) & 0x0000ffff;
	} else {
		return ((int)(bytes >> 4)) & 0x0000ffff;
	}
}

int getdfnum(FILE *fat12, int startClus){
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	int offset;
	int counts[2] = {0, 0};
	
	int currentClus = startClus;
	int value = 0;

	while (value < 0xff8){
		value = getFATValue(fat12, currentClus);
		if(value == 0xff7){
			aprint(MSG_CIRCULAR_CLUSTER, COLOR_RED);
			break;
		}
		int base = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;
		char *clusData = (char *)malloc(BytesPerClus);
		fillContent(fat12, clusData, base, BytesPerClus);
		char *content = clusData;
		for (int i = 0; i < EntPerClus; i++){
			offset = 32 * i;
			if (!isValidEntry(content, offset)) continue;
			if (content[offset + 11] != 16) counts[1]++;
			else counts[0]++;
		}
	}
	return (counts[0] << 16) + (counts[1]);
}

// 从fat12读取RootEntry信息
void fillRootEntry(FILE *fat12, RootEntry *rep, int base, int offset){
	fillContent(fat12, rep, base + offset, 32);
}

// 从fat12读取BPB信息。
void fillBPB(FILE* fat12,BPB* bpb_ptr){
	fillContent(fat12, bpb_ptr, 11, 25);
}

// 从fat12读取信息。
void fillContent(FILE *fat12, void *p, int start, int size){
	int check;
	check = fseek(fat12, start, SEEK_SET);	
	if(check==-1){
        aprint(MSG_IMG_WRONG, COLOR_RED);
    }
	check = fread(p, 1, size, fat12);
	if(check!=size){
        aprint(MSG_IMG_WRONG, COLOR_RED);
    }
	
}

void directoryOutPut(const char *fpath, vector<string> dlist, vector<string> flist){
	aprint(fpath, COLOR_WHITE);
	aprint(":\n",COLOR_WHITE);

	string root = "/";
	if (root.compare(fpath) != 0){
		aprint(".  ..  ", COLOR_RED);
	}
	for (string s : dlist){
		aprint(s.c_str(), COLOR_RED);
		aprint("  ", COLOR_WHITE);
	}
	for (string s : flist){
		aprint(s.c_str(), COLOR_WHITE);
		aprint("  ", COLOR_WHITE);
	}
	aprint("\n", COLOR_WHITE);
}

void directoryOutPutDetail(const char *fpath, int dnum, int fnum, vector<string> dlist, vector<string> flist, vector<int> dnlist, vector<int> fnlist, vector<int> fszlist){
	aprint(fpath, COLOR_WHITE);
	aprint(" ", COLOR_WHITE);
	aprintnum(dnum, COLOR_WHITE);
	aprint(" ", COLOR_WHITE);
	aprintnum(fnum, COLOR_WHITE);
	aprint(":\n", COLOR_WHITE);

	string root = "/";
	if (root.compare(fpath) != 0){
		aprint(".\n", COLOR_RED);
		aprint("..\n", COLOR_RED);
	}

	for (int i = 0; i < dlist.size(); i++){
		aprint(dlist[i].c_str(), COLOR_RED);
		aprint("  ", COLOR_WHITE);
		aprintnum(dnlist[i], COLOR_WHITE);
		aprint(" ", COLOR_WHITE);
		aprintnum(fnlist[i], COLOR_WHITE);
		aprint("\n", COLOR_WHITE);
	}

	for (int i = 0; i < flist.size(); i++){
		aprint(flist[i].c_str(), COLOR_WHITE);
		aprint("  ", COLOR_WHITE);
		aprintnum(fszlist[i], COLOR_WHITE);
		aprint("\n", COLOR_WHITE);
	}
	
	aprint("\n", COLOR_WHITE);
}

bool isFatherPath(const char *fpath, const char *spath){
	if (string(spath).find(string(fpath)) != 0)
		return false;
	return true;
}

bool isValidEntry(char *content,int offset){
	if (content[offset] == '\0') return false;
	for (int j=offset;j<offset+11;j++) {
		if (!(((content[j] >= 48) && (content[j] <= 57)) || ((content[j] >= 65) && (content[j] <= 90)) ||
				((content[j] >= 97) && (content[j] <= 122)) || (content[j] == ' '))) return false;
	}
	return true;
}

vector<string> split(string str, string s) {
	vector<string> result = vector<string>();
	int start = 0, end;
	int slen = s.length();
	end = str.find(s);
	while (end != -1) {
		result.push_back(str.substr(start, end - start));
		start = end + slen;
		end = str.find(s, start);
	}
	if (start < str.length()) {
		result.push_back(str.substr(start, str.length() - start));
	}
	return result;
}

void aprintnum(int num, int color){
	string str = to_string(num);
	aprint(str.c_str(), color);
}