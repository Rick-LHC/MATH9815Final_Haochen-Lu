// BondMarketDataService
// BondMarketDataConnector to get the data from txt file

#ifndef BONDMARKETDATA_HPP
#define BONDMARKETDATA_HPP

#include "marketdataservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "productservice.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>

// Bond market data service
class BondMarketDataService : public MarketDataService<Bond>
{
	typedef ServiceListener<BondOrderBook> myListener;
	typedef std::vector<myListener*> listener_container;

protected:
	listener_container listeners;
	std::unordered_map<string, BondOrderBook> id_orderbook_map; // key: bond ID value: Bond order book
public:
	BondMarketDataService() {}

	// Get data on our service given a key
	virtual BondOrderBook & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondOrderBook &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Get the best bid/offer order
	virtual const BidOffer& GetBestBidOffer(const string &);

	// Aggregate the order book
	virtual const BondOrderBook& AggregateDepth(const string &);
};

// Corresponding subscribe connector
class BondMarketDataConnector : public Connector<BondOrderBook>
{
protected:
	Service<string, BondOrderBook>* bondMarketDataService;

public:
	BondMarketDataConnector(string, Service<string, BondOrderBook>*, BondProductService*); // ctor

	// Publish data to the Connector
	virtual void Publish(BondOrderBook &);

};

BondOrderBook & BondMarketDataService::GetData(string key)
{
	return id_orderbook_map[key];
}

void BondMarketDataService::OnMessage(BondOrderBook &_bondOrderBook)
{
	// push the _bondOrderBook into map
	string pd_id = _bondOrderBook.GetProduct().GetProductId();
	if (id_orderbook_map.find(pd_id) == id_orderbook_map.end()) // if not found this one then create one
		id_orderbook_map.insert(std::make_pair(pd_id, _bondOrderBook));
	else
		id_orderbook_map[pd_id] = _bondOrderBook;

	// call the listeners
	for (auto private_l : listeners)
		private_l->ProcessAdd(_bondOrderBook);
}

void BondMarketDataService::AddListener(myListener *_listener)
{
	listeners.push_back(_listener);
}

const BondMarketDataService::listener_container& BondMarketDataService::GetListeners() const
{
	return listeners;
}

const BidOffer& BondMarketDataService::GetBestBidOffer(const string &pd_id)
{
	return id_orderbook_map[pd_id].GetBestBidOffer();
}

const BondOrderBook& BondMarketDataService::AggregateDepth(const string &pd_id)
{
	BondOrderBook orderbook = id_orderbook_map[pd_id];
	std::vector<Order> bidOrders = orderbook.GetBidStack();
	std::vector<Order> offerOrders = orderbook.GetOfferStack();

	// aggregation
	std::unordered_map<double, long> bidMap;
	std::unordered_map<double, long> offerMap;

	double temp_price; // temp price
	double temp_quantity;
	// for bid orders
	for (auto iter = bidOrders.begin(); iter != bidOrders.end(); iter++)
	{
		temp_price = iter->GetPrice();
		temp_quantity = iter->GetQuantity();
		if (bidMap.find(temp_price) != bidMap.end())
			bidMap[temp_price] += temp_quantity;
		else
			bidMap.insert(std::make_pair(temp_price, temp_quantity));
	}

	// for offer orders
	for (auto iter = offerOrders.begin(); iter != offerOrders.end(); iter++)
	{
		temp_price = iter->GetPrice();
		temp_quantity = iter->GetQuantity();
		if (offerMap.find(temp_price) != offerMap.end())
			offerMap[temp_price] += temp_quantity;
		else
			offerMap.insert(std::make_pair(temp_price, temp_quantity));
	}

	std::vector<Order> newBorders;
	std::vector<Order> newOorders;

	// rebuild the bid and offer orders
	for (auto iter = bidMap.begin(); iter != bidMap.end(); iter++)
	{
		newBorders.push_back(Order(iter->first, iter->second, BID));
	}
	for (auto iter = offerMap.begin(); iter != offerMap.end(); iter++)
	{
		newOorders.push_back(Order(iter->first, iter->second, OFFER));
	}

	// re-construct the order book
	BondOrderBook neworderbook(orderbook.GetProduct(), newBorders, newOorders);

	id_orderbook_map[pd_id] = neworderbook;
	return id_orderbook_map[pd_id];
}

BondMarketDataConnector::BondMarketDataConnector(
	string path, Service<string, BondOrderBook>* _bondMarketDataService, BondProductService* _bondProductService) :
	bondMarketDataService(_bondMarketDataService)
{
	fstream file(path, std::ios::in);
	string line;

	char sep = ',';

	if (file.is_open())
	{
		std::cout << "Market data: Begin to read data..." << endl;
		getline(file, line); // discard header

		int temp_count = 0;
		int count_percentage = 1;

		while (getline(file, line))
		{
			++temp_count;

			stringstream lineStream(line);

			// save all trimmed data
			std::vector<string> cells;
			string cell;
			while (getline(lineStream, cell, sep))
			{
				boost::algorithm::trim(cell);
				cells.push_back(cell);
			}

			// make the corresponding trade object
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(cells[0]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = cells[1];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); // bond id, bond id type, ticker, coupon, maturity
			// mid price
			double midprice = StrtoPrice(cells[2]);
			// 5 bid orders and 5 offer orders
			std::vector<Order> bidOrders;
			std::vector<Order> offerOrders;

			long temp_quantity;
			double temp_price;

			for (int i = 1; i <= 5; i++)
			{
				temp_quantity = std::stol(cells[7 + i]);

				temp_price = midprice - StrtoPrice(cells[2 + i]);
				Order bid(temp_price, temp_quantity, BID);
				temp_quantity = std::stol(cells[7 + i]);
				temp_price = midprice + StrtoPrice(cells[2 + i]);
				Order offer(temp_price, temp_quantity, OFFER);
				bidOrders.push_back(bid);
				offerOrders.push_back(offer);
			}

			// order book object
			BondOrderBook orderbook(bond, bidOrders, offerOrders);

			bondMarketDataService->OnMessage(orderbook);

			if ((temp_count) % (6 * 1000) == 0)
			{
				std::cout << "%" << count_percentage << " completed" << endl;
				++count_percentage;
			}

		}
		std::cout << "Market data: finished!" << endl;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;;
	}
}

void BondMarketDataConnector::Publish(BondOrderBook &data)
{
	// undefined publish() for subsribe connector
}

#endif // !BONDMARKETDATA_HPP

