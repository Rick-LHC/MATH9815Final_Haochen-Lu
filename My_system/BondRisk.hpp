// BondRiskService for modeling the risk of bond positions
// BondRiskListener for the data flow from BondPositionService to BondRiskListener

#ifndef BONDRISK_HPP
#define BONDRISK_HPP

#include<vector>
#include<unordered_map>
#include "riskservice.hpp"
#include "positionservice.hpp"
#include "productservice.hpp"
#include "products.hpp"
#include "soa.hpp"

// Bond risk service
class BondRiskService : public RiskService<Bond>
{
	typedef ServiceListener<BondPV01> myListener;
	typedef std::vector<myListener*> listener_container;
protected:
	BondProductService* bondProductService;
	listener_container listeners;
	std::unordered_map<string, BondPV01> pv01Map; // key on product identifier

	// key: sector name, value: sector	PV01
	std::unordered_map<string, PV01<BucketedSector<Bond>>> bucketpv01Map; 

public:
	BondRiskService(BondProductService*, std::unordered_map<string, double>&);

	// Get data on our service given a key
	virtual BondPV01 & GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(BondPV01 &);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(myListener *);

	// Get all listeners on the Service.
	virtual const listener_container& GetListeners() const;

	// Add a position that the service will risk
	virtual void AddPosition(BondPos &);

	// Update the bucketed risk for the bucket sector
	virtual void UpdateBucketedRisk(const BucketedSector<Bond> &);

	// Get the bucketed risk for the bucket sector
	virtual const PV01<BucketedSector<Bond>>& GetBucketedRisk(const BucketedSector<Bond> &) const;
};

// corresponding service listener
class BondRiskListener : public ServiceListener<BondPos>
{
protected:
	BondRiskService* bondRiskService;

public:
	BondRiskListener(BondRiskService*); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(BondPos &);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(BondPos &);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(BondPos &);
};

BondRiskService::BondRiskService(BondProductService* _bondProductService, std::unordered_map<string, double>& _pv01) :
	bondProductService(_bondProductService)
{
	for (auto& item:_pv01)
	{
		string productId = item.first;
		auto _bond = bondProductService->GetData(productId);
		pv01Map.insert(std::make_pair(productId,BondPV01(_bond, item.second, 0)));
	}
}

BondPV01 & BondRiskService::GetData(string key)
{
	return pv01Map[key];
}

void BondRiskService::OnMessage(BondPV01 &data)
{ 
	// No OnMessage() defined for the intermediate service 
}

void BondRiskService::AddListener(myListener *listener)
{
	listeners.push_back(listener);
}

const BondRiskService::listener_container& BondRiskService::GetListeners() const
{
	return listeners;
}

void BondRiskService::AddPosition(BondPos &position)
{
	// get the corresponding pv01
	string productId = position.GetProduct().GetProductId();
	BondPV01 productPv = pv01Map[productId];

	// Update the pv01 object
	long long newQt = position.GetAggregatePosition() + productPv.GetQuantity();
	BondPV01 new_productPV(productPv.GetProduct(), productPv.GetPV01(), newQt);
	pv01Map[productId] = new_productPV;

	// call the listeners to update
	for (auto listener : listeners)
		listener->ProcessUpdate(new_productPV);
}

void BondRiskService::UpdateBucketedRisk(const BucketedSector<Bond> &sector)
{
	std::vector<Bond> products = sector.GetProducts();
	string productId;
	long long sum_qt = 0;
	double sum_pv01 = 0.0;

	// calculate the overall pv01 and the overall quantity
	for (Bond product : products)
	{
		productId = product.GetProductId();

		auto tempPV = pv01Map.at(productId);
		auto tempQt = tempPV.GetQuantity();
		sum_qt += tempQt;
		sum_pv01 += tempPV.GetPV01() * tempQt;
	}

	double unit_pv01 = 0.0;
	if (sum_qt != 0)
		unit_pv01 = sum_pv01 / sum_qt;

	PV01<BucketedSector<Bond>> bucketpv01(sector, unit_pv01, sum_qt);

	// insert the data to the map
	string name = sector.GetName();
	if (bucketpv01Map.find(name) == bucketpv01Map.end()) 
		// if this product does not exist before, creat one
		bucketpv01Map.insert(std::make_pair(name, bucketpv01));
	else
		bucketpv01Map[name] = bucketpv01;
}

const PV01<BucketedSector<Bond>>& BondRiskService::GetBucketedRisk(const BucketedSector<Bond> &sector) const
{
	std::string name = sector.GetName();
	return bucketpv01Map.at(name);

}

BondRiskListener::BondRiskListener(BondRiskService* _bondRiskService) :
	bondRiskService(_bondRiskService){}

void BondRiskListener::ProcessAdd(BondPos &_bondPos)
{ 
	// not defined for this service
}

void BondRiskListener::ProcessRemove(BondPos &_bondPos)
{ 
	// not defined for this service
}

void BondRiskListener::ProcessUpdate(BondPos &_bondPos)
{
	bondRiskService->AddPosition(_bondPos);
}

#endif // !BONDRISK_HPP
