// Simulate the trade data with some instructions

#ifndef BONDTRADEDATAGENERATOR_HPP
#define BONDTRADEDATAGENERATOR_HPP

#include "productservice.hpp"
#include "products.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// generate the trade data and write it to the file specified by path
void bond_trade_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);


	if (file.is_open())
	{
		std::cout << "Trade: Simulating the trade data..."<<endl;
		// header
		file << "TradeID,BondIDType,BondID,Side,Quantity,Price,BookId"<<endl;

		int n = bondVec.size();
		int bondIndex;
		// each product has 10 data
		string books[3]{ "TRSY1","TRSY2" ,"TRSY3" };
		for (long i = 0; i < n * 10; i++)
		{
			bondIndex = i % n;
			Bond bond = bondVec[bondIndex];

			std::string tradeId = ("TRADE" + std::to_string(bond.GetMaturityDate().year()) + bond.GetTicker() + std::to_string(i + 1));

			// bond id type
			std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN";
			// side (alternate between buy and sell for each product)
			std::string side = ((i/n) % 2 == 0) ? "BUY" : "SELL";
			// quantity (alternate among 1M, 2M, 3M, 4M and 5M for each product)
			long quantity = 1000000 * ((i / n) % 5 + 1);
			// price (99 buy, 100 sell)
			double price = (side == "BUY")? 99.0: 100.0; 

			// book id, alternate between three books
			string bookId = books[(i / n) % 3];

			// make the output
			file << tradeId << "," << Idtype << "," << bond.GetProductId() << "," << side << ","
				<< quantity << "," << PricetoStr(price) << "," << bookId << ""<<endl;
		}
		std::cout << "Trade: Simulation finished!"<<endl;;
	}
	else
	{
		std::cout << "Cannot open the file!"<<endl;;
	}

}


#endif // !BONDTRADEDATAGENERATOR_HPP
