#ifndef CSV_WRITER_H_
#define CSV_WRITER_H_

#include <iostream>
#include <string>
#include <fstream>

using namespace std;

class CSVWrite {

private:
    std::ofstream fs;
    const std::string delimiter;
    bool begin;

    /// write string
	CSVWrite& write (const string & val);

public:
	CSVWrite(const string filename, const string deli = ",");

    ~CSVWrite(); // flush and close file

	void endrow(); // end of each line

	CSVWrite& operator << ( CSVWrite& (* val)(CSVWrite&)); // overwrite operator <<

	CSVWrite& operator << (const std::string & val); // write string
};

inline static CSVWrite& endrow(CSVWrite& file) {
    file.endrow();
    return file;
}

#endif /* CSV_WRITER_H */
