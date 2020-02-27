#ifndef PROCRECEIPT_H
#define PROCRECEIPT_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
//#include <filesystem>
#include <stdio.h>
#include <string.h>
#include <cctype>

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::stringstream;
using std::ifstream;
using std::ofstream;

//namespace fs = std::filesystem;

bool starornum(char);
//void tokenize(vector<string>*, string);
string proc_line(string, int);
void proc_receipt(string, string, bool);

void tempfunc(void);

#endif /* READRECEIPT_H */
