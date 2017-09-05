#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#define MAXBLOCKSIZE 1024
#define MAXLENGTH 1024
#define MAXITEMNUM 316000
#define URLPREFIX "https://play.google.com/store/apps/details?id="

#pragma comment(lib,"wininet.lib") 

using namespace std;

HINTERNET hsession;
HANDLE mutex4err,mutex4fin,mutex4out,mutex4tbl;
ofstream outerr,outfile,outtable;
ifstream infile;
int curitem=0;
bool downloaded[MAXITEMNUM]={0};

string UTF8ToGBK(const std::string& strUTF8);
string GBKToUTF8(const std::string& strGBK);
bool match(const string &src,const string &st1,const string &st2,int &pos,int &length,const int limitlen);
DWORD WINAPI download(LPVOID lpparam);

class DATA
{
public:
	int id;				// 序号
	string url;			// 链接
	string permission;	// 权限
	void output()
	{
		outfile<<id<<"\t"<<url<<"\t"<<permission<<endl;
	}
};

class NODE
{
public:
	string title;
	string detail;
	int l,r;
	NODE(){};
	NODE(const string &t,const string &d)
	{
		title=t;
		detail=d;
		l=r=-1;
	}
};

vector<NODE> v;

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

int insert(const string &str1,const string &str2,const bool needout=1)
{
	if(v.size()<=1)
	{
		if(v.size()==1&&needout)outtable<<v.size()<<"\t"<<str1<<"\t"<<str2<<endl;
		v.push_back(NODE(str1,str2));
		return v.size()-1;
	}
	int i=1;
	while(1)
	{
		if(str1<v[i].title)
		{
			if(v[i].l==-1)
			{
				v[i].l=v.size();
				if(needout)outtable<<v.size()<<"\t"<<str1<<"\t"<<str2<<endl;
				v.push_back(NODE(str1,str2));
				return v.size()-1;
			}
			else i=v[i].l;
		}
		else if(str1>v[i].title)
		{
			if(v[i].r==-1)
			{
				v[i].r=v.size();
				if(needout)outtable<<v.size()<<"\t"<<str1<<"\t"<<str2<<endl;
				v.push_back(NODE(str1,str2));
				return v.size()-1;
			}
			else i=v[i].r;
		}
		else
		{
			if(str2==v[i].detail)return i;
			else if(str2<v[i].detail)
			{
				if(v[i].l==-1)
				{
					v[i].l=v.size();
					if(needout)outtable<<v.size()<<"\t"<<str1<<"\t"<<str2<<endl;
					v.push_back(NODE(str1,str2));
					return v.size()-1;
				}
				else i=v[i].l;
			}
			else
			{
				if(v[i].r==-1)
				{
					v[i].r=v.size();
					if(needout)outtable<<v.size()<<"\t"<<str1<<"\t"<<str2<<endl;
					v.push_back(NODE(str1,str2));
					return v.size()-1;
				}
				else i=v[i].r;
			}
		}
	}
}

string toString(int n)
{
	stringstream ss;
	string str;
	ss<<n;
	ss>>str;
	return str;
}

DWORD WINAPI download(LPVOID lpparam)
{
	char url[MAXLENGTH];
	while(1)
	{
		DATA data;
		ULONG number=1;
		int item,trytimes=0,i,j;
		string str;
		WaitForSingleObject(mutex4fin,INFINITE);
		if(infile.peek()==EOF)return 0;
		while(infile.getline(url,MAXLENGTH))
		{
			curitem++;
			if(!downloaded[curitem])break;
		}
		item=curitem;
		printf("%d.%s\n",item,url);
		ReleaseMutex(mutex4fin);
		if(item>=MAXITEMNUM)return 0;
		data.id=item;
		data.url=url;
		HINTERNET handle=NULL;
		handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)data.url.c_str(),NULL,0,INTERNET_FLAG_DONT_CACHE,0);
		while(handle==NULL&&trytimes<3)
		{
			trytimes++;
			handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)data.url.c_str(),NULL,0,INTERNET_FLAG_DONT_CACHE,0);
		}
		if(handle!=NULL)
		{
			string tmp,title,detail;
			int pos,match;
			char readchar[MAXBLOCKSIZE+1];
			while(number>0)
			{
				InternetReadFile(handle,readchar,MAXBLOCKSIZE-1,&number);
				for(i=0;i<number;i++)
					str+=readchar[i];
			}
			InternetCloseHandle(handle);
			str=UTF8ToGBK(str);
//			printf("%s\n",str.c_str());
			const string cmpstr1="<h4 class=\"doc-permissions-header\">";
			const string cmpstr2="<span class=\"doc-permission-group-title\">";
			const string cmpstr3="<div class=\"doc-permission-description\">";
			for(pos=0;pos<str.length()-cmpstr1.length();pos++)
			{
				match=1;
				for(j=0;j<cmpstr1.length();j++)
					if(str[pos+j]!=cmpstr1[j])
					{
						match=0;
						break;
					}
				if(match)break;
			}
			for( ;pos<str.length()-cmpstr2.length(); )
			{
				int pos1,length1;
				match=1;
				for(j=0;j<cmpstr2.length();j++)
					if(str[pos+j]!=cmpstr2[j])
					{
						match=0;
						break;
					}
				if(match==1)
				{
					pos1=pos+cmpstr2.length();
					for(length1=1;pos1+length1<str.length();length1++)
						if(str[pos1+length1]=='<')break;
					title=str.substr(pos1,length1);
					pos=pos1+length1;
				}
				else
				{
					match=2;
					for(j=0;j<cmpstr3.length();j++)
						if(str[pos+j]!=cmpstr3[j])
						{
							match=0;
							break;
						}
					if(match==2)
					{
						int index;
						pos1=pos+cmpstr3.length();
						for(length1=1;pos1+length1<str.length();length1++)
							if(str[pos1+length1]=='<')break;
						detail=str.substr(pos1,length1);
						WaitForSingleObject(mutex4tbl,INFINITE);
						index=insert(title,detail);
						ReleaseMutex(mutex4tbl);
						if(data.permission.length()==0)data.permission=toString(index);
						else data.permission+=";"+toString(index);
						pos=pos1+length1;
					}
					else pos++;
				}
			}
			WaitForSingleObject(mutex4out,INFINITE);
			data.output();
			ReleaseMutex(mutex4out);
		}
		else
		{
			WaitForSingleObject(mutex4err,INFINITE);
			outerr<<item<<endl;
			ReleaseMutex(mutex4err);
		}
	}
	return 0;
}

int main()
{
	ifstream fintmp1,fintmp2,fintmp3;
	HANDLE handle;
	int i,j,k,maxthreadnum;
	char str[MAXLENGTH],title[MAXLENGTH]="\0",detail[MAXLENGTH]="\0";
	char destfilename[MAXLENGTH]="permission.txt",desttablename[MAXLENGTH]="table.txt";
	printf("Enter max thread num :\n");
	scanf("%d",&maxthreadnum);
	fintmp1.open(destfilename);
	if(fintmp1==NULL)
	{
		outfile.open(destfilename);
		outfile<<"序号\t网页链接\t权限"<<endl;
	}
	else
	{
		outfile.open(destfilename,ios::out|ios::app);
		fintmp1.getline(str,MAXLENGTH);
		while(fintmp1.getline(str,MAXLENGTH))
		{
			for(i=0;i<strlen(str);i++)
				if(str[i]=='\t')
				{
					k=0;
					for(j=0;j<i;j++)
						k=k*10+str[j]-'0';
					downloaded[k]=1;
					break;
				}
		}
		fintmp1.close();
	}
	fintmp2.open(desttablename);
	insert("","");
	if(fintmp2==NULL)
	{
		outtable.open(desttablename);
		outtable<<"编号\t分类\t条目"<<endl;
	}
	else
	{
		outtable.open(desttablename,ios::out|ios::app);
		fintmp2.getline(str,MAXLENGTH);
		while(fintmp2.getline(str,MAXLENGTH))
		{
			for(i=0;i<strlen(str);i++)
			{
				if(str[i]=='\t')
				{
					for(j=i+1;j<strlen(str);j++)
						if(str[j]=='\t')
						{
							for(k=i+1;k<j;k++)
							{
								title[k-i-1]=str[k];
							}
							title[k-i-1]='\0';
							for(k=j+1;str[k]!='\r'&&str[k]!='\n'&&str[k]!='\0';k++)
								detail[k-j-1]=str[k];
							detail[k-j-1]='\0';
							break;
						}
					break;
				}
			}
			insert(title,detail,0);
		}
		fintmp2.close();
	}
	infile.open("urllist.txt");
	outerr.open("err.txt",ios::out|ios::app);
	hsession=InternetOpen((char*)(LPCWSTR)"RookIE/1.0",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if(hsession!=NULL)
	{
		printf("\nConnected Successfully!\n\n");
		mutex4err=mutex4fin=mutex4out=mutex4tbl=CreateMutex(NULL,false,NULL);
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
