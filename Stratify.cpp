//Stratify.cpp
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <time.h>
#include "Stratify.h"
#include "Stock.h"
#include <iostream>
#include <iomanip>
#include <numeric>
#include "OperatorOverloading.h"
#include <algorithm>

// para constructor
stratify::stratify(vector<Stock*>& vstock_, string group_name_)
{
	vstock = vstock_;
	group_name = group_name_;
	group_size = vstock.size();
	srand((unsigned int)time(NULL)); //set random seed
}

//generate one bootstrap-sample
vector<double> stratify::OneSample()
{
	vector<double> AAR = vector<double>(60, 0.0);

	for (int i = 0; i < 30; i++)
	{
		int id = rand() % group_size; //random int between 0 and (group_size-1)
		AAR = (i / (i + 1.0)) * AAR + (1 / (i + 1.0)) * ((vstock.at(id))->getAR());//operator overloading
	}
	return AAR;
}

//call function-OneSample 30 times
void stratify::BootStrap()
{
	for (int k = 0; k < 30; k++)
	{
		vector<double> AAR = OneSample();
		// CAAR is the cumulative sum of AAR
		vector<double> CAAR(60, 0.0);
		partial_sum(AAR.begin(), AAR.end(), CAAR.begin());
		// step by step calculation for mean/variance
		Aver_AAR = (k / (k + 1.0)) * Aver_AAR + (1 / (k + 1.0)) * AAR;//operator overloading
		Aver_CAAR = (k / (k + 1.0)) * Aver_CAAR + (1 / (k + 1.0)) * CAAR;
		Sd_AAR = (k / (k + 1.0)) * Sd_AAR + (1 / (k + 1.0)) * pow(AAR, 2.0);
		Sd_CAAR = (k / (k + 1.0)) * Sd_CAAR + (1 / (k + 1.0)) * pow(CAAR, 2.0);
	}
	// final step calculation for standard deviation
	Sd_AAR = pow(Sd_AAR - pow(Aver_AAR, 2.0), 0.5);
	Sd_CAAR = pow(Sd_CAAR - pow(Aver_CAAR, 2.0), 0.5);

}

// show info of the group
void stratify::PullGroupInfo()
{
	cout << endl << "AAR, AAR-SD, CAAR and CAAR-SD of " << group_name << " group :" << endl << endl;

	cout << setiosflags(ios::fixed) << setprecision(4); 
	cout << setiosflags(ios::right) <<
		setw(10) << "Date" << "|" <<
		setw(10) << "AAR" << "|" <<
		setw(10) << "AAR-SD" << "|" <<
		setw(10) << "CAAR" << "|" <<
		setw(10) << "CAAR-SD" << endl;

	for (int i = -29; i <= 30; i++)
		cout << setiosflags(ios::right) <<
		setw(10) << i << "|" <<
		setw(10) << Aver_AAR.at(i + 29) << "|" <<
		setw(10) << Sd_AAR.at(i + 29) << "|" <<
		setw(10) << Aver_CAAR.at(i + 29) << "|" <<
		setw(10) << Sd_CAAR.at(i + 29) << endl;

}

// compare two "pointer to Stock" based on "surprise" in "Stock" object
bool cmp_by_value(Stock*& lhs, Stock*& rhs)
{
	return lhs->getSurprise() < rhs->getSurprise();
}

// data structure transformation, surprise sorting, stratify objects initialization, AR calculation & Bootstrap
void Stratify_Calculate_Bootstrap(map<string, Stock>& stock_map_, ETF& spy,
	stratify& Beat, stratify& Meet, stratify& Miss)
{
	vector<Stock*> BeatList, MeetList, MissList;
	// 1. data structure transformation
	vector<Stock*> stocklist;
	for (auto it = stock_map_.begin(); it != stock_map_.end(); it++)
	{
		/*
		int temp = it->second.getPrice().size();
		if (temp != 61)
		{
			cout << "Debug: wrong price vector size" << endl;
			cout << it->first << "   " << temp << endl;
		}
		*/
		stocklist.push_back(&(it->second));
	}

	// 2. surprise sorting
	sort(stocklist.begin(), stocklist.end(), cmp_by_value);

	// 3. stratify objects initialization
	//int StockNum = stocklist.size();
	int itcount = 0;
	for (auto it = stocklist.begin(); it != stocklist.end(); it++, itcount++)
	{
		// first 109 SPY stocks have negative surprise
		if (itcount < 109) //(StockNum / 3)
		{
			MissList.push_back(*it);
			(*it)->SetGroup("Miss");

		} // 110-261 SPY stocks have positive surprise smaller than 0.05
		else if (itcount < 261) //(2 * StockNum / 3)
		{
			MeetList.push_back(*it);
			(*it)->SetGroup("Meet");
		}
		else // other SPY stocks have positive surprise larger than 0.05
		{
			BeatList.push_back(*it);
			(*it)->SetGroup("Beat");
		}
		(*it)->SetReturn();
		(*it)->SetAR(spy); // 4. AR calculation
	}
	Beat = stratify(BeatList, "Beat");
	Meet = stratify(MeetList, "Meet");
	Miss = stratify(MissList, "Miss");

	// 5. Bootstrap
	Beat.BootStrap();
	Meet.BootStrap();
	Miss.BootStrap();
}

// plot CAAR of all 3 groups utilzing Gnuplot
void plotResults(const char* GnuplotPath, stratify& Beat_, stratify& Meet_, stratify& Miss_) {


	vector<double> Beat_CAAR = Beat_.Get_Aver_CAAR();
	vector<double> Meet_CAAR = Meet_.Get_Aver_CAAR();
	vector<double> Miss_CAAR = Miss_.Get_Aver_CAAR();
	//cout << "Debug: "<<Beat_CAAR.size() << endl;

	double* xData = (double*)malloc(60 * sizeof(double));
	double* yData1 = (double*)malloc(60 * sizeof(double));
	double* yData2 = (double*)malloc(60 * sizeof(double));
	double* yData3 = (double*)malloc(60 * sizeof(double));
	for (int i = 0; i <= 59; i++) {
		xData[i] = double(i);
		yData1[i] = Beat_CAAR.at(i);
		yData2[i] = Meet_CAAR.at(i);
		yData3[i] = Miss_CAAR.at(i);
	}

	FILE* gnuplotPipe, * tempDataFile;
	const char* BeatData = "Beat Group";
	const char* MeetData = "Meet Group";
	const char* MissData = "Miss Group";
	double x1, y1, x2, y2, x3, y3;

	gnuplotPipe = _popen(GnuplotPath, "w");// Gnuplot path in Windows 
	if (gnuplotPipe) {
		fprintf(gnuplotPipe, "plot \"%s\" with lines, \"%s\" with lines, \"%s\" with lines\n",
			BeatData, MeetData, MissData);
		fflush(gnuplotPipe);

		tempDataFile = fopen(BeatData, "w");
		for (int i = 0; i <= 59; i++) {
			x1 = xData[i];
			y1 = yData1[i];
			fprintf(tempDataFile, "%lf %lf\n", x1, y1);
		}
		fclose(tempDataFile);

		tempDataFile = fopen(MeetData, "w");
		for (int i = 0; i <= 59; i++) {
			x2 = xData[i];
			y2 = yData2[i];
			fprintf(tempDataFile, "%lf %lf\n", x2, y2);
		}
		fclose(tempDataFile);

		tempDataFile = fopen(MissData, "w");
		for (int i = 0; i <= 59; i++) {
			x3 = xData[i];
			y3 = yData3[i];
			fprintf(tempDataFile, "%lf %lf\n", x3, y3);
		}
		fclose(tempDataFile);
		// dont write following codes or you cannot use this function in the loop
		/*
		remove(BeatData);
		remove(MeetData);
		remove(MissData);
		*/
		// dont write following codes or undesirable output while exiting
		//fprintf(gnuplotPipe, "exit \n");
		cout << "Successfully printed." << endl;
	}
	else {
		printf("gnuplot not found...");
	}
}