/*
 * $FU-Copyright$
 */

#ifndef OMNET_EARA_CONFIGURATION_H_
#define OMNET_EARA_CONFIGURATION_H_

#include "OMNeTConfiguration.h"
#include "EARAConfiguration.h"
#include "EnergyAwareRoutingTable.h"
#include "OMNeTEARAForwardingPolicy.h"

OMNETARA_NAMESPACE_BEGIN

class OMNeTEARAConfiguration : public virtual OMNeTConfiguration, public EARAConfiguration {
    public:
        OMNeTEARAConfiguration(cModule* module);

        virtual unsigned int getMaximumEnergyValue() const;
        virtual float getInfluenceOfMinimumEnergyValue() const;
        virtual unsigned int getRouteDiscoveryDelayInMilliSeconds() const;
        virtual ARA::EnergyAwareRoutingTable* getEnergyAwareRoutingTable() const;
        virtual ARA::EARAPacketFactory* getEARAPacketFactory() const;
        virtual EARAForwardingPolicy* getForwardingPolicy();
        virtual float getPEANTEnergyThreshold() const;

    private:
        float influenceOfMinimumEnergyValue;
        unsigned int maximumEnergyValue;
        unsigned int routeDiscoveryDelayInMilliSeconds;
        float peantEnergyThreshold;
};

OMNETARA_NAMESPACE_END
#endif
