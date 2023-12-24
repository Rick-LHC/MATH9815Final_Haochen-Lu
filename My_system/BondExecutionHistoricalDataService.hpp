#ifndef BONDEXECUTIONHISTORICALDATASERVICE_HPP
#define BONDEXECUTIONHISTORICALDATASERVICE_HPP

#include "historicaldataservice.hpp"
#include "BondExecution.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>


// Bond historical data service for position data
class BondExecutionHistoricalDataService : public HistoricalDataService<Bond_ExOrder>
{
	typedef ServiceListener<Bond_ExOrder> myListener;
	typedef std::vector<myListener*> listener_container;
protected:
	listener_container listeners;
	Connector<Bond_ExOrder>* bondExecutionHistoricalDataConnector;
	std::unordered_map<string, Bond_ExOrder> orderMap; // key on product indentifier

public:
	BondExecutionHistoricalDataService(Connector<Bond_ExOrder>*);

	// Get data on our service given a key
	virtual Bond_ExOrder & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_ExOrder &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string, const Bond_ExOrder&);
};

// corresponding publish connector
class BondExecutionHistoricalDataConnector : public Connector<Bond_ExOrder>
{
protected:
	fstream file;
	unordered_map<OrderType, string>OrderTypes{ {OrderType::FOK,"FOK"},{OrderType::IOC,"LOC"},{OrderType::LIMIT,"LIMIT"},{OrderType::MARKET,"MARKET"},{OrderType::STOP,"STOP"} };

public:
	BondExecutionHistoricalDataConnector(string);

	// Publish data to the Connector
	virtual void Publish(Bond_ExOrder &);

};

// corresponding service listener
class BondExecutionHistoricalDataListener : public ServiceListener<Bond_ExOrder>
{
protected:
	BondExecutionHistoricalDataService* bondExecutionHistoricalDataService;

public:
	BondExecutionHistoricalDataListener(BondExecutionHistoricalDataService*); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Bond_ExOrder &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Bond_ExOrder &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Bond_ExOrder &);
};

BondExecutionHistoricalDataService::BondExecutionHistoricalDataService(
	Connector<Bond_ExOrder>* _bondExecutionHistoricalDataConnector) :
	bondExecutionHistoricalDataConnector(_bondExecutionHistoricalDataConnector){}

Bond_ExOrder & BondExecutionHistoricalDataService::GetData(string key)
{
	return orderMap[key];
}

void BondExecutionHistoricalDataService::OnMessage(Bond_ExOrder &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondExecutionHistoricalDataService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondExecutionHistoricalDataService::listener_container & BondExecutionHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondExecutionHistoricalDataService::PersistData(string key, const Bond_ExOrder& _bond_ExOrder)
{
	// insert data
	if (orderMap.find(key) == orderMap.end()) // if not found this one then create one
		orderMap.insert(std::make_pair(key, _bond_ExOrder));
	else
		orderMap[key] = _bond_ExOrder;

	// publish the data
	Bond_ExOrder temp(_bond_ExOrder);
	bondExecutionHistoricalDataConnector->Publish(temp);

}

BondExecutionHistoricalDataConnector::BondExecutionHistoricalDataConnector(string _path):
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,OrderType,OrderID,BondIDType,BondID,Side,VisibleQuantity,HiddenQuantity,Price,IsChildOrder,ParentOrderId"<<endl;;
}

void BondExecutionHistoricalDataConnector::Publish(Bond_ExOrder &_bond_ExOrder)
{
	
	if (file.is_open())
	{
		// build output
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoStr(time.date());
		std::string _time = boost::posix_time::to_simple_string(time.time_of_day());
		_time.erase(_time.end() - 3, _time.end());

		OrderType type = _bond_ExOrder.GetOrderType(); // 
		std::string typeStr = OrderTypes[type];
		// get the product
		Bond bond = _bond_ExOrder.GetProduct(); 
		// get the bond id type
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; 
		std::string side = (_bond_ExOrder.GetSide() == BID) ? "BID" : "OFFER";
		// get price
		std::string price = PricetoStr(_bond_ExOrder.GetPrice()); 
		std::string isChild = (_bond_ExOrder.IsChildOrder() == true) ? "TRUE" : "FALSE";

		// make the output
		file << date << " " << _time << "," << typeStr << "," << _bond_ExOrder.GetOrderId() << "," 
			<< Idtype << "," << bond.GetProductId() << ","
			<< side << "," << std::to_string(_bond_ExOrder.GetVisibleQuantity()) << ","
			<< std::to_string(_bond_ExOrder.GetHiddenQuantity()) << "," << price << ","
			<< isChild << "," << _bond_ExOrder.GetParentOrderId() << ""<<endl;;
	}
	else
	{
		std::cout << " Cannot open the file!" << endl;
	}
}

BondExecutionHistoricalDataListener::BondExecutionHistoricalDataListener(
	BondExecutionHistoricalDataService* _bondExecutionHistoricalDataService):
	bondExecutionHistoricalDataService(_bondExecutionHistoricalDataService){}

void BondExecutionHistoricalDataListener::ProcessAdd(Bond_ExOrder &_bond_ExOrder)
{
	string key = _bond_ExOrder.GetProduct().GetProductId();
	bondExecutionHistoricalDataService->PersistData(key, _bond_ExOrder);
}

void BondExecutionHistoricalDataListener::ProcessRemove(Bond_ExOrder &_bond_ExOrder)
{ 
	// not defined for this service
}

void BondExecutionHistoricalDataListener::ProcessUpdate(Bond_ExOrder &_bond_ExOrder)
{ 
	// not defined for this service
}

#endif // !BONDEXECUTIONHISTORICALDATA_HPP
