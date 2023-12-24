// BondInquiryConnector for the inquiry interaction with the client
// BondInquiryService to get the data from txt file

#ifndef BONDINQUIRY_HPP
#define BONDINQUIRY_HPP

#include "inquiryservice.hpp"
#include "products.hpp"
#include "productservice.hpp"
#include "soa.hpp"
#include "boost/date_time/gregorian/gregorian.hpp" 
#include "boost/algorithm/string.hpp" 
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

// Bond inquiry service
class BondInquiryService : public InquiryService<Bond>
{
	typedef ServiceListener<BondInq> myListener;
	typedef std::vector<myListener*> listener_container;

private:
	listener_container listeners;
	Connector<BondInq>* bondInquiryConnector;
	std::unordered_map<string, BondInq> inquiryMap; // key on inquiry identifier

public:
	BondInquiryService() {}; // empty ctor

	// Set the inner connector (specific for publish and subscribe connectors)
	void SetConnector(Connector<BondInq>*);

	// Get data on our service given a key
	virtual BondInq & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondInq &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const vector< myListener* >& GetListeners() const;

	// Send a quote back to the client
	virtual void SendQuote(const string &, double);

	// Reject an inquiry from the client
	virtual void RejectInquiry(const string &);

};


// corresponding subscribe and publish connector
class BondInquiryConnector : public Connector<BondInq>
{
protected:
	BondInquiryService* bondInquiryService;
	unordered_map<string, InquiryState>states{ {"CUSTOMER_REJECTED",InquiryState::CUSTOMER_REJECTED},{"DONE",InquiryState::DONE,},{"QUOTED",InquiryState::QUOTED},{"RECEIVED",InquiryState::RECEIVED},{"REJECTED",InquiryState::REJECTED} };

public:
	BondInquiryConnector(string , BondInquiryService*,BondProductService*);

	// Publish data to the Connector
	virtual void Publish(BondInq &);

};

// corresponding service listener
class ToBondInquiryListener : public ServiceListener<BondInq>
{
protected:
	BondInquiryService* bondInquiryService;

public:
	ToBondInquiryListener(BondInquiryService* ); // Ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondInq &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondInq &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondInq &);
};

void BondInquiryService::SetConnector(Connector<BondInq>* _bondInquiryConnector)
{
	bondInquiryConnector = _bondInquiryConnector;
}

BondInq & BondInquiryService::GetData(string key)
{
	return inquiryMap[key];
}

void BondInquiryService::OnMessage(BondInq &_bondInq)
{
	// update the inquiry map
	string orderId = _bondInq.GetInquiryId();
	if (inquiryMap.find(orderId) == inquiryMap.end()) // if not found this one then create one
		inquiryMap.insert(std::make_pair(orderId, _bondInq));
	else
		inquiryMap[orderId] = _bondInq;

	// call the listeners
	for (auto private_l : listeners)
		private_l->ProcessUpdate(_bondInq);
}

void BondInquiryService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondInquiryService::listener_container& BondInquiryService::GetListeners() const
{
	return listeners;
}

void BondInquiryService::SendQuote(const string &inquiryId, double price)
{
	// send back the quote to the connector
	BondInq inquiry = inquiryMap[inquiryId]; // retrieve the corresponding inquiry

	// create a new inquiry with the new info
	BondInq newInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(),
		price, inquiry.GetState()); 

	bondInquiryConnector->Publish(newInquiry);
}

void BondInquiryService::RejectInquiry(const string &inquiryId)
{
	BondInq inquiry = inquiryMap[inquiryId]; // get the corresponding inquiry

	/// transition the inquiry state to REJECTION
	BondInq newInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(),
		inquiry.GetPrice(), REJECTED); 

	//send it back
	bondInquiryConnector->Publish(newInquiry); 
}

BondInquiryConnector::BondInquiryConnector(string path, BondInquiryService* _bondInquiryService,
	BondProductService* _bondProductService) :
	bondInquiryService(_bondInquiryService)
{
	bondInquiryService->SetConnector(this);

	fstream file(path, std::ios::in);
	string line;

	char sep = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Inquiry: Begin to read data" << endl;
		getline(file, line); // discard header
		
		while (getline(file, line))
		{
			stringstream sin(line);

			// save all trimmed data into a vector of strings
			std::vector<string> tempData;
			string info;
			while (getline(sin, info, sep))
			{
				boost::algorithm::trim(info);
				tempData.push_back(info);
			}

			// make the corresponding inquiry object
			// inquiry Id
			string inquiryId = tempData[0];
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(tempData[1]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = tempData[2];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); // bond id, bond id type, ticker, coupon, maturity
			// inquiry side
			Side side = (boost::algorithm::to_upper_copy(tempData[3]) == "BUY") ? BUY : SELL;
			// inquiry quantity
			long quantity = std::stol(tempData[4]);
			// inquiry price
			double price = StrtoPrice(tempData[5]);
			// inquiry state
			boost::algorithm::to_upper(tempData[6]);
			InquiryState state = states[tempData[6]];

			// inquiry object
			BondInq inquiry(inquiryId, bond, side, quantity, price, state);

			bondInquiryService->OnMessage(inquiry);
		}
		std::cout << "Inquiry: finished!" << endl;
	}
	else
	{
		std::cout << "Cannot open the file!" << endl;
	}

}

// Publish data to the Connector
void BondInquiryConnector::Publish(BondInq &data)
{
	if (data.GetState() == REJECTED) // if the inquiry rejected by the service
		bondInquiryService->OnMessage(data);
	else // if not rejected by the service
	{
		// transition the inquiry to the QUOTED state
		BondInq _quoted(data.GetInquiryId(), data.GetProduct(), data.GetSide(), data.GetQuantity(),data.GetPrice(), QUOTED);

		// send it back to the service
		bondInquiryService->OnMessage(_quoted);

		// transition the inquiry to the DONE state (or the customer can reject it)
		BondInq _done(data.GetInquiryId(), data.GetProduct(), data.GetSide(), data.GetQuantity(),data.GetPrice(), DONE);

		// send it back to the service
		bondInquiryService->OnMessage(_done);
	}

}

ToBondInquiryListener::ToBondInquiryListener(BondInquiryService* _bondInquiryService) :
	bondInquiryService(_bondInquiryService)
{
}

void ToBondInquiryListener::ProcessAdd(BondInq &_BondInq)
{ 
	// not defined for this service
}

void ToBondInquiryListener::ProcessRemove(BondInq &_BondInq)
{ 
	// not defined for this service
}

void ToBondInquiryListener::ProcessUpdate(BondInq &_BondInq)
{
	if (_BondInq.GetState() == RECEIVED) 
		// if inquiry received, send a quote of 100.0
		bondInquiryService->SendQuote(_BondInq.GetInquiryId(), 100.0);
}

#endif // !BONDINQUIRY_HPP