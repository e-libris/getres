#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>

#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "procreceipt.h"

using namespace std;

/*******************************************************************************
 * data struct for use with write_data (OLD WRITE FUNCTION)
 ******************************************************************************/
/*
struct bufstruct	{
	char* buffer;
	size_t size;
};
*/

/*******************************************************************************
 * write fuction to pass to libcurl.
 * 
 * code taken from: https://curl.haxx.se/libcurl/c/url2file.html
 ******************************************************************************/
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)	{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

/*******************************************************************************
 * void download(vector<string>&
 * 
 * takes a vector of grocery item codes, searches meijer.com, obtains the url
 * for the associated product image and then retrieves it.  
 ******************************************************************************/ 
void download(vector<string>& v)	{
	
	string url, filename, line;
	bool flag = false;
	size_t found;
	
	CURL* handle;
	FILE* pagefile;

	curl_global_init(CURL_GLOBAL_ALL);
	
	handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);
	// meijer doesn't like curl/linux
	curl_easy_setopt(handle, CURLOPT_USERAGENT, 
			"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:54.0) Gecko/20100101 Firefox/72.0");
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);

	// for each code in the vector, build the search url and download its page
	// to resources/page.out.  open page.out.  if "0 items found", do nothing.
	// else, extract the image url and download the image
	for(auto& i : v)	{
		url = "https://www.meijer.com/shop/en/search/?text=" + i;
		filename = "resources/" + i + ".png";
		curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
		pagefile = fopen("resources/page.txt", "wb");
		if(pagefile)	{
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, pagefile);
			curl_easy_perform(handle);
			fclose(pagefile);
		}
		ifstream f("resources/page.txt");
		while(getline(f,line))	{
			found = line.find("0 items found for keyword");
			if(found != string::npos) {	// found a match
				flag = true;
				break;
			}
		}
		f.close();
		// flag is true if '0 items found'  if flag is false we found a match
		if(!flag)	{
			f.open("resources/page.txt");
			while(getline(f,line))	{
				found = line.find("https://www.meijer.com/content/dam/meijer/product/");
				if(found != string::npos)	{
					line = line.substr(line.find("https"));
					line = line.substr(0, line.find("\" alt=\""));
					curl_easy_setopt(handle, CURLOPT_URL, line.c_str());
					pagefile = fopen(filename.c_str(), "wb");
					if(pagefile)	{
						cout << "downloading: " << line << endl;
						curl_easy_setopt(handle, CURLOPT_WRITEDATA, pagefile);
						curl_easy_perform(handle);
						fclose(pagefile);
					}
						
				}
			}
			f.close();
		}
		flag = false;
	}
		
	curl_easy_cleanup(handle);
	curl_global_cleanup();
	
	remove("resources/page.txt");
	
}

/*******************************************************************************
 * int main()
 * 
 * this program processes meijer receipt files in text format, creates a 
 * single csv file with all pertinent fields.  then, for each item, it attempts
 * to download the item image from the meijer.com website.
 ******************************************************************************/
int main()	{
	
	pid_t pid;
	if((pid = fork()) < 0)  {
		printf("fork error\n");
		exit(-1);
	}
    
    // convert all our pdf files (if any) to txt files.  requires pdftotext
    if(pid == 0)    {
		char cmd[255];
		getcwd(cmd, sizeof(cmd));
		if((strlen(cmd) + 12) < sizeof(cmd)) 
			strcat(cmd, "/convert.sh");	
		execl(cmd, cmd, NULL);		 	     // run convert.sh pdf to txt script
		printf("errno msg: %s\n", strerror(errno));		// should not reach here		
		exit(-1);
    }
    
    // main block of program
    else {
		int status;
		waitpid(pid, &status, 0);
		
		char* str = NULL;		// stores getline reads
		size_t str_size = 0;	// store the size of the line getline
		ssize_t linesize;		// check that we didn't error out
		char* saveptr;			// for strtok
		
		vector<string> links;	// for storing codes which we will lookup on
								// 	the meijer website
								
		struct dirent* entry;	// get an ls of data folder
		DIR* dr = opendir("./data");
		// for each txt file in data directory, pass it to proc_receipt().
		// appends to file master.csv
		truncate("master.csv", 0);
		if(dr)	{
			while((entry = readdir(dr)))	{
				char filename[strlen(entry->d_name)+8];
				strcpy(filename, "./data/");
				strcat(filename, entry->d_name);
				if(strstr(entry->d_name, ".txt"))	{	
					proc_receipt(filename, "master.csv", true);
				}
			}
		}
		
		// open up the master.csv file we just wrote.  for each line, if it's 
		// not line type 0 or line type 7, extract the product code and build
		// the vector.  
		FILE* receipt = fopen("master.csv", "r");
		while((linesize = getline(&str, &str_size, receipt))>0)	{
			
			if(str[0]=='0' || str[0]=='7')
				continue;
			else	{
				char* token = strtok_r(str, ",", &saveptr);
				token = strtok_r(NULL, ",", &saveptr);
				if(token[0]=='*')
					token++;
				links.push_back(token);		
			}
		}
		
		// if resources doesn't exist, create it
		mkdir("./resources", 0755);		  // mkdir returns -1 on fail or exists, 
										  // use stat for more robust handling
		
		// unique works if we sort first, but doesn't alter the container, call
		// erase
		sort(links.begin(), links.end());
		links.erase(unique(links.begin(), links.end()), links.end());
		
		// download each item code resource
		download(links);			
	}
	
	return 0;
}
