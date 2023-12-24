#ifndef BONDALGOSTREAMING_HPP
#define BONDALGOSTREAMING_HPP

#include "pricingservice.hpp"
#include "streamingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

// Algo Stream with a price stream on both side
// Type T is the product type
template<typename T>
class AlgoStream
{
private:
	PriceStream<T> stream;

public:
	AlgoStream(const PriceStream<T>& _stream); // ctor
	AlgoStream() = default;

	// Get the price stream
	const PriceStream<T>& GetStream() const;

};

//Type alias
typedef AlgoStream<Bond> Bond_Ags;

// Bond algo streaming service to determine the prices on both sides
// key on the product identifier
// value on an AlgoStream object
class BondAlgoStreamingService : public Service<string, Bond_Ags>
{
	typedef ServiceListener<Bond_Ags> myListener;
	typedef vector<myListener*> Listener_container;
protected:
	Listener_container listeners;

	// key: product identifier, value: AlgoStream object
	std::unordered_map<string, Bond_Ags> algostream_Map;

	// help to decide the quantity of the price stream
	long counter = 0; 

public:
	BondAlgoStreamingService() {}

	// Get data on our service given a key
	virtual Bond_Ags & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_Ags &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const Listener_container& GetListeners() const;

	// Generate the price stream and update it to the stored data
	virtual void AddStream(const BondPrice&);
};

// Bond algo-streaming service listener
// register in the bond pricing service to process the price data to BondAlgoStreamingService
class ToBondAlgoStreamingListener : public ServiceListener<BondPrice>
{
protected:
	BondAlgoStreamingService* bondAlgoStreamingService;

public:
	ToBondAlgoStreamingListener(BondAlgoStreamingService*);

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondPrice &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondPrice &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondPrice &);
};

template <typename T>
AlgoStream<T>::AlgoStream(const PriceStream<T>& _stream) : stream(_stream){}

template <typename T>
const PriceStream<T>& AlgoStream<T>::GetStream() const
{
	return stream;
}

Bond_Ags & BondAlgoStreamingService::GetData(string key)
{
	return algostream_Map[key];
}

void BondAlgoStreamingService::OnMessage(Bond_Ags &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondAlgoStreamingService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondAlgoStreamingService::Listener_container& BondAlgoStreamingService::GetListeners() const
{
	return listeners;
}

void BondAlgoStreamingService::AddStream(const BondPrice& _bondPrice)
{
	// mid price and bid-offer spread from BondPrice object
	double mid = _bondPrice.GetMid();
	double spread = _bondPrice.GetBidOfferSpread();

	// visibleQt:hiddenQt=1:2
	long visibleQt = (counter % 2 == 0) ? 1000000 : 2000000;

	long hiddenQt = 2 * visibleQt;
	double gap = spread / 2;

	//creat Order
	PriceStreamOrder bidOrder(mid - gap, visibleQt, hiddenQt, BID);
	PriceStreamOrder offerOrder(mid + gap, visibleQt, hiddenQt, OFFER);

	// Generate a price stream
	Bond_Ps stream(_bondPrice.GetProduct(), bidOrder, offerOrder);
	Bond_Ags algostream(stream);

	// Add an algo stream related to the price stream to the stored data
	string pd_id = _bondPrice.GetProduct().GetProductId();
	
	if (algostream_Map.find(pd_id) == algostream_Map.end()) // if not found this one then create one
		algostream_Map.insert(std::make_pair(pd_id, algostream));
	else
		algostream_Map[pd_id] = algostream;

	counter++;

	// Call the listeners (update)
	for (auto temp_l : listeners)
		temp_l->ProcessUpdate(algostream);
}

ToBondAlgoStreamingListener::ToBondAlgoStreamingListener(BondAlgoStreamingService* _bondAlgoStreamingService) :
	bondAlgoStreamingService(_bondAlgoStreamingService){}

void ToBondAlgoStreamingListener::ProcessAdd(BondPrice &_BondPrice)
{
	// Add a algo stream based on the data
	bondAlgoStreamingService->AddStream(_BondPrice);
}

void ToBondAlgoStreamingListener::ProcessRemove(BondPrice &_BondPrice)
{ 
	// not defined for this service
}

void ToBondAlgoStreamingListener::ProcessUpdate(BondPrice &_BondPrice)
{ 
	// not defined for this service
}

#endif // !BONDALGOSTREAMING_HPP
