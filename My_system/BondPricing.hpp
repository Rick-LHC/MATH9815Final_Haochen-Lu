// BondPricingService
// bond pricing connector for getting the data from txt file

#ifndef BONDPRICING_HPP
#define BONDPRICING_HPP

#include "pricingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "productservice.hpp"
#include "boost/algorithm/string.hpp" // string algorithm
#include "boost/date_time/gregorian/gregorian.hpp" // date operation
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>

// Bond pricing service


class BondPricingService : public PricingService<Bond>
{
	typedef ServiceListener<BondPrice> myListener;
	typedef vector<myListener*> Listener_container;
protected:
	Listener_container listeners;
	std::unordered_map<string, BondPrice> id_price_map; // key on product identifier

public:
	BondPricingService() {} // empty ctor

	// Get data on our service given a key
	virtual BondPrice & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	/*
	connector call the OnMessage function in the serviceto pass the data to this Service
	*/
	virtual void OnMessage(BondPrice &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const Listener_container& GetListeners() const;
};

// Corresponding subscribe connector
class BondPricingConnector : public Connector<BondPrice>
{
	typedef ServiceListener<BondPrice> myListener;
	typedef vector<myListener*> Listener_container;
protected:
	Service<string, BondPrice>* bondPricingService;

public:
	BondPricingConnector(const string&, Service<string, BondPrice>*, BondProductService*);

	// Publish data to the Connector
	virtual void Publish(BondPrice &);

};

BondPrice & BondPricingService::GetData(string key)
{
	return id_price_map[key];
}

void BondPricingService::OnMessage(BondPrice &_BondPrice)
{
	// push the data into map
	string pd_id = _BondPrice.GetProduct().GetProductId();
	if (id_price_map.find(pd_id) == id_price_map.end())
		id_price_map.insert(std::make_pair(pd_id, _BondPrice));
	else
		id_price_map[pd_id] = _BondPrice;

	// call the listeners
	for (auto private_l : listeners)

		private_l->ProcessAdd(_BondPrice);
}

void BondPricingService::AddListener(myListener * _myListener)
{
	listeners.push_back(_myListener);
}

const BondPricingService::Listener_container& BondPricingService::GetListeners() const
{
	return listeners;
}

BondPricingConnector::BondPricingConnector(const string& path,
	Service<string, BondPrice>* _bondPricingService, BondProductService* _bondProductService) :
	bondPricingService(_bondPricingService)
{
	fstream file(path, std::ios::in);
	string line;
	char sep = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Price: Begin to read data..." << endl;
		getline(file, line); // discard header

		int temp_count = 0;
		int count_percentage = 1;

		while (getline(file, line))
		{
			++temp_count;

			stringstream lineStream(line);

			// save all trimmed data into a vector of strings
			std::vector<string> cells;
			string cell;
			while (getline(lineStream, cell, sep))
			{
				boost::algorithm::trim(cell);
				cells.push_back(cell);
			}

			BondIdType type = (boost::algorithm::to_upper_copy(cells[0]) == "ISIN") ? ISIN : CUSIP;
			string pd_id = cells[1];
			// the bond product
			Bond bond = _bondProductService->GetData(pd_id); // bond id, bond id type, ticker, coupon, maturity
			// bond price
			double mid = StrtoPrice(cells[2]);
			// bond price spread
			double spread = StrtoPrice(cells[3]);

			// price object
			BondPrice price(bond, mid, spread);

			bondPricingService->OnMessage(price);

			if ((temp_count) % (6 * 10000) == 0)
			{
				std::cout << "%" << count_percentage * 10 << " completed" << endl;
				++count_percentage;
			}
		}
		std::cout << "Price: finished!" << endl;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}
}

void BondPricingConnector::Publish(BondPrice &data)
{
	// undefined publish() for subsribe connector
}

#endif // !BONDPRICING_HPP
