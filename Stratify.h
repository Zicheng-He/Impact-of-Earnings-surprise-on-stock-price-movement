#pragma once
#ifndef Stratify_h
#define Stratify_h

#include <vector>
#include "Stock.h"
using namespace std;

class stratify
{
private:
	vector<Stock*> vstock; // vector of Stock* storing pointers to stock in this group 
	string group_name; // "Beat", "Meet" or "Miss" 
	int group_size; // number of stocks in the group
	vector<double> Aver_AAR = vector<double>(60, 0.0);
	vector<double> Sd_AAR = vector<double>(60, 0.0);
	vector<double> Aver_CAAR = vector<double>(60, 0.0);
	vector<double> Sd_CAAR = vector<double>(60, 0.0);

public:
	stratify() { group_size = 0; } // default constructor
	stratify(vector<Stock*>& vstock_, string group_name_); // para constructor
	vector<double> OneSample(); //generate one bootstrap-sample
	void BootStrap(); //call function-OneSample 30 times
	void PullGroupInfo(); // show info of the group
	vector<double> Get_Aver_CAAR() { return Aver_CAAR; }
};

// compare two "pointer to Stock" based on "surprise" in "Stock" object
bool cmp_by_value(Stock*& lhs, Stock*& rhs);

// data structure transformation, stratify objects initialization, AR calculation & Bootstrap
void Stratify_Calculate_Bootstrap(map<string, Stock>& stock_map_, ETF& spy, stratify& Beat, stratify& Meet, stratify& Miss);

// plot CAAR of all 3 groups utilzing Gnuplot
void plotResults(const char* GnuplotPath, stratify& Beat_, stratify& Meet_, stratify& Miss_);

#endif // Stratify_h

