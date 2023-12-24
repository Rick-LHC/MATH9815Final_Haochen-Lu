#ifndef BONDEXECUTION_HPP
#define BONDEXECUTION_HPP

#include "executionservice.hpp"
#include "BondAlgoExecution.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <random>

// Bond execution service
class BondExecutionService : public ExecutionService<Bond>
{
	typedef ServiceListener<Bond_ExOrder> myListener;
	typedef std::vector<myListener*> listener_container;

protected:
	listener_container listeners;
	std::unordered_map<string, Bond_ExOrder> orderMap; // key on product identifier

public:
	BondExecutionService() {} // empty ctor

	// Get data on our service given a key
	virtual Bond_ExOrder & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_ExOrder &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Execute an order on a market
	void ExecuteOrder(const Bond_ExOrder&, Market);
};


// Bond streaming service listener
class BondExecutionListener : public ServiceListener<Bond_AgEx>
{
protected:
	BondExecutionService* bondExecutionService;
	Market markets[3]{ BROKERTEC, ESPEED, CME };

public:
	BondExecutionListener(BondExecutionService*); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Bond_AgEx &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Bond_AgEx &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Bond_AgEx &);
};


Bond_ExOrder & BondExecutionService::GetData(string key)
{
	return orderMap[key];
}

void BondExecutionService::OnMessage(Bond_ExOrder &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondExecutionService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondExecutionService::listener_container& BondExecutionService::GetListeners() const
{
	return listeners;
}

void BondExecutionService::ExecuteOrder(const Bond_ExOrder& order, Market market)
{
	// execute the order (push data to the map)
	string pd_id = order.GetProduct().GetProductId();
	if (orderMap.find(pd_id) == orderMap.end()) // if not found this one then create one
		orderMap.insert(std::make_pair(pd_id, order));
	else
		orderMap[pd_id] = order;

	// call the listeners
	Bond_ExOrder temp(order);
	for (auto private_l : listeners)
		private_l->ProcessAdd(temp);
}

BondExecutionListener::BondExecutionListener(BondExecutionService* _bondExecutionService) :
	bondExecutionService(_bondExecutionService)
{
}

void BondExecutionListener::ProcessAdd(Bond_AgEx &data)
{ // not defined for this service
}

void BondExecutionListener::ProcessRemove(Bond_AgEx &data)
{ // not defined for this service
}

void BondExecutionListener::ProcessUpdate(Bond_AgEx &data)
{
	// decide the exchange
	int val = rand() % 3;
	Market exg(markets[val]);

	// call the order execution
	Bond_ExOrder order = data.GetOrder();
	bondExecutionService->ExecuteOrder(order, exg);

}

#endif // !BONDEXECUTION_HPP
