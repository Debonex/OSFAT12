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

void fillBPB(FILE* FAT12,BPB* bpb_ptr);                             	//从fat12读取BPB信息。
void fillRootEntry(FILE* fat12, RootEntry* rep, int base, int offset);	//从fat12读取RootEntry信息。
void fillContent(FILE *fat12, void *p, int start, int size);			//从fat12读取信息。
int  getFATValue(FILE * fat12 , int num);	                        	//读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值，
void printRoot(FILE* fat12, RootEntry* rep);							//打印fat12文件系统的根目录和其子目录
void printSub(FILE *fat12,const char *fpath, int startClus);			//打印一个子目录（非根目录）
vector<string> split(string str,string s);

static char PATH_IMG[] = "ref.img";
static char MSG_TEST[] = "test succeed!\n";
static char MSG_QUIT[] = "quit successfully!\n";
static char MSG_INVALID_PATH[] = "Invalid path.\n";
static char MSG_INVALID_OPT[] = "Invalid command option.\n";
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

	while(true){
        getline(cin,input);

        //ls;
        if(input.compare("ls")==0){
			printRoot(fat12, rep);
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
				else{
					if(args[i][0]!='/'){
						isPathValid = false;
					}else{
						if (path.length() == 0)
							path = input;
						else
							isPathValid = false;	//多路径命令无效
					}
				}
			}

			if (optl && isPathValid && isOptValid){
				aprint(MSG_TEST, COLOR_GREEN);
			}
			else if(!optl && isPathValid && isOptValid){
				aprint(MSG_TEST, COLOR_GREEN);
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
			aprint(MSG_TEST, COLOR_WHITE);
		}
		else {
			aprint(MSG_UNRECOGNIZED_ORDER, COLOR_GREEN);
		}
    }

	fclose(fat12);
    return 0;
}




void printRoot(FILE* fat12, RootEntry* rep){

    int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz);
	int offset = 0;
	int check;
    char fileName[12];
	int tempi;
	int fcount = 0;		//文件数量
	int dcount = 0;		//文件夹数量
	vector<int> directoryIndexList = vector<int>();
	vector<string> directoryNameList = vector<string>();
	char rootpath[] = "/";

	aprint("/:\n",COLOR_WHITE);

    for(int i=0;i<RootEntCnt;i++){
		//从img读取rootEntry信息
		offset = 32 * i;
		fillRootEntry(fat12, rep, base, offset);

		//跳过空条目
		if (rep->DIR_Name[0] == '\0')
			continue;

		//输出根目录下文件夹名和文件名
		if ((rep->DIR_Attr & 0x10) == 0){			//文件
			fcount++;
			tempi = 0;
			for (int j = 0; j < 8; j++){
				if (rep->DIR_Name[j] != ' '){
					fileName[tempi++] = rep->DIR_Name[j];
				}
				else
					break;
			}
			if (rep->DIR_Name[8] != ' '){
				fileName[tempi++] = '.';
				for (int j = 8; j < 11; j++){
					if (rep->DIR_Name[j] != ' '){
						fileName[tempi++] = rep->DIR_Name[j];
					}
					else
						break;
				}
			}
			fileName[tempi] = '\0';
			aprint(fileName, COLOR_WHITE);
			aprint("  ", COLOR_WHITE);
		}
		else{									//目录
			tempi = 0;
			dcount++;
			for (int j = 0; j < 11; j++){
				if (rep->DIR_Name[j] != ' '){
					fileName[tempi++] = rep->DIR_Name[j];
				}
				else
					break;
			}
			fileName[tempi] = '\0';
			aprint(fileName, COLOR_RED);
			aprint("  ", COLOR_WHITE);
			directoryIndexList.push_back(i);
			string temps = fileName;
			temps = "/" + temps + "/";
			directoryNameList.push_back(temps);
		}		
	}
	if (fcount + dcount > 0)
		aprint("\n", COLOR_WHITE);

	//遍历根目录下的文件夹
	for (int i = 0; i < directoryIndexList.size(); i++){
		offset = directoryIndexList[i] * 32;
		fillRootEntry(fat12, rep, base, offset);
		printSub(fat12, directoryNameList[i].c_str(), rep->DIR_FstClus);
	}	
}

void printSub(FILE *fat12,const char *fullName, int startClus){
	//数据区的第一个簇（即2号簇）的偏移字节
	//公式之所以在分子上加上(BPB_BytsPerSec–1)，是为了保证根目录区没有填满整数个扇区时仍然适用。
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

	RootEntry *rep;
	int offset;
	int tempi;
	char fileName[12];
	vector<int> directoryIndexList = vector<int>();
	vector<string> directoryNameList = vector<string>();
	int counts[] = {0, 0, 0};

	int currentClus = startClus;
	int value = 0;
	aprint(fullName, COLOR_WHITE);
	aprint(":\n", COLOR_WHITE);
	aprint(".  ..  ", COLOR_RED);

	while(value<0xFF8){
		value = getFATValue(fat12,currentClus);
		if (value == 0xFF7){													//环簇
			aprint(MSG_CIRCULAR_CLUSTER, COLOR_RED);
			break;
		}
		int bytesPerClus = SecPerClus * BytsPerSec;								//每簇的字节数
		int entPerClus = bytesPerClus / 32;										//每簇的条目数	
		int base = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;		//簇开始的位置

		char *clusData = (char *)malloc(bytesPerClus);
		fillContent(fat12, clusData, base, bytesPerClus);
		char *content = clusData;

		for (int i = 0; i < entPerClus; i++){

			memset(fileName, 0, strlen(fileName));
			offset = 32 * i;

			if (content[offset] == '\0')
				continue;

			int boolean = 0;
			for (int j=offset;j<offset+11;j++) {
				if (!(((content[j] >= 48)&&(content[j] <= 57)) ||
					((content[j] >= 65)&&(content[j] <= 90)) ||
							((content[j] >= 97)&&(content[j] <= 122)) ||
								(content[j] == ' '))) {
									boolean = 1;	//非英文及数字、空格
									break;
				}	
			}
			if (boolean == 1) {
				continue;
			}	//非目标文件不输出

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
				aprint(fileName, COLOR_WHITE);
				aprint("  ", COLOR_WHITE);
			}
			else{										//文件夹
				counts[0]++;
				tempi = 0;
				for (int j = 0; j < 11; j++){
					if (content[offset + j] != ' '){
						fileName[tempi++] = content[offset + j];
					}else{
						break;
					}
				}
				fileName[tempi] = '\0';
				aprint(fileName, COLOR_RED);
				aprint("  ", COLOR_WHITE);
				directoryIndexList.push_back(i);
				string temps = fileName;
				temps = "/" + temps + "/";
				directoryNameList.push_back(temps);
			}
		}
		counts[2] = content[offset + 26] << 4 + content[offset + 27];
	}

	aprint("\n", COLOR_WHITE);
	currentClus = value;
	for (int i = 0; i < directoryIndexList.size(); i++){
		offset = directoryIndexList[i] * 32;
		printSub(fat12, directoryNameList[i].c_str(), counts[2]);
	}
	
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
		printf("fseek in getFATValue failed!");

	check = fread(bytes_ptr, 1, 2, fat12);
	if (check != 2)
		printf("fread in getFATValue failed!");

	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	if (type == 0) {
		return bytes << 4;
	} else {
		return bytes >> 4;
	}
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