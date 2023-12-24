// BondStreamingHistoricalDataService
// BondStreamingHistoricalDataConnector for publishing data, and 
// BondStreamingHistoricalDataService for data flow from bond streaming service

#ifndef BONDSTREAMINGHISTORICALDATASERVICE_HPP
#define BONDSTREAMINGHISTORICALDATASERVICE_HPP

#include "historicaldataservice.hpp"
#include "BondStreaming.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>

// Bond historical data service for position data
class BondStreamingHistoricalDataService : public HistoricalDataService<Bond_Ps>
{

	typedef ServiceListener<Bond_Ps> myListener;
	typedef vector<myListener*> listener_container;

protected:
	listener_container listeners;
	Connector<Bond_Ps>* bondStreamingHistoricalDataConnector;
	std::unordered_map<string, Bond_Ps> stream_Map; // key on product indentifier

public:
	BondStreamingHistoricalDataService(Connector<Bond_Ps>* _bondStreamingHistoricalDataConnector);

	// Get data on our service given a key
	virtual Bond_Ps & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_Ps &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string, const Bond_Ps&);
};

// corresponding publish connector
class BondStreamingHistoricalDataConnector : public Connector<PriceStream<Bond>>
{
	typedef PriceStream<Bond> Bond_Ps;

protected:
	fstream file;
public:
	BondStreamingHistoricalDataConnector(string path);

	// Publish data to the Connector
	virtual void Publish(Bond_Ps &data);

};

// corresponding service listener
class ToBondStreamingHistoricalDataListener : public ServiceListener<Bond_Ps>
{

protected:
	BondStreamingHistoricalDataService* bondStreamingHistoricalDataService;

public:
	ToBondStreamingHistoricalDataListener(BondStreamingHistoricalDataService*);

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Bond_Ps &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Bond_Ps &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Bond_Ps &);
};

BondStreamingHistoricalDataService::BondStreamingHistoricalDataService(Connector<Bond_Ps>* _bondStreamingHistoricalDataConnector) :
	bondStreamingHistoricalDataConnector(_bondStreamingHistoricalDataConnector) {}

Bond_Ps & BondStreamingHistoricalDataService::GetData(string key)
{
	return stream_Map[key];
}

void BondStreamingHistoricalDataService::OnMessage(Bond_Ps &data)
{
	// No OnMessage() defined for the intermediate service 
}

void BondStreamingHistoricalDataService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondStreamingHistoricalDataService::listener_container& BondStreamingHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondStreamingHistoricalDataService::PersistData(string key, const Bond_Ps& data)
{
	// push data into the map
	if (stream_Map.find(key) == stream_Map.end()) // if not found this one then create one
		stream_Map.insert(std::make_pair(key, data));
	else
		stream_Map[key] = data;

	// publish the data
	Bond_Ps temp_Bond_Ps(data);
	bondStreamingHistoricalDataConnector->Publish(temp_Bond_Ps);

}

BondStreamingHistoricalDataConnector::BondStreamingHistoricalDataConnector(string _path) :
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,BidPrice,BidVisibleQuantity,BidHiddenQuantity,"
		<< "OfferPrice,OfferVisibleQuantity,OfferHiddenQuantity" << endl;
}

void BondStreamingHistoricalDataConnector::Publish(Bond_Ps &_Bond_Ps)
{
	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string _date = DatetoStr(time.date());

		std::string _time = boost::posix_time::to_simple_string(time.time_of_day());
		_time.erase(_time.end() - 3, _time.end());

		Bond _bond = _Bond_Ps.GetProduct();

		// get the bond id type
		std::string Idtype = (_bond.GetBondIdType() == ISIN) ? "ISIN" : "CUSIP";

		auto _bidOrder = _Bond_Ps.GetBidOrder();
		auto _offerOrder = _Bond_Ps.GetOfferOrder();

		file << _date << " " << _time << "," << Idtype << "," << _bond.GetProductId() << ","
			<< std::to_string(_bidOrder.GetPrice()) << ","
			<< std::to_string(_bidOrder.GetVisibleQuantity()) << ","
			<< std::to_string(_bidOrder.GetHiddenQuantity()) << ","
			<< std::to_string(_offerOrder.GetPrice()) << ","
			<< std::to_string(_offerOrder.GetVisibleQuantity()) << ","
			<< std::to_string(_offerOrder.GetHiddenQuantity()) << endl;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}
}

ToBondStreamingHistoricalDataListener::ToBondStreamingHistoricalDataListener(
	BondStreamingHistoricalDataService* _bondStreamingHistoricalDataService) :
	bondStreamingHistoricalDataService(_bondStreamingHistoricalDataService) {}

void ToBondStreamingHistoricalDataListener::ProcessAdd(Bond_Ps &_bond_Ps)
{
	string key = _bond_Ps.GetProduct().GetProductId();
	bondStreamingHistoricalDataService->PersistData(key, _bond_Ps);
}

void ToBondStreamingHistoricalDataListener::ProcessRemove(Bond_Ps &_bond_Ps)
{
	// not defined for this service
}

void ToBondStreamingHistoricalDataListener::ProcessUpdate(Bond_Ps &_bond_Ps)
{
	// not defined for this service
}

#endif // !BONDSTREAMINGHISTORICALDATASERVICE_HPP
