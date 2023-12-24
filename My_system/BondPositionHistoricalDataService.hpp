// bond position historical data service for maintaining the position data, and
// bond position historical data service connector for publishing data, and 
// bond position historical data service listener for data inflow from bond position service

#ifndef BONDPOSITIONHISTORICALDATASERIVCE_HPP
#define BONDPOSITIONHISTORICALDATASERIVCE_HPP

#include "historicaldataservice.hpp"
#include "BondPosition.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>


// Bond historical data service for position data
class BondPositionHistoricalDataService : public HistoricalDataService<BondPos>
{
	typedef ServiceListener<BondPos> myListener;
	typedef std::vector<myListener*> listener_container;
	typedef Connector<BondPos> myConnector;
protected:
	listener_container listeners;
	myConnector* bondPositionHistoricalDataConnector;
	std::unordered_map<string, BondPos> id_pos_map; // key on product indentifier

public:
	BondPositionHistoricalDataService(myConnector*); // ctor

	// Get data on our service given a key
	virtual BondPos & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondPos &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string, const BondPos&);
};

// corresponding publish connector
class BondPositionHistoricalDataConnector : public Connector<BondPos>
{
protected:
	fstream file;
public:
	BondPositionHistoricalDataConnector(string); // ctor

	// Publish data to the Connector
	virtual void Publish(BondPos &);

};

// corresponding service listener
class ToBondPositionHistoricalDataListener : public ServiceListener<BondPos>
{
protected:
	BondPositionHistoricalDataService* bondPositionHistoricalDataService;

public:
	ToBondPositionHistoricalDataListener(BondPositionHistoricalDataService*); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondPos &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondPos &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondPos &);
};

BondPositionHistoricalDataService::BondPositionHistoricalDataService(myConnector*_bondPositionHistoricalDataConnector) :
	bondPositionHistoricalDataConnector(_bondPositionHistoricalDataConnector){}


BondPos & BondPositionHistoricalDataService::GetData(string key)
{
	return id_pos_map[key];
}

void BondPositionHistoricalDataService::OnMessage(BondPos &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondPositionHistoricalDataService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondPositionHistoricalDataService::listener_container& BondPositionHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondPositionHistoricalDataService::PersistData(string key, const BondPos& data)
{
	// push data into the map
	if (id_pos_map.find(key) == id_pos_map.end()) // if not found this one then create one
		id_pos_map.insert(std::make_pair(key, data));
	else
		id_pos_map[key] = data;

	// publish the data
	BondPos temp(data);
	bondPositionHistoricalDataConnector->Publish(temp);

}

BondPositionHistoricalDataConnector::BondPositionHistoricalDataConnector(string _path) : 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,BookId,Positions"<<endl;;
}

void BondPositionHistoricalDataConnector::Publish(BondPos &data)
{
	std::stringstream ss;
	// hard-coded the book id
	std::string book1 = "TRSY1";
	std::string book2 = "TRSY2";
	std::string book3 = "TRSY3";


	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoStr(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		Bond bond = data.GetProduct(); // get the product
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; // get the bond id type
		// make the output
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book1 << "," << std::to_string(data.GetPosition(book1)) << ""<<endl;;
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book2 << "," << std::to_string(data.GetPosition(book2)) << ""<<endl;;
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book3 << "," << std::to_string(data.GetPosition(book3)) << ""<<endl;;
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< "AGGREGATED" << "," << std::to_string(data.GetAggregatePosition()) << ""<<endl;;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}
}

ToBondPositionHistoricalDataListener::ToBondPositionHistoricalDataListener(BondPositionHistoricalDataService* 
	_bondPositionHistoricalDataService): bondPositionHistoricalDataService(_bondPositionHistoricalDataService)
{
}

void ToBondPositionHistoricalDataListener::ProcessAdd(BondPos &data)
{ // not defined for this service
}

void ToBondPositionHistoricalDataListener::ProcessRemove(BondPos &data)
{ // not defined for this service
}

void ToBondPositionHistoricalDataListener::ProcessUpdate(BondPos &data)
{
	string key = data.GetProduct().GetProductId();
	bondPositionHistoricalDataService->PersistData(key, data);
}

#endif // !BONDPOSITIONHISTORICALDATASERIVCE_HPP
