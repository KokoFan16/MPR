/*
 * csv_write.cpp
 *
 *      Author: kokofan
 */

#include "csv_writer.hpp"

CSVWrite& CSVWrite::write (const string & val){
	if (!begin){ fs << delimiter; }
	else { begin = false; }
	fs << val;
	return *this;
}

CSVWrite::CSVWrite(const string filename, const string deli): fs(), delimiter(deli), begin(true) {
	fs.exceptions(ios::failbit | ios::badbit);
	fs.open(filename); // open file
}

CSVWrite::~CSVWrite() { fs.flush(); fs.close(); } // flush and close file

void CSVWrite::endrow() { fs << endl; begin = true; } // end of each line

CSVWrite& CSVWrite::operator << ( CSVWrite& (* val)(CSVWrite&)) { return val(*this); } // overwrite operator <<

CSVWrite& CSVWrite::operator << (const std::string & val) { return write(val); } // write string






