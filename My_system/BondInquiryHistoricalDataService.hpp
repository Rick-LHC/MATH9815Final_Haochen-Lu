// BondInquiryHistoricalDataService
// BondInquiryHistoricalDataConnector
// ToBondInquiryHistoricalDataListener for data flow from BondInquiryService to BondInquiryHistoricalDataService

#ifndef BONDINQUIRYHISTORICALDATASERVICE_HPP
#define BONDINQUIRYHISTORICALDATASERVICE_HPP

#include "historicaldataservice.hpp"
#include "BondInquiry.hpp"
#include "products.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>


// Bond historical data service for position data
class BondInquiryHistoricalDataService : public HistoricalDataService<BondInq>
{
protected:
	std::vector<ServiceListener<BondInq>*> listeners;
	Connector<BondInq>* bondInquiryHistoricalDataConnector;
	std::unordered_map<string, BondInq> inquiryMap; // key on inquiry indentifier

public:
	BondInquiryHistoricalDataService(Connector<BondInq>*); 

	// Get data on our service given a key
	virtual BondInq & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondInq &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<BondInq> *);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<BondInq>* >& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string, const BondInq&);
};

// corresponding publish connector
class BondInquiryHistoricalDataConnector : public Connector<BondInq>
{
protected:
	fstream file;
	unordered_map<InquiryState, string>states{ {InquiryState::CUSTOMER_REJECTED,"CUSTOMER_REJECTED"},{InquiryState::DONE,"DONE"},{InquiryState::QUOTED,"QUOTED"},{InquiryState::RECEIVED,"RECEIVED"},{InquiryState::REJECTED,"REJECTED"} };
public:
	BondInquiryHistoricalDataConnector(string); // ctor

	// Publish data to the Connector
	virtual void Publish(Inquiry <Bond> &);

};

// corresponding service listener
class ToBondInquiryHistoricalDataListener : public ServiceListener<BondInq>
{
protected:
	BondInquiryHistoricalDataService* bondInquiryHistoricalDataService;

public:
	ToBondInquiryHistoricalDataListener(BondInquiryHistoricalDataService* ); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondInq &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondInq &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondInq &);
};

BondInquiryHistoricalDataService::BondInquiryHistoricalDataService(Connector<BondInq>* 
	_bondInquiryHistoricalDataConnector): 
	bondInquiryHistoricalDataConnector(_bondInquiryHistoricalDataConnector){}


BondInq & BondInquiryHistoricalDataService::GetData(string key)
{
	return inquiryMap[key];
}

void BondInquiryHistoricalDataService::OnMessage(BondInq &_bondInq)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondInquiryHistoricalDataService::AddListener(ServiceListener<BondInq> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<BondInq>* >& BondInquiryHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondInquiryHistoricalDataService::PersistData(string key, const BondInq& _bondInq)
{
	// insert data into the map
	if (inquiryMap.find(key) == inquiryMap.end()) 
		// if not found this one then create one
		inquiryMap.insert(std::make_pair(key, _bondInq));
	else
		inquiryMap[key] = _bondInq;

	// publish the data
	BondInq temp(_bondInq);
	bondInquiryHistoricalDataConnector->Publish(temp);

}

BondInquiryHistoricalDataConnector::BondInquiryHistoricalDataConnector(string _path) : 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,InquiryID,BondIDType,BondID,Side,Quantity,Price,State"<<endl;;
}

void BondInquiryHistoricalDataConnector::Publish(Inquiry <Bond> &_bondInq)
{
	std::stringstream ss;
	
	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoStr(time.date());
		std::string _time = boost::posix_time::to_simple_string(time.time_of_day());
		_time.erase(_time.end() - 3, _time.end());

		//collect the attributes needed to build the output
		// get the product
		Bond bond = _bondInq.GetProduct(); 

		// get the bond id type
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; 
		std::string side = (_bondInq.GetSide() == BUY) ? "BUY" : "SELL";

		// get price
		std::string priceStr = PricetoStr(_bondInq.GetPrice()); 

		// get the state
		InquiryState state = _bondInq.GetState(); 

		std::string stateStr = states[state];

		// make the output
		file << date << " " << _time << "," << _bondInq.GetInquiryId() << "," << Idtype << "," 
			<< bond.GetProductId() << "," << bond.GetTicker() << ","
			<< std::to_string(_bondInq.GetQuantity()) << "," << priceStr << "," << stateStr << ""<<endl;;
	}
	else
	{
		std::cout << "Cannot open the file!"<<endl;;
	}
}

ToBondInquiryHistoricalDataListener::ToBondInquiryHistoricalDataListener(BondInquiryHistoricalDataService*
	_bondInquiryHistoricalDataService) :
	bondInquiryHistoricalDataService(_bondInquiryHistoricalDataService)
{
}

void ToBondInquiryHistoricalDataListener::ProcessAdd(BondInq &_bondInq)
{ 
	// not defined for this service
}

void ToBondInquiryHistoricalDataListener::ProcessRemove(BondInq &_bondInq)
{ 
	// not defined for this service
}

void ToBondInquiryHistoricalDataListener::ProcessUpdate(BondInq &_bondInq)
{ 
	string key = _bondInq.GetProduct().GetProductId();
	bondInquiryHistoricalDataService->PersistData(key, _bondInq);
}

#endif // !BONDINQUIRYHISTORICALDATASERVICE_HPP
