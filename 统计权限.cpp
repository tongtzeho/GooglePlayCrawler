#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
	ifstream infile;
	char str[10000];
	int cnt[300]={0},i;
	infile.open("permission.txt");
	infile.getline(str,10000);
	while(infile.getline(str,10000,'\t'))
	{
		infile.getline(str,10000,'\t');
		infile.getline(str,10000);
		int k=0;
		for(i=0;i<strlen(str);i++)
		{
			if(str[i]!=';')
			{
				k=k*10+str[i]-'0';
			}
			if(str[i]==';'||i==strlen(str)-1)
			{
				cnt[k]++;
				k=0;
			}
		}
	}
	infile.close();
	for(i=1;i<=299;i++)
		cout<<i<<"\t"<<cnt[i]<<endl;
	return 0;
}
