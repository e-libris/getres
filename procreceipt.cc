#include "procreceipt.h"

/*******************************************************
* bool starornum(char)
* 
* return true if the character passed in is a num (0-9)
* or an asterisk
*******************************************************/
bool starornum(char c)	{
	return ((int)c == 42 || ((int)c>=48 && (int)c<=57));
}

/*******************************************************
* string addspaces(string, string)
* 
* adds a space to a meijer receipt line containing 
* characters '@' and '/'.  useful formatting for text
* processing
*******************************************************/
string addspaces(string s, string s2)	{
	size_t found = s.find(s2);
	if(found != string::npos)	{
		if(!isspace(s[found-1]))
			s.insert(found, " ");
		if(!isspace(s[found+1]))
			s.insert(found+1, " ");
	}
	return s;
}

/*******************************************************
* string proc_line(string, int)
* 
* does the bulk of receipt processing.  takes a string
* which is a line from a meijer receipt, and an int 
* representing the line type (0-7, see notes.txt). 
* returns a csv formatted line.
*******************************************************/
string proc_line(string line, int n)	{
	vector<string> v;
	stringstream ss(line);
	while(ss>>line)
		v.push_back(line);
		
	switch(n)	{
		// date/cashier line.  also denotes beginning of a logical receipt in master.csv
		case 0:		
			line = "0," + v[0] + "," + v.back();
			break;
		// type 1: logical item contained on 1 line
		case 1:
			line = "1," + v[0] + ",";
			for(size_t i = 1; i<v.size()-2; i++)
				line = line + v[i] + " ";
			line = line.substr(0, line.length()-1) + ",1," 
				   + v[v.size()-2] + "," + "NULL," + v[v.size()-2];
			break;
		case 2:
			line = "2," + v[5] + ",";
			for(size_t i = 6; i<v.size(); i++)
				line = line + v[i] + " ";				
			line = line.substr(0, line.length()-1) + ",1," 
				   + v[3] + "," + v[1] + "," + v[3];
			break;
		case 3:
			line = "3," + v[5] + ",";
			for(size_t i = 6; i<v.size(); i++)
				line = line + v[i] + " ";
			line = line.substr(0, line.length()-1) + "," 
			       + v[0] + "," + v[3] + ",NULL," + v[2];
			break;
		case 4:
			line = "4," + v[7] + ",";
			for(size_t i = 8; i<v.size(); i++)
				line = line + v[i] + " ";
			line = line.substr(0, line.length()-1) + "," 
				   + v[0] + "," + v[5] + ",NULL," + v[2] + v[3] + v[4];
			break;
		case 5:
			line = "5," + v[8] + ",";
			for(size_t i = 9; i<v.size(); i++)
				line = line + v[i] + " ";
			line = line.substr(0, line.length()-1) + "," 
				   + v[5] + "," + v[3] + "," + v[1] + "," + v[7];
			break;
		case 6:
			line = "6," + v[10] + ",";
			for(size_t i =11; i<v.size(); i++)
				line = line + v[i] + " ";
			line = line.substr(0, line.length()-1) + "," 
				   + v[5] + "," + v[3] + "," + v[1] + "," + v[7] + v[8] + v[9];
			break;
		default:
			line = "ERR";
	}
	
    ifstream f("limited.db");
	string code, lookup;
	bool flag = false;
	ss.clear();
	ss.str("");
	ss<<line;
	getline(ss, code, ',');
	getline(ss, code, ',');
	if(code[0]=='*')
		code = code.substr(1);
	
	while(getline(f,lookup))	{
		ss.clear();
		ss.str("");
		ss<<lookup;
		getline(ss, lookup, ',');
		if(code==lookup)	{
			getline(ss, lookup, ',');
			getline(ss, lookup, ',');
			getline(ss, lookup, ',');
			line = line + "," + lookup;
			flag = true;
			break;
		}
		else;
	}
	if (!flag && line[0] != '0' && line[0] != '7')
		line = line + ",99";
	
	return line;
}

////////////////////////////////////////////////////////////////////////////////
// process an entire receipt.
// 'in' represents the input file, 'out' the output file
// if true, 'b' opens the file for appending, as in the case of building master
// receipts
// else it is processing a single file, don't append.
////////////////////////////////////////////////////////////////////////////////
void proc_receipt(string in, string out, bool b)	{
	
	string line, line2, tax, total;
	string savings = "0.00";
	stringstream ss;
	size_t found;
	
	// receipt file for processing.  this should be a processed pdf, in txt form
	ifstream f(in);
	// file to output our formatted csv to.  
	ofstream o;
	// if true, append (master receipts).  if false, do not
	b ? o.open(out, ofstream::app) : o.open(out);
	
	getline(f, line);
	getline(f, line);
	line2 = line;
	getline(f, line);
	line = line2 + " " + line;
	
	// create a type 0 line.  contains date, cashier name.  type 0 lines 
	// indicate the beginning of a new receipt in both receipt types.
	o << proc_line(line, 0) << endl;
	
	// proceed to SALE portion of meijer receipt
	while(line!="SALE")	{
		getline(f, line);
		// if we find a SAVINGS TOTAL line along the way, we record the value
		if((found = line.find("SAVINGS TOTAL"))!=string::npos)	{
			ss<<line;
			while(ss>>line);
			savings = line;
		}
	}
	
	// we reached SALE, proceed 1 line
	getline(f, line);
	// until we reach the TOTAL portion of the receipt, process lines
	// individual items and their information reside in this portion
	while(line != "TOTAL")	{
		// disregard empty lines
		while(line.empty())
			getline(f, line);
		// first character of the line is a '*' or '0-9' char, and the line
		// length is greater than forty.  we found a type 1 line.
		if(starornum(line[0]) && line.length()>40)	{
			// ignore coupon lines
			if((found = line.find("Vendor Coupon"))!=string::npos);
			else 
				o << proc_line(line, 1) << endl;
		}
		// first char is '*' or '0-9', line length is less than forty
		// this line has information about its item on subsequent lines
		// do more processing to discover what case		
		else if(starornum(line[0]) && line.length()<40) {
			// proceed 1 line
			getline(f, line2);
			// line 2 contains a '@' or a '/'.  do some format normalizing
			// to make later processing easier.
			line2 = addspaces(line2, "@");
			line2 = addspaces(line2, "/");
			
			if(line2.length() > 40)	{
				line = line2 + " " + line;
				// found "was", line is type 2
				if((found = line2.find("was"))!=string::npos)	{
					o << proc_line(line, 2) << endl;
				}
				else if((found = line2.find("@"))!=string::npos)	{
					if((found = line2.find("/"))!=string::npos)	{
						// found '@' and '/', type 4
						o << proc_line(line, 4) << endl;
					}
					else	{
						// found '@' but NOT '/', type 3
						o << proc_line(line, 3) << endl;
					}
				}
			}
			// didn't find '@'
			else	{
				int num;
				// no '@', but found a '/'?  type 6
				// else no '@' and no '/', type 5
				(found = line2.find("/")) != string::npos ? num = 6 : num = 5;
				line = line2 + " " + line;
				getline(f, line2);
				line = line2 + " " + line;
				o << proc_line(line, num) << endl;
			}
		}
		getline(f, line);
	}
	// line now contains TOTAL
	while((found = line.find("TOTAL TAX"))==string::npos)
		getline(f, line);
	ss.str("");
	ss.clear();
	ss<<line;
	while(ss>>line);
	tax = line;
	getline(f, line);
	ss.str("");
	ss.clear();
	ss<<line;
	while(ss>>line);
	total = line;
	o << "7," << savings << "," << tax << "," << total << endl;
}

void tempfunc(void)	{
	cout << "hi\n";
}
