// BondStreamingService
// ToBondStreamingListener for the data inflow from BondAlgoStreamingService

#ifndef BONDSTREAMING_HPP
#define BONDSTREAMING_HPP

#include "streamingservice.hpp"
#include "BondAlgoStreaming.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

class BondStreamingService : public StreamingService<Bond>
{
	typedef ServiceListener<Bond_Ps> myListener;
	typedef vector<myListener*> Listener_container;
protected:
	Listener_container listeners;
	std::unordered_map<string, Bond_Ps> stream_Map; // key on product identifier
public:
	BondStreamingService() {};

	// Get data on our service given a key
	virtual Bond_Ps & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond_Ps &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener* );

	// Get all listeners on the Service.
	virtual const Listener_container& GetListeners() const;

	// Publish two-way prices
	void PublishPrice(const Bond_Ps& );
};

// Bond streaming service listener
class ToBondStreamingListener : public ServiceListener<Bond_Ags>
{
protected:
	BondStreamingService* bondStreamingService;

public:
	ToBondStreamingListener(BondStreamingService* );

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Bond_Ags &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Bond_Ags &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Bond_Ags &);
};

Bond_Ps & BondStreamingService::GetData(string key)
{
	return stream_Map[key];
}

void BondStreamingService::OnMessage(Bond_Ps &_Bond_Ps)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondStreamingService::AddListener(myListener *_myListener)
{
	listeners.push_back(_myListener);
}

const BondStreamingService::Listener_container& BondStreamingService::GetListeners() const
{
	return listeners;
}

void BondStreamingService::PublishPrice(const Bond_Ps& priceStream)
{
	// push data to the map
	string pd_id = priceStream.GetProduct().GetProductId();
	if (stream_Map.find(pd_id) == stream_Map.end()) // if not found this one then create one
		stream_Map.insert(std::make_pair(pd_id, priceStream));
	else
		stream_Map[pd_id] = priceStream;

	// call the listeners
	Bond_Ps temp_Bond_Ps(priceStream);
	for (auto private_l : listeners)
		private_l->ProcessAdd(temp_Bond_Ps);
}

ToBondStreamingListener::ToBondStreamingListener(BondStreamingService* _bondStreamingService):
	bondStreamingService(_bondStreamingService){}

void ToBondStreamingListener::ProcessAdd(Bond_Ags &_bond_Ags)
{ 
	// not defined for this service
}

void ToBondStreamingListener::ProcessRemove(Bond_Ags &_bond_Ags)
{ 
	// not defined for this service
}

void ToBondStreamingListener::ProcessUpdate(Bond_Ags &_bond_Ags)
{
	// publish the price stream
	Bond_Ps stream = _bond_Ags.GetStream();
	bondStreamingService->PublishPrice(stream);
}

#endif // !BONDSTREAMING_HPP
