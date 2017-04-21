/*
 * CmdSetUtility utility methods
 * Copyright (c) 2016 Shenghua Su
 *
 * build:  g++ -o f2sh file2StrHeader.cpp
 * export: ./f2sh -i inputfile -o outputfile
 *
 */

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main( int argc, const char* argv[])
{
	bool formatErr = false;

	do { // flow control

		if (argc != 5) {
			formatErr = true;
			break;
		}

		int flagCount = (argc - 1) / 2;
		int inputFileNameArgIndex = -1;
		int outputFileNameArgIndex = -1;

		for (int i = 0; i < flagCount; ++i) {

			const char *flag = argv[1 + i * 2];
			int fileNameArgIndex = 2 + i * 2;

			if (string(flag) == "-i")
				inputFileNameArgIndex = fileNameArgIndex;
			else if (string(flag) == "-o")
				outputFileNameArgIndex = fileNameArgIndex;
			else {
				formatErr = true;
				break;
			}
		}
		if (formatErr) break;

		if (inputFileNameArgIndex == -1 || outputFileNameArgIndex == -1) {
			formatErr = true;
			break;
		}

	    std::fstream fi(argv[inputFileNameArgIndex], std::fstream::in);
	    std::fstream fo(argv[outputFileNameArgIndex], std::fstream::out);
	    std::string linestr;
    	while (!fi.eof()) {
    		std::getline(fi, linestr);
    		if (linestr.length() > 0) {
    			fo << "\"";
    			fo << linestr;
    			fo << "\\n\"";
    		}
    		if (fi.peek() != EOF) {
    			fo << "\\\n";
    		}
		}

	    fi.close();
	    fo.close();

	} while (false); // end of flow control

	if (formatErr) {
		std::cout << "use format: " << argv[0] << " -i inputFile -o outputFile" << std::endl;
		return -1;
	}
	else
		return 0;
}
