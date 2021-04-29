#include <iostream>
#include "../ListPrograms/InstalledPrograms.h"
using namespace std;

int main()
{
	system("pause");
	vector<Software> list = InstalledPrograms::GetInstalledPrograms();
	wcout <<"size = "<<list.size()<<endl;
	for(vector<Software>::iterator iter = list.begin(); iter!=list.end(); iter++) {
		wcout<<iter->InstallLocation<<L"  "<<iter->Version<<L"  "<<iter->Icon<<L"  "<<iter->Architecture<<endl;
	}
	system("pause");
}