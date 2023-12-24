// Simulate the market data (orderbook) with some instructions

#ifndef BONDMARKETDATAGENERATOR_HPP
#define BONDMARKETDATAGENERATOR_HPP

#include "productservice.hpp"
#include "products.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include<algorithm>

// generate the market data (orderbook) and write it to the file specified by path
void bond_market_data_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);

	if (file.is_open())
	{
		std::cout << "Market data: Simulating the market data" << endl;
		// header
		file << "BondIDType,BondID,Price,Spread1,Spread2,Spread3,Spread4,Spread5,"
			<< "Size1,Size2,Size3,Size4,Size5" << endl;;

		int n = bondVec.size(); // # of bonds

		//cout << "n is: " << n << endl;

		int bondIndex;
		// each product has 1000000 data
		double temp_unite = 1 / 128.0;
		vector<double>spreads{ temp_unite,temp_unite *2.0,temp_unite *3.0,temp_unite *4.0,temp_unite *5.0 };
		double pre_spread;
		int temp_count = 1;
		for (long i = 0; i < n * 100000; ++i)
		{
			bondIndex = i % n;
			Bond bond = bondVec[bondIndex];
			// bond id type
			std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN";
			// price (ocsillate from 99 to 101 to 99 with 1/256 as increments/decrements for each product)
			int temp = (i / n) % 1024;
			double price = 99.0 + ((temp < 512) ? temp / 256.0 : (1024 - temp) / 256.0);
			std::string priceStr = PricetoStr(price);
			// top spread (alternate between 1/128 and 1/64 for each product)
			int temp2 = (i / n) % 6;
			pre_spread = (temp2 < 3) ? temp2 / 128.0 : (6 - temp2) / 128.0;
			auto temp_spreads(spreads);
			std::for_each(temp_spreads.begin(), temp_spreads.end(), [&](double& x) {x += pre_spread; });

			// make the output
			file << Idtype << "," << bond.GetProductId() << "," << priceStr << ","
				<< PricetoStr(temp_spreads[0]) << "," << PricetoStr(temp_spreads[1]) << "," << PricetoStr(temp_spreads[2]) << "," << PricetoStr(temp_spreads[3]) << ","
				<< PricetoStr(temp_spreads[4]) << "," << std::to_string(10000000) << "," << std::to_string(20000000) << ","
				<< std::to_string(30000000) << "," << std::to_string(40000000) << "," << std::to_string(50000000)
				<< "" << endl;

			if (((i + 1)) % (n * 10000) == 0)
			{
				std::cout << "%" << temp_count * 10 << " completed" << endl;
				++temp_count;
			}
		}
		std::cout << "Market data: Simulation finished!" << endl;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}
}




#endif // !BONDMARKETDATAGENERATOR_HPP
