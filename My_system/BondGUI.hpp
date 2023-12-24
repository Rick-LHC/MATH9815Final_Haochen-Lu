// bond GUI service for modeling GUI, 
// bond GUI connector for publishing data, and
// bond GUI service listener for data inflow from bond pricing service

#ifndef BONDGUI_HPP
#define BONDGUI_HPP

#include "pricingservice.hpp"
#include "GUIService.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <vector>
#include <unordered_map>
#include <chrono> // model the throttles
#include <fstream>
#include <sstream>
#include <iostream>


// Bond GUI Service
typedef std::chrono::milliseconds timeUnite;

class BondGUIService : public GUIService<Bond>
{
	typedef ServiceListener<BondPrice> myListener;
	typedef std::vector<myListener*> Listener_container;
	typedef Connector<BondPrice> myConnector;

protected:
	Listener_container listeners;
	myConnector* bondGuiConnector; // publish connector
	std::unordered_map<string, BondPrice> id_price_map; // key on product identifier

	// throttles modeling
	timeUnite interval;

public:
	BondGUIService(int _interval, myConnector* _bondGuiConnector); // ctor

	// Get data on our service given a key
	virtual BondPrice & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondPrice &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const Listener_container& GetListeners() const;

	// Add a price to the service
	virtual void AddPrice(const BondPrice &price);

	// Get the interval in the throttle
	const timeUnite& GetTimeInterval() const;
};

// corresponding publish connector
class BondGUIConnector : public Connector<BondPrice>
{
protected:
	fstream file;
public:
	BondGUIConnector(string _path); // ctor

	// Publish data to the Connector
	virtual void Publish(BondPrice &data);
};

// corresponding service listener
class ToBondGUIListener : public ServiceListener<BondPrice>
{
protected:
	BondGUIService* bondGuiService;

	// model throttle
	std::chrono::system_clock::time_point start;
	timeUnite interval;
	int counter = 0; // only update 100 times

public:
	ToBondGUIListener(BondGUIService* _bondGuiService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondPrice &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondPrice &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondPrice &data);
};

BondGUIService::BondGUIService(int _interval, myConnector* _bondGuiConnector) :
	interval(_interval), bondGuiConnector(_bondGuiConnector)
{
}

BondPrice & BondGUIService::GetData(string key)
{
	return id_price_map[key];
}

void BondGUIService::OnMessage(BondPrice &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondGUIService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondGUIService::Listener_container & BondGUIService::GetListeners() const
{
	return listeners;
}

void BondGUIService::AddPrice(const BondPrice &price)
{
	// push the data into the service
	string productId = price.GetProduct().GetProductId();
	if (id_price_map.find(productId) == id_price_map.end()) // if not found this one then create one
		id_price_map.insert(std::make_pair(productId, price));
	else
		id_price_map[productId] = price;

	// publish it
	BondPrice temp(price);
	bondGuiConnector->Publish(temp);

}

const timeUnite& BondGUIService::GetTimeInterval() const
{
	return interval;
}

BondGUIConnector::BondGUIConnector(string _path) :
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,Price"<<endl;;
}

void BondGUIConnector::Publish(BondPrice &data)
{
	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoStr(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		Bond bond = data.GetProduct(); // get the product
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; // get the bond id type
		std::string priceStr = PricetoStr(data.GetMid());
		// make the output
		file << date << " " << timeofDay << "," << Idtype << ","
			<< bond.GetProductId() << "," << priceStr << ""<<endl;;
	}
	else
	{
		std::cout << "Cannot open the file! Maybe the path is not right?"<<endl;;
	}
}

ToBondGUIListener::ToBondGUIListener(BondGUIService* _bondGuiService) :
	bondGuiService(_bondGuiService)
{
	interval = bondGuiService->GetTimeInterval();
	start = std::chrono::system_clock::now(); // start the clock
}

void ToBondGUIListener::ProcessAdd(BondPrice &data)
{
	// throttle control
	if (std::chrono::system_clock::now() - start >= interval && counter < 100)
	{
		bondGuiService->AddPrice(data);
		start = std::chrono::system_clock::now(); // restart the clock
		counter++;
	}
}

void ToBondGUIListener::ProcessRemove(BondPrice &data)
{ 
	// not defined for this service
}

void ToBondGUIListener::ProcessUpdate(BondPrice &data)
{ 
	// not defined for this service
}


#endif // !BONDGUI_HPP
