#ifndef BONDALGOEXECUTION_HPP
#define BONDALGOEXECUTION_HPP

#include "marketdataservice.hpp"
#include "executionservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <sstream>

// The AlgoExecution class with a reference to an ExecutionOrder object
// Type T is the product type
template <typename T>
class AlgoExecution
{
private:
	ExecutionOrder<T> order;

public:
	AlgoExecution(const ExecutionOrder<T>&);
	AlgoExecution() {}

	// Get the execution order
	const ExecutionOrder<T>& GetOrder() const;
};

//Type alias
typedef AlgoExecution<Bond> Bond_AgEx;

// Bond algo-execution service to determine the execution order
// key on the product identifier
// value on an AlgoExecution object
class BondAlgoExecutionService : public Service<string, Bond_AgEx>
{
	typedef ServiceListener<Bond_AgEx> myListener;
	typedef std::vector<myListener*> listener_container;

protected:
	listener_container listeners;

	// key: product identifier, value: AlgoExecution<Bond>
	std::unordered_map<string, Bond_AgEx> id_AgEx_map; 
	
	long counter = 0;//to decide the side of the order 

public:
	BondAlgoExecutionService() {}

	// Get data on our service given a key
	virtual Bond_AgEx & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_AgEx &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;
	
	// Generate the execution order and update it to the stored data
	virtual void AddOrder(const BondOrderBook&);
};

// Bond algo-execution service listener
// register in bondmarketdataservice to process the orderbook data to BondAlgoExecutionService
class BondAlgoExecutionListener : public ServiceListener<BondOrderBook>
{
protected:
	BondAlgoExecutionService* bondAlgoExecutionService;

public:
	BondAlgoExecutionListener(BondAlgoExecutionService*); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondOrderBook &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondOrderBook &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondOrderBook &);
};

template <typename T>
AlgoExecution<T>::AlgoExecution(const ExecutionOrder<T>& _order) : order(_order){}


template <typename T>
const ExecutionOrder<T>& AlgoExecution<T>::GetOrder() const
{
	return order;
}

Bond_AgEx & BondAlgoExecutionService::GetData(string key)
{
	return id_AgEx_map[key];
}

void BondAlgoExecutionService::OnMessage(Bond_AgEx &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondAlgoExecutionService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondAlgoExecutionService::listener_container& BondAlgoExecutionService::GetListeners() const
{
	return listeners;
}

void BondAlgoExecutionService::AddOrder(const BondOrderBook& orderBook)
{
	auto product = orderBook.GetProduct();
	string productId = product.GetProductId();

	// Get the best bid and offer in the order book
	BidOffer bestBidOffer = orderBook.GetBestBidOffer();

	double bestbid = bestBidOffer.GetOfferOrder().GetPrice();
	double bestoffer = bestBidOffer.GetBidOrder().GetPrice();

	// Generate an execution order only if the spread is tightest
	std::string orderId;
	if (bestoffer - bestbid <= 1.0 / 128)
	{
		// determine the attributes of the execution order

		// Creat order ID:productID+productTicker+counter (e.g. ORDER2034T11)
		orderId = ("ORDER"+ std::to_string(product.GetMaturityDate().year()) + product.GetTicker() + std::to_string(counter));

		// parent order ID (null)
		string parentOrderId("N/A");

		// ischild to decide if this order is a child order or not (always false)
		bool isChild(false);

		// quantity
		long totalQt;
		long visibleQt;
		long hiddenQt;

		// immediate-or-cancel order type
		OrderType type = IOC;

		// determine the side of the execution
		PricingSide side = (counter % 2 == 1) ? BID : OFFER;

		// visible : hidden = 1 : 3 the same with that in BondAlgoStreamingService 
		totalQt = (side == OFFER)? bestBidOffer.GetOfferOrder().GetQuantity(): bestBidOffer.GetBidOrder().GetQuantity();
		hiddenQt = totalQt * 2.0 / 3.0;
		visibleQt = totalQt - hiddenQt;

		// generate the execution order
		Bond_ExOrder execution(product, side, orderId, type, bestoffer, visibleQt, hiddenQt, parentOrderId, isChild);

		// Add an algo execution related to the execution order to the stored data
		Bond_AgEx algoexecution(execution);
		if (id_AgEx_map.find(productId) == id_AgEx_map.end()) // if not found this one then create one
			id_AgEx_map.insert(std::make_pair(productId, algoexecution));
		else
			id_AgEx_map[productId] = algoexecution;

		// Call the listeners (update)
		for (auto private_l : listeners)
			private_l->ProcessUpdate(algoexecution);

		counter++;
	}

}

BondAlgoExecutionListener::BondAlgoExecutionListener(BondAlgoExecutionService* _bondAlgoExecutionService) :
	bondAlgoExecutionService(_bondAlgoExecutionService)
{
}

void BondAlgoExecutionListener::ProcessAdd(BondOrderBook &_bondOrderBook)
{
	bondAlgoExecutionService->AddOrder(_bondOrderBook);
}

void BondAlgoExecutionListener::ProcessRemove(BondOrderBook &_bondOrderBook)
{ // not defined for this service
}

void BondAlgoExecutionListener::ProcessUpdate(BondOrderBook &_bondOrderBook)
{ // not defined for this service
}

#endif // ! BONDALGOEXECUTION_HPP
