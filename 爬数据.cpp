#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include <string>
#include <fstream>
#include <iostream>

#define MAXBLOCKSIZE 1024
#define MAXLENGTH 1024
#define MAXITEMNUM 350000
#define URLPREFIX "https://play.google.com/store/apps/details?id="

#pragma comment(lib,"wininet.lib") 

using namespace std;

HINTERNET hsession;
HANDLE mutex4err,mutex4fin,mutex4out;
ofstream outerr,outfile;
ifstream infile;
int itemnum=0,itemnumfinal=0;

string UTF8ToGBK(const std::string& strUTF8);
string GBKToUTF8(const std::string& strGBK);
bool match(const string &src,const string &st1,const string &st2,int &pos,int &length,const int limitlen);
DWORD WINAPI download(LPVOID lpparam);

class DATA
{
public:
	int id;				// 序号
	string url;			// 链接
	string name;		// 名称
	string intro;		// 说明
	string score;		// 平均评分
	string votes;		// 评分人数
	string date;		// 更新日期
	string ver;			// 当前版本
	string android;		// 所需的安卓版本
	string category;	// 类别
	string install;		// 最少安装次数
	string size;		// 大小
	string price;		// 价格
	string range;		// 内容分级
	void output()
	{
		outfile<<id<<"\t";
		outfile<<name<<"\t";
		outfile<<score<<"\t";
		outfile<<votes<<"\t";
		outfile<<date<<"\t";
		outfile<<ver<<"\t";
		outfile<<android<<"\t";
		outfile<<category<<"\t";
		outfile<<install<<"\t";
		outfile<<size<<"\t";
		outfile<<price<<"\t";
		outfile<<range<<"\t";
		outfile<<url<<"\t";
		outfile<<intro<<endl;
	}
};

string UTF8ToGBK(const std::string& strUTF8)  
{  
    int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);  
    unsigned short * wszGBK = new unsigned short[len + 1];  
    memset(wszGBK, 0, len * 2 + 2);  
    MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)strUTF8.c_str(), -1, wszGBK, len);  
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);  
    char *szGBK = new char[len + 1];  
    memset(szGBK, 0, len + 1);  
    WideCharToMultiByte(CP_ACP,0, wszGBK, -1, szGBK, len, NULL, NULL);  
    std::string strTemp(szGBK);  
    delete[]szGBK;  
    delete[]wszGBK;  
    return strTemp;  
}

string GBKToUTF8(const std::string& strGBK)
{
	string strOutUTF8 = "";
	WCHAR * str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char * str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
}

bool match(const string &src,const string &st1,const string &st2,int &pos,int &length,const int limitlen=-1)
{
	int len=src.length(),l1=st1.length(),l2=st2.length();
	int i;
	bool issame,issame2;
	for(pos=l1+1;pos<len-l2-1;pos++)
	{
		issame=1;
		for(i=1;i<=l1;i++)
			if(st1[l1-i]!=src[pos-i])
			{
				issame=0;
				break;
			}
		if(issame)
		{
			for(length=1;pos+length<len-l2-1;length++)
			{
				issame2=1;
				if(limitlen>0&&length>limitlen)break;
				for(i=0;i<l2;i++)
					if(st2[i]!=src[pos+length+i])
					{
						issame2=0;
						break;
					}
				if(issame2)return 1;
			}
		}
	}
	return 0;
}

DWORD WINAPI download(LPVOID lpparam)
{
	char itemname[MAXLENGTH];
	while(1)
	{
		DATA data;
		ULONG number=1;
		int item,trytimes=0,i;
		string str;
		WaitForSingleObject(mutex4fin,INFINITE);
		item=itemnum;
		infile.getline(itemname,MAXLENGTH);
		if(item<=itemnumfinal)printf("%d . %s\n",item,itemname);
		itemnum++;
		ReleaseMutex(mutex4fin);
		if(item>itemnumfinal)return 0;
		data.id=item;
		data.url=string(URLPREFIX)+string(itemname);
		HINTERNET handle=NULL;
		handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)data.url.c_str(),NULL,0,INTERNET_FLAG_DONT_CACHE,0);
		while(handle==NULL&&trytimes<3)
		{
			trytimes++;
			handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)data.url.c_str(),NULL,0,INTERNET_FLAG_DONT_CACHE,0);
		}
		if(handle!=NULL)
		{
			string tmp;
			int pos,length;
			char readchar[MAXBLOCKSIZE+1];
			while(number>0)
			{
				InternetReadFile(handle,readchar,MAXBLOCKSIZE-1,&number);
				for(i=0;i<number;i++)
					str+=readchar[i];
			}
			InternetCloseHandle(handle);
//			str=UTF8ToGBK(str);
//			printf("%s\n",str.c_str());
			if(match(str,GBKToUTF8("<title>"),GBKToUTF8(" - Google Play 上"),pos,length))data.name=str.substr(pos,length);
			if(match(str,GBKToUTF8("<h2>说明</h2>"),GBKToUTF8("</div>"),pos,length))
			{
				tmp=str.substr(pos,length);
				bool left_brac=0;
				for(i=0;i<length;i++)
				{
					if(left_brac)
					{
						if(tmp[i]=='>')
						{
							left_brac=0;
							data.intro+=' ';
						}
					}
					else
					{
						if(tmp[i]=='<'&&i+1<length&&(tmp[i+1]=='/'||tmp[i+1]=='a'||tmp[i+1]=='p'||tmp[i+1]=='b'||tmp[i+1]=='d'))left_brac=1;
						else data.intro+=tmp[i];
					}
				}
			}
			if(match(str,GBKToUTF8("rating-value\">"),GBKToUTF8("</div>"),pos,length,10))data.score=str.substr(pos,length);
			if(match(str,GBKToUTF8("<div class=\"votes\">"),GBKToUTF8("</div>"),pos,length,20))
			{
				tmp=str.substr(pos,length);
				for(i=0;i<tmp.length();i++)
					if(tmp[i]>='0'&&tmp[i]<='9')data.votes+=tmp[i];
			}
			if(match(str,GBKToUTF8("<time itemprop=\"datePublished\">"),GBKToUTF8("</time>"),pos,length,25))data.date=str.substr(pos,length);
			if(match(str,GBKToUTF8("itemprop=\"softwareVersion\">"),GBKToUTF8("</dd>"),pos,length,20))data.ver=str.substr(pos,length);
			if(match(str,GBKToUTF8("版本：</dt><dd>"),GBKToUTF8("</dd><dt>"),pos,length,40))data.android=str.substr(pos,length);
			if(match(str,GBKToUTF8("</dt><dd><a href="),GBKToUTF8("</a></dd>"),pos,length))
			{
				tmp=str.substr(pos,length);
				for(i=0;i<tmp.length();i++)
					if(tmp[i]=='>')break;
				data.category=str.substr(pos+i+1,length-i-1);
			}
			if(match(str,GBKToUTF8("<dd itemprop=\"numDownloads\">"),GBKToUTF8("<d"),pos,length,40))
			{
				tmp=str.substr(pos,length);
				for(i=0;i<tmp.length();i++)
				{
					if(tmp[i]>='0'&&tmp[i]<='9')data.install+=tmp[i];
					else if(tmp[i]==' '||tmp[i]=='-')break;
				}
			}
			if(match(str,GBKToUTF8("<dd itemprop=\"fileSize\">"),GBKToUTF8("</dd>"),pos,length,20))data.size=str.substr(pos,length);
			if(match(str,GBKToUTF8("content=\"0\"></span>"),GBKToUTF8("<span"),pos,length,20))data.price=str.substr(pos,length);
			else if(match(str,GBKToUTF8("itemprop=\"price\" content=\""),GBKToUTF8("\""),pos,length,20))data.price=str.substr(pos,length);
			if(match(str,GBKToUTF8("<dd itemprop=\"contentRating\">"),GBKToUTF8("</dd>"),pos,length,20))data.range=str.substr(pos,length);
			WaitForSingleObject(mutex4out,INFINITE);
			data.output();
			ReleaseMutex(mutex4out);
		}
		else
		{
			WaitForSingleObject(mutex4err,INFINITE);
			outerr<<itemnum<<endl;
			ReleaseMutex(mutex4err);
		}
	}
	return 0;
}

int main()
{
	ifstream fintmp;
	HANDLE handle;
	int i,maxthreadnum;
	char str[MAXLENGTH];
	char destfilename[MAXLENGTH];
	printf("Enter max thread num :\n");
	scanf("%d",&maxthreadnum);
	printf("\nEnter item num :\n");
	scanf("%d",&itemnum);
	printf("\nEnter item num (final):\n");
	scanf("%d",&itemnumfinal);
	printf("\nEnter Dest file name:\n");
	cin.get();
	cin.getline(destfilename,MAXLENGTH);
	fintmp.open(destfilename);
	if(fintmp==NULL)
	{
		outfile.open(destfilename);
		outfile<<(unsigned char)0xEF<<(unsigned char)0xBB<<(unsigned char)0xBF;
		outfile<<GBKToUTF8("序号\t名称\t平均评分\t评分人数\t更新日期\t最新版本\t所需的安卓版本\t类别\t最少安装次数\t大小\t价格\t内容分级\t网页链接\t说明")<<endl;
	}
	else
	{
		fintmp.close();
		outfile.open(destfilename,ios::out|ios::app);
	}
	infile.open("itemlist.txt");
	for(i=1;i<itemnum;i++)
		infile.getline(str,MAXLENGTH);
	outerr.open("err.txt",ios::out|ios::app);
	hsession=InternetOpen((char*)(LPCWSTR)"RookIE/1.0",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if(hsession!=NULL)
	{
		printf("\nConnected Successfully!\n\n");
		mutex4err=mutex4fin=mutex4out=CreateMutex(NULL,false,NULL);
		for(i=1;i<=maxthreadnum;i++)
		{
			handle=CreateThread(NULL,0,download,NULL,0,NULL);
			CloseHandle(handle);
		}
		while(1)
		{
			Sleep(86400000);
		}
	}
	return 0;
}
