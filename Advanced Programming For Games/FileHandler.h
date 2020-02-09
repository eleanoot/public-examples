/*
Author: Eleanor Gregory
Date: Oct 2019

A class to manage reading and writing of files 
without extended disk time opening and closing them.

/ᐠ .ᆺ. ᐟ\ﾉ

*/

#pragma once
#include <fstream>
#include <string>
#include "Puzzle.h"
using namespace std;

enum MODE
{
	READ,
	WRITE,
};

class FileHandler
{
public:
	
	FileHandler();
	FileHandler(string name);
	~FileHandler();

	void set_file_name(string name);

	void open(MODE mode = READ);
	void close();

	template <class T> void write(const T& toWrite) throw (runtime_error)
	{
		if (!out.is_open())
		{
			throw runtime_error("File not open to write to");
		}
		out << toWrite << endl;
	}

	void read_int(int& location);
	void read_line(string& location);
	void switch_mode(MODE mode);

	void reset_file();

	bool write_is_open();
	bool read_is_open();


private:
	string fileName;
	ifstream in;
	ofstream out;
	MODE current_mode;	
};

