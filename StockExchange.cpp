// StockExchange.cpp : main project file.
#pragma once
#include "stdafx.h" // should be removed, if not windows platform
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>

#pragma warning (disable:4996)
using namespace std;

#define MICROSEC_PER_DAY 86400000000 // how many microseconds are in one day: 1000 x 1000 x 60 x 60 x 24
//##################################################################################################################
// CTradeInfo class - represents the particular symbol trade
//##################################################################################################################
class CTradeInfo
{
public:

	CTradeInfo()
	{
		Reset();
	}

	~CTradeInfo()
	{
		m_PriceMap.clear();
	}

	void CalculateNewValues(const bool & bIsItFirstTime,
		const unsigned long long & lCurrentTimeStamp,
		const int & iCurrentQuantity,
		const int & iCurrentPrice);

	void Reset() // set everything to the default
	{
		m_lMaxTimeGap = 0;
		m_iTotalVolumeTreaded = 0;
		m_iMaxTradedPrice = 0;
		m_iWeightedAveragePrice = 0;

		m_bSomethingWrong = false;
		m_lPrevTimeStamp = 0;

		m_PriceMap.clear();
	}

	unsigned long long GetMaxTimeGap()
	{
		return m_lMaxTimeGap;
	}

	int GetTotalVolumeTreaded()
	{
		return m_iTotalVolumeTreaded;
	}

	int GetMaxTradedPrice()
	{
		return m_iMaxTradedPrice;
	}

	int GetWeightedAveragePrice()
	{
		return m_iWeightedAveragePrice;
	}

	bool IsSomethingWrong()
	{
		return m_bSomethingWrong;
	}

private:

	unsigned long long m_lMaxTimeGap;
	int m_iTotalVolumeTreaded;
	int m_iMaxTradedPrice;
	int m_iWeightedAveragePrice;

	bool m_bSomethingWrong;
	unsigned long long m_lPrevTimeStamp;

	// to calculate Weighted Average Price:
	map<int, int> m_PriceMap;
	pair<map<int, int>::iterator, bool> m_ret;
	map<int, int>::iterator m_it;

	void CalcMaxTimeGap(const unsigned long long & lCurrentTimeStamp);
	void CalcTotalVolumeTreaded(const int & iCurrentQuantity);
	void CalcMaxTradedPrice(const int & iCurrentPrice);
	void CalcWeightedAveragePrice(const int & iCurrentQuantity, const int & iCurrentPrice);
};
//##################################################################################################################
void CTradeInfo::CalculateNewValues(const bool & bIsItFirstTime, const unsigned long long & lCurrentTimeStamp, const int & iCurrentQuantity, const int & iCurrentPrice)
{
	if (!bIsItFirstTime) // the time gap should be o, if some particual trade symbol is used only once
		CalcMaxTimeGap(lCurrentTimeStamp);
	else
		m_lPrevTimeStamp = lCurrentTimeStamp;

	CalcMaxTradedPrice(iCurrentPrice);
	CalcWeightedAveragePrice(iCurrentQuantity, iCurrentPrice);
}
//##################################################################################################################
void CTradeInfo::CalcMaxTimeGap(const unsigned long long & lCurrentTimeStamp)
{
	unsigned long long lCurrentTimeGap;
	//..................................................................................................................
	if (m_bSomethingWrong) return;

	if (lCurrentTimeStamp < m_lPrevTimeStamp) // an extra security
	{
		m_bSomethingWrong = true;
		return;
	}

	lCurrentTimeGap = lCurrentTimeStamp - m_lPrevTimeStamp;
	if (lCurrentTimeGap > m_lMaxTimeGap)
	{
		m_lMaxTimeGap = lCurrentTimeGap;
	}

	m_lPrevTimeStamp = lCurrentTimeStamp;
}
//##################################################################################################################
void CTradeInfo::CalcTotalVolumeTreaded(const int & iCurrentQuantity)
{
	if (m_bSomethingWrong) return;

	m_iTotalVolumeTreaded = m_iTotalVolumeTreaded + iCurrentQuantity;
}
//##################################################################################################################
void CTradeInfo::CalcMaxTradedPrice(const int & iCurrentPrice)
{
	if (m_bSomethingWrong) return;

	if (m_iMaxTradedPrice < iCurrentPrice)
	{
		m_iMaxTradedPrice = iCurrentPrice;
	}
}
//##################################################################################################################
void CTradeInfo::CalcWeightedAveragePrice(const int & iCurrentQuantity, const int & iCurrentPrice)
{
	int iSum;
	//..................................................................................................................
	if (m_bSomethingWrong) return;

	CalcTotalVolumeTreaded(iCurrentQuantity); // increase iTotalVolumeTreaded here

	if (m_iTotalVolumeTreaded <= 0)
	{
		m_bSomethingWrong = true;
		return;
	}

	// Then we calculate weighted average price:

	m_ret = m_PriceMap.insert(pair<int, int>(iCurrentPrice, iCurrentQuantity));
	if (!m_ret.second) // we already have that element
	{
		m_ret.first->second = m_ret.first->second + iCurrentQuantity; // add quantity to the same price entry
	}

	// Go through the map

	iSum = 0;

	for (m_it = m_PriceMap.begin(); m_it != m_PriceMap.end(); ++m_it)
	{
		iSum = iSum + (m_it->first * m_it->second);
	}

	m_iWeightedAveragePrice = iSum / m_iTotalVolumeTreaded;
}
//##################################################################################################################
// Global variables and structures:
//##################################################################################################################
map<string, CTradeInfo> gTradeInfoMap;
pair<map<string, CTradeInfo>::iterator, bool> g_ret;
map<string, CTradeInfo>::iterator g_it;
FILE *g_pf;
//##################################################################################################################
// Global Functions:
//##################################################################################################################
void MakeMapFile()
{
	// put information from the map into output file
	g_pf = fopen("output.csv", "w");

	if (!g_pf)
	{
		cout << "Cannot create output file!\n";
		return;
	}

	// Go through the map

	for (g_it = gTradeInfoMap.begin(); g_it != gTradeInfoMap.end(); ++g_it)
	{
		fprintf(g_pf, "%s,%llu,%d,%d,%d\n",
			g_it->first.c_str(),
			g_it->second.GetMaxTimeGap(),
			g_it->second.GetTotalVolumeTreaded(),
			g_it->second.GetWeightedAveragePrice(),
			g_it->second.GetMaxTradedPrice());
	}

	fclose(g_pf);
}
//#######################################################################################################################
bool CheckCorrectness(const unsigned long long & lCurrentTimeStamp,
	const unsigned long long & lPrevTimeStamp,
	const string & sCurrentTrade,
	const int & iCurrentQuantity,
	const int & iCurrentPrice)
{
	// returns true, if everything correct, else returns false

	if (lCurrentTimeStamp > MICROSEC_PER_DAY) return false; // we try to count more than one day long
	if (sCurrentTrade.length() != 3) return false; // wrong trade symbol length
	if (iCurrentQuantity <= 0) return false;
	if (iCurrentPrice <= 0) return false;
	if (lPrevTimeStamp > lCurrentTimeStamp) return false;
	return true;
}
//##################################################################################################################
void CleanAll()
{
	// release all global resources
	fclose(g_pf);
	MakeMapFile(); // write the good portion into the output file, of write everything, if success
	gTradeInfoMap.clear();
	getchar();
}
//##################################################################################################################
int main()
{
	int iScanRes;
	int iCurrentStringNumber;
	unsigned long long lCurrentTimeStamp;
	unsigned long long lPrevTimeStamp;
	string sCurrentTrade;
	int iCurrentQuantity;
	int iCurrentPrice;
	CTradeInfo CurrentTradeInfo;
	string sFileName;
	//....................................................................................................................
	// wait for input file name to be entered:
	cout << "Please enter input file name:\n";
	getline(cin, sFileName);
	//....................................................................................................................
	// Open the file:
	g_pf = fopen(sFileName.c_str(), "r");
	if (!g_pf)
	{
		cout << "Cannot open input file!\n";
		cout << "Press any key...\n";
		getchar();
		return 0;
	}

	iCurrentStringNumber = 1;
	lPrevTimeStamp = 0;

	//Read the file:
	while (1)
	{
		char Buffer[100]; // in case if something is wrong in the file string
		iScanRes = fscanf(g_pf, "%llu,%[^,],%d,%d", &lCurrentTimeStamp, Buffer, &iCurrentQuantity, &iCurrentPrice);

		if (iScanRes == EOF)
			break;

		sCurrentTrade = Buffer;

		// See if everything is correct when we read the next string:
		if ((iScanRes != 4) || !CheckCorrectness(lCurrentTimeStamp, lPrevTimeStamp, sCurrentTrade, iCurrentQuantity, iCurrentPrice))
		{
			cout << "Wrong information in the file (string number: " << iCurrentStringNumber << ")!\n";
			cout << "Press any key...\n";
			CleanAll();
			return 0;
		}

		if (gTradeInfoMap.count(sCurrentTrade) == 0) // no such entry yet
		{
			// insert our trade symbol information into the map firt time:
			CurrentTradeInfo.Reset();
			CurrentTradeInfo.CalculateNewValues(true, lCurrentTimeStamp, iCurrentQuantity, iCurrentPrice);

			if (CurrentTradeInfo.IsSomethingWrong())
			{
				cout << "Wrong information in the file (string number: " << iCurrentStringNumber << ")!\n";
				cout << "Press any key...\n";
				CleanAll();
				return 0;
			}

			g_ret = gTradeInfoMap.insert(pair<string, CTradeInfo>(sCurrentTrade, CurrentTradeInfo));
			if (!g_ret.second) // something is wrong
			{
				cout << "Wrong calculation (input file string number: " << iCurrentStringNumber << ")!\n";
				cout << "Press any key...\n";
				CleanAll();
				return 0;
			}

		} // if (gTradeInfoMap.count(sCurrentTrade) == 0)
		else // we already have that entry
		{
			// recalculate information for an existing trade symbol entry:
			gTradeInfoMap[sCurrentTrade].CalculateNewValues(false, lCurrentTimeStamp, iCurrentQuantity, iCurrentPrice);

			if (gTradeInfoMap[sCurrentTrade].IsSomethingWrong())
			{
				cout << "Wrong calculation (input file string number: " << iCurrentStringNumber << ")!\n";
				cout << "Press any key...\n";
				CleanAll();
				return 0;
			}

		}

		++iCurrentStringNumber;
		lPrevTimeStamp = lCurrentTimeStamp;
	} // while (1)

	cout << "Output File successfully created!\n";
	cout << "Press any key...\n";
	CleanAll();
	return 0;
}
//##################################################################################################################