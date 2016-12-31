#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include <string>
#include <fstream>

#define MAXBLOCKSIZE 1024
#define MAXLENGTH 1024
#define MAXITEMNUM 500000
#define MAXPAGESIZE 500000
#define FIRSTITEM "com.anguanjia.safe"
#define GOOGLEPLAYPREFIX "https://play.google.com/store/apps/details?id="
#define CMPSTR "apps/details?id="

#pragma comment(lib,"wininet.lib") 

using namespace std;

HANDLE mutex4threadnum,mutex4insert,mutex4output,mutex4itemnum,mutex4err;
HINTERNET hsession;
int threadnum=0,readitemnum=0,finditemnum=0;

class NODE
{
public:
	int l,r;
	string item;
};

NODE item[MAXITEMNUM];
ofstream outfile,outerr;

bool insert(string itemname)
{
	if(readitemnum==0)
	{
		item[0].item=itemname;
		item[0].l=item[0].r=-1;
		return 1;
	}
	int i=0;
	int length=itemname.length();
	while(i>=0)
	{
		int l1=item[i].item.length();
		int cmp=0;
		if(length<l1)cmp=-1;
		else if(length>l1)cmp=1;
		else
		{
			if(itemname==item[i].item)cmp=0;
			else if(itemname<item[i].item)cmp=-1;
			else cmp=1;
		}
		if(cmp==0)return 0;
		else if(cmp==-1)
		{
			if(item[i].l!=-1)i=item[i].l;
			else
			{
				item[i].l=readitemnum;
				item[readitemnum].item=itemname;
				item[readitemnum].l=item[readitemnum].r=-1;
				return 1;
			}
		}
		else
		{
			if(item[i].r!=-1)i=item[i].r;
			else
			{
				item[i].r=readitemnum;
				item[readitemnum].item=itemname;
				item[readitemnum].l=item[readitemnum].r=-1;
				return 1;
			}
		}
		cmp=0;
	}
	return 0;
}

DWORD WINAPI download(LPVOID lpparam)
{
	int itemnum,i;
	char readchar[MAXBLOCKSIZE+1];
	char url[MAXLENGTH]=GOOGLEPLAYPREFIX;
	char str[MAXPAGESIZE]="\0";
	int trytimes=0,prefixlen=strlen(CMPSTR),totallength=0;
	ULONG number=1;
	WaitForSingleObject(mutex4itemnum,INFINITE);
//	if(finditemnum%20==0)
	{
		printf("%d.  name = %s  \nnumber of items in list = %d\nnumber of threads = %d\n\n",finditemnum,item[finditemnum].item.c_str(),readitemnum,threadnum);
	}
	itemnum=finditemnum;
	finditemnum++;
	ReleaseMutex(mutex4itemnum);
	strcat(url,item[itemnum].item.c_str());
	HINTERNET handle=NULL;
	handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)url,NULL,0,INTERNET_FLAG_DONT_CACHE,0);
	while(handle==NULL&&trytimes<3)
	{
		trytimes++;
		handle=InternetOpenUrl(hsession,(char*)(LPCWSTR)url,NULL,0,INTERNET_FLAG_DONT_CACHE,0);
	}
	if(handle!=NULL)
	{
		while(number>0)
		{
			InternetReadFile(handle,readchar,MAXBLOCKSIZE-1,&number);
			if(totallength+number<MAXPAGESIZE)
			{
				for(i=0;i<number;i++)
					str[totallength+i]=readchar[i];
				totallength+=number;
			}
			else
			{
				totallength=MAXPAGESIZE;
			}
		}
		InternetCloseHandle(handle);
		for(i=0;i<totallength-prefixlen;i++)
		{
			if(!strncmp(str+i,CMPSTR,prefixlen))
			{
				int j=i+prefixlen;
				while(j<totallength)
				{
					if(!((str[j]>='0'&&str[j]<='9')||(str[j]>='a'&&str[j]<='z')||(str[j]>='A'&&str[j]<='Z')||str[j]=='.'||str[j]=='_'))break;
					j++;
				}
				string ins=str+i+prefixlen;
				ins=ins.substr(0,j-i-prefixlen);
				if(ins!=item[itemnum].item)
				{
					WaitForSingleObject(mutex4insert,INFINITE);
					if(insert(ins))
					{
						readitemnum++;
						WaitForSingleObject(mutex4output,INFINITE);
						outfile<<ins<<endl;
						ReleaseMutex(mutex4output);
					}
					ReleaseMutex(mutex4insert);
				}
				i=j;
			}
		}
	}
	else
	{
		WaitForSingleObject(mutex4err,INFINITE);
		outerr<<itemnum<<endl;
		ReleaseMutex(mutex4err);
	}
	WaitForSingleObject(mutex4threadnum,INFINITE);
	threadnum--;
	ReleaseMutex(mutex4threadnum);
	return 0;
}

int main()
{
	HANDLE handle;
	ifstream infile4num,infile4set;
	int maxthreadnum;
	printf("Enter max thread number:\n");
	scanf("%d",&maxthreadnum);
/*	infile4num.open("itemnum.txt");
	if(infile4num!=NULL)
	{
		infile4num>>finditemnum;
		infile4num.close();*/
	printf("Enter find item number:\n");
	scanf("%d",&finditemnum);
	infile4set.open("itemlist.txt");
	if(infile4set!=NULL)
	{
		char readitemname[MAXLENGTH];
		while(infile4set.getline(readitemname,MAXLENGTH))
		{
			insert(readitemname);
			readitemnum++;
		}
		infile4set.close();
	}
//	}
	if(readitemnum==0)
	{
		ofstream outfile4num;
		outfile4num.open("itemnum.txt");
		outfile4num<<0;
		outfile4num.close();
		outfile.open("itemlist.txt");
		outfile<<FIRSTITEM<<endl;
		insert(FIRSTITEM);
		readitemnum++;
	}
	else
	{
		outfile.open("itemlist.txt",ios::out|ios::app);
	}
	outerr.open("err.txt",ios::out|ios::app);
	hsession=InternetOpen((char*)(LPCWSTR)"RookIE/1.0",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if(hsession!=NULL)
	{
		printf("Connected to server successfully!\n");
		mutex4threadnum=mutex4insert=mutex4output=mutex4itemnum=mutex4err=CreateMutex(NULL,false,NULL);
		while(1)
		{
			while(threadnum>=maxthreadnum)
			{
				Sleep(50);
			}
			while(finditemnum>=readitemnum)
			{
				Sleep(50);
			}
			handle=CreateThread(NULL,0,download,NULL,0,NULL);
			if(handle!=NULL)
			{
				CloseHandle(handle);
				WaitForSingleObject(mutex4threadnum,INFINITE);
				threadnum++;
				ReleaseMutex(mutex4threadnum);
			}
		}
	}
	outfile.close();
	return 0;
}

/*
90929.  name = com.digitaldreams.livetrainstatus
number of items in list = 197456 , number of threads = 80
*/