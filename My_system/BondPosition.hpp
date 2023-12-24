// ToBondPositionListener for the data flow from BondTradeBookingService to BondPositionService

#ifndef BONDPOSITION_HPP
#define BONDPOSITION_HPP

#include "productservice.hpp"
#include "positionservice.hpp"
#include "tradebookingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

// Bond position service
class BondPositionService : public PositionService<Bond>
{
	typedef ServiceListener<BondPos> myListener;
	typedef std::vector<myListener*> listener_container;

protected:
	listener_container listeners;
	std::unordered_map<string, BondPos> id_pos_map; // key on product identifier

public:
	BondPositionService(BondProductService*, std::string); 

	// Get data on our service given a key
	virtual BondPos & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondPos &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Add a trade to the service
	virtual void AddTrade(const BondTrade&);

};

//from BondTradeBookingService to BondPositionService
//
class ToBondPositionListener : public ServiceListener<BondTrade>
{
protected:
	BondPositionService* bondPositionService;

public:
	ToBondPositionListener(BondPositionService*); //constructor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondTrade &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondTrade &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondTrade &);
};

BondPositionService::BondPositionService(BondProductService* bondProductService, std::string ticker)
{
	std::vector<Bond> _products = bondProductService->GetBonds(ticker);

	// initialize the position map
	for (auto bd:_products)
	{
		BondPos position(bd);
		id_pos_map.insert(std::make_pair(bd.GetProductId(), position));
	}
}

BondPos & BondPositionService::GetData(string key)
{
	return id_pos_map[key];
}

void BondPositionService::OnMessage(BondPos &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondPositionService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondPositionService::listener_container& BondPositionService::GetListeners() const
{
	return listeners;
}

void BondPositionService::AddTrade(const BondTrade &trade)
{
	// Update the position based on this trade
	string pd_id = trade.GetProduct().GetProductId();
	string book = trade.GetBook();
	long tmp_qt = trade.GetQuantity();
	long qt= (trade.GetSide() == BUY) ? tmp_qt : -tmp_qt;
	BondPos pos = id_pos_map[pd_id];
	pos.AddNewPosition(book, qt);
	id_pos_map[pd_id] = pos;

	// Send this pos to the listeners
	for (auto private_l : listeners)
		private_l->ProcessUpdate(pos);
}

ToBondPositionListener::ToBondPositionListener(BondPositionService* _bondPositionService) :
	bondPositionService(_bondPositionService){}

void ToBondPositionListener::ProcessAdd(BondTrade &_bondTrade)
{ 
	// not defined for this service
}

void ToBondPositionListener::ProcessRemove(BondTrade &_bondTrade)
{ 
	// not defined for this service
}

void ToBondPositionListener::ProcessUpdate(BondTrade &_bondTrade)
{
	bondPositionService->AddTrade(_bondTrade);
}

#endif // !BONDPOSITION_HPP
