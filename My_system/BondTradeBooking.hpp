// BondTradeBookingService
// BondTradeBookingConnector for getting the data from txt file  
// ToBondTradeBookingListener for the data flow from BondExecutionService to BondTradeBookingService

#ifndef BONDTRADEBOOKING_HPP
#define BONDTRADEBOOKING_HPP

#include "tradebookingservice.hpp"
#include "BondExecution.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "productservice.hpp"
#include "boost/algorithm/string.hpp" 
#include "boost/date_time/gregorian/gregorian.hpp" 
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>

// Bond trade booking service
class BondTradeBookingService : public TradeBookingService<Bond>
{
	typedef ServiceListener<BondTrade> myListener;
	typedef std::vector<myListener*> listener_container;

protected:
	listener_container listeners;
	long counter = 0; // counter to determine the trade book of the trade coming from bond execution service
	std::unordered_map<string, BondTrade> tradeMap; // key on trade identifier

public:
	BondTradeBookingService() {} // empty ctor

	// Get data on our service given a key
	virtual BondTrade & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	/*
	也就是说connector通过调用service中的OnMessage 把数据传给service
	*/
	virtual void OnMessage(BondTrade &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const vector< myListener* >& GetListeners() const;

	// Book the trade
	virtual void BookTrade(const BondTrade &);

	// Get the current value of counter
	const long GetCounter() const;

};

// corresponding subscribe connector
class BondTradeBookingConnector : public Connector<BondTrade>
{
protected:
	Service<string, BondTrade>* bondTradeBookingService;

public:
	BondTradeBookingConnector(string path,
		Service<string, BondTrade>* _bondTradeBookingService, BondProductService* _bondProductService); // ctor

	// Publish data to the Connector
	virtual void Publish(BondTrade &data);

};

// corresponding service listener
//From BondExecutionService to BondTradeBookingService
class ToBondTradeBookingListener : public ServiceListener<Bond_ExOrder>
{
protected:
	BondTradeBookingService* bondTradeBookingService;

public:
	ToBondTradeBookingListener(BondTradeBookingService* _bondTradeBookingService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Bond_ExOrder &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Bond_ExOrder &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Bond_ExOrder &data);
};

BondTrade & BondTradeBookingService::GetData(string key)
{
	return tradeMap[key];

}

void BondTradeBookingService::OnMessage(BondTrade &data)
{
	// book the trade
	BookTrade(data);

}

void BondTradeBookingService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondTradeBookingService::listener_container& BondTradeBookingService::GetListeners() const
{
	return listeners;
}

void BondTradeBookingService::BookTrade(const BondTrade &trade)
{
	string tradeId = trade.GetTradeId();
	if (tradeMap.find(tradeId) == tradeMap.end()) // if not found this one then create one
		tradeMap.insert(std::make_pair(tradeId, trade));
	else
		tradeMap[tradeId] = trade;
	++counter;

	// call the listeners
	BondTrade temp(trade);
	for (auto private_l : listeners)
		private_l->ProcessUpdate(temp);
}

const long BondTradeBookingService::GetCounter() const
{
	return counter;
}

BondTradeBookingConnector::BondTradeBookingConnector(
	string path, Service<string, BondTrade>* _bondTradeBookingService, BondProductService* _bondProductService) :
	bondTradeBookingService(_bondTradeBookingService)
{
	fstream file(path, std::ios::in);
	string line;
	std::stringstream ss;
	char sep = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Trade: Begin to read data..."<<endl;;
		getline(file, line); // discard header

		while (getline(file, line))
		{
			stringstream lineStream(line);

			// save all data into a vector of strings
			std::vector<string> cells;
			string cell;
			while (getline(lineStream, cell, sep))
			{
				boost::algorithm::trim(cell);
				cells.push_back(cell);
			}

			// build the corresponding BondTrade object
			// tradeId
			string tradeId = cells[0];
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(cells[1]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = cells[2];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); 

			// trade side
			Side side = (boost::algorithm::to_upper_copy(cells[3]) == "BUY") ? BUY : SELL;

			long quantity = std::stol(cells[4]);

			double price = StrtoPrice(cells[5]);
			// book id
			string bookId = cells[6];

			// BondTade object
			BondTrade trade(bond, tradeId, price, bookId, quantity, side);

			bondTradeBookingService->OnMessage(trade);
		}
		std::cout << "Trade: finished!"<<endl;;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}
}

void BondTradeBookingConnector::Publish(BondTrade &data)
{
	// undefined publish() for subsribe connector
}

ToBondTradeBookingListener::ToBondTradeBookingListener(BondTradeBookingService* _bondTradeBookingService) :
	bondTradeBookingService(_bondTradeBookingService) {}


void ToBondTradeBookingListener::ProcessAdd(Bond_ExOrder &_bond_ExOrder)
{
	// Determine the atributes of the trade
	long counter = bondTradeBookingService->GetCounter();
	string books[3]{ "TRSY1","TRSY2" ,"TRSY3" };
	Bond bond = _bond_ExOrder.GetProduct();

	// Trade ID (e.g. TRADE2024T23)
	
	string tradeId = ("TRADE"+ std::to_string(bond.GetMaturityDate().year()) + bond.GetTicker() + std::to_string(counter));
	string bookId = books[counter % 3];

	// determine the side
	Side side = (_bond_ExOrder.GetSide() == BID) ? SELL : BUY;

	// generate a trade based on the coming execution order
	BondTrade trade(_bond_ExOrder.GetProduct(), tradeId, _bond_ExOrder.GetPrice(), bookId,
		_bond_ExOrder.GetHiddenQuantity() + _bond_ExOrder.GetVisibleQuantity(), side);

	// book the trade
	bondTradeBookingService->BookTrade(trade);

}

void ToBondTradeBookingListener::ProcessRemove(Bond_ExOrder &_bond_ExOrder)
{ 
	// not defined for this service
}

void ToBondTradeBookingListener::ProcessUpdate(Bond_ExOrder &_bond_ExOrder)
{ 
	// not defined for this service
}

#endif // !BONDTRADEBOOKING_HPP
