/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2021 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    DijkstraRouter.h
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @author  Michael Behrisch
/// @date    Mon, 25 July 2005
///
// Dijkstra shortest path algorithm using travel time or other values
/****************************************************************************/
#pragma once
#include <config.h>

#include <cassert>
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <limits>
#include <algorithm>
#include <iterator>
#include <utils/common/ToString.h>
#include <utils/common/MsgHandler.h>
#include <utils/common/StdDefs.h>
#include "EffortCalculator.h"
#include "SUMOAbstractRouter.h"
#include "MapKTree.h"

//#define DijkstraRouter_DEBUG_QUERY
//#define DijkstraRouter_DEBUG_QUERY_VISITED
//#define DijkstraRouter_DEBUG_QUERY_PERF
//#define DijkstraRouter_DEBUG_BULKMODE

#define DijkstraRouter_DEBUG_QUERY_VISITED_OUT std::cerr

#define CA_SUMO true // Adevarat pentru CA_SUMO si false pentru SUMO
#define VEHICLE_LENGTH 7.0 // Lungimea totala a unui vehicul plus o distanta de siguranta intre masini
#define BASE_DELAY 1 // O intarziere specifica structurii oraselor

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class DijkstraRouter
 * @brief Computes the shortest path through a network using the Dijkstra algorithm.
 *
 * The template parameters are:
 * @param E The edge class to use (MSEdge/ROEdge)
 * @param V The vehicle class to use (MSVehicle/ROVehicle)
 *
 * The router is edge-based. It must know the number of edges for internal reasons
 *  and whether a missing connection between two given edges (unbuild route) shall
 *  be reported as an error or as a warning.
 *
 */

template<class E, class V>
class DijkstraRouter : public SUMOAbstractRouter<E, V> {
public:
    MapKTree mapKTree;
    int vehicleId = 0;
    double totalLength = 0.0;
    /**
     * @class EdgeInfoByEffortComparator
     * Class to compare (and so sort) nodes by their effort
     */
    class EdgeInfoByEffortComparator {
    public:
        /// Comparing method
        bool operator()(const typename SUMOAbstractRouter<E, V>::EdgeInfo* nod1, const typename SUMOAbstractRouter<E, V>::EdgeInfo* nod2) const {
            if (nod1->effort == nod2->effort) {
                return nod1->edge->getNumericalID() > nod2->edge->getNumericalID();
            }
            return nod1->effort > nod2->effort;
        }
    };


    /// Constructor
    DijkstraRouter(const std::vector<E*>& edges, bool unbuildIsWarning, typename SUMOAbstractRouter<E, V>::Operation effortOperation,
                   typename SUMOAbstractRouter<E, V>::Operation ttOperation = nullptr, bool silent = false, EffortCalculator* calc = nullptr,
                   const bool havePermissions = false, const bool haveRestrictions = false) :
        SUMOAbstractRouter<E, V>("DijkstraRouter", unbuildIsWarning, effortOperation, ttOperation, havePermissions, haveRestrictions),
        mySilent(silent), myExternalEffort(calc) {
        for (typename std::vector<E*>::const_iterator i = edges.begin(); i != edges.end(); ++i) {
            this->myEdgeInfos.push_back(typename SUMOAbstractRouter<E, V>::EdgeInfo(*i));
        }
    }

    /// Destructor
    virtual ~DijkstraRouter() { }

    virtual SUMOAbstractRouter<E, V>* clone() {
        auto clone = new DijkstraRouter<E, V>(this->myEdgeInfos, this->myErrorMsgHandler == MsgHandler::getWarningInstance(),
                                              this->myOperation, this->myTTOperation, mySilent, myExternalEffort,
                                              this->myHavePermissions, this->myHaveRestrictions);
        clone->setAutoBulkMode(this->myAutoBulkMode);
        return clone;
    }

    /** @brief Builds the route between the given edges using the minimum effort at the given time
        The definition of the effort depends on the wished routing scheme */
    bool compute(const E* from, const E* to, const V* const vehicle,
                 SUMOTime msTime, std::vector<const E*>& into, bool silent = false) {
        assert(from != nullptr && (vehicle == nullptr || to != nullptr));
        // check whether from and to can be used
        if (this->myEdgeInfos[from->getNumericalID()].prohibited || this->isProhibited(from, vehicle)) {
            if (!silent) {
                this->myErrorMsgHandler->inform("Vehicle '" + Named::getIDSecure(vehicle) + "' is not allowed on source edge '" + from->getID() + "'.");
            }
            return false;
        }
        if (to != nullptr && (this->myEdgeInfos[to->getNumericalID()].prohibited || this->isProhibited(to, vehicle))) {
            if (!silent) {
                this->myErrorMsgHandler->inform("Vehicle '" + Named::getIDSecure(vehicle) + "' is not allowed on destination edge '" + to->getID() + "'.");
            }
            return false;
        }
        double length = 0.; // dummy for the via edge cost update
        this->startQuery();
#ifdef DijkstraRouter_DEBUG_QUERY
        std::cout << "DEBUG: starting search for '" << Named::getIDSecure(vehicle) << "' time: " << STEPS2TIME(msTime) << "\n";
#endif
        const SUMOVehicleClass vClass = vehicle == nullptr ? SVC_IGNORING : vehicle->getVClass();
        std::tuple<const E*, const V*, SUMOTime> query = std::make_tuple(from, vehicle, msTime);
        if ((this->myBulkMode || (this->myAutoBulkMode && query == myLastQuery)) && !this->myAmClean) {
#ifdef DijkstraRouter_DEBUG_BULKMODE
            if (query != myLastQuery) {
                std::cout << " invalid bulk mode. myLastQuery="
                          << std::get<0>(myLastQuery)->getID() << ","
                          << std::get<1>(myLastQuery)->getID() << ","
                          << time2string(std::get<2>(myLastQuery))
                          << " query="
                          << std::get<0>(query)->getID() << ","
                          << std::get<1>(query)->getID() << ","
                          << time2string(std::get<2>(query))
                          << "\n";
            }
#endif
            const auto& toInfo = this->myEdgeInfos[to->getNumericalID()];
            if (toInfo.visited) {
                this->buildPathFrom(&toInfo, into);
                this->endQuery(1);
                return true;
            }
        } else {
            this->init(from->getNumericalID(), msTime);
            if (myExternalEffort != nullptr) {
                myExternalEffort->setInitialState(from->getNumericalID());
            }
            this->myAmClean = false;
        }
        myLastQuery = query;
        // loop
        int num_visited = 0;
#ifdef DijkstraRouter_DEBUG_QUERY_VISITED
        DijkstraRouter_DEBUG_QUERY_VISITED_OUT << "<edgeData>\n <interval begin=\"" << time2string(msTime) << "\" end=\"" << toString(msTime + 1000) << "\">\n";
#endif
        int depTime = this->myFrontierList.front()->leaveTime;
        while (!this->myFrontierList.empty()) {
            num_visited += 1;
            // use the node with the minimal length
            auto* const minimumInfo = this->myFrontierList.front();
            const E* const minEdge = minimumInfo->edge;
#ifdef DijkstraRouter_DEBUG_QUERY
            std::cout << "DEBUG: hit '" << minEdge->getID() << "' Eff: " << minimumInfo->effort << ", Leave: " << minimumInfo->leaveTime << " Q: ";
            for (auto& it : this->myFrontierList) {
                std::cout << "\n   " << it->effort << ", " << it->edge->getID();
            }
            std::cout << "\n";
#endif
#ifdef DijkstraRouter_DEBUG_QUERY_VISITED
            DijkstraRouter_DEBUG_QUERY_VISITED_OUT << "  <edge id=\"" << minEdge->getID() << "\" index=\"" << num_visited << "\" cost=\"" << minimumInfo->effort << "\" time=\"" << minimumInfo->leaveTime << "\"/>\n";
#endif
            // check whether the destination node was already reached
            if (minEdge == to) {
                //propagate last external effort state to destination edge
                if (myExternalEffort != nullptr) {
                    myExternalEffort->update(minEdge->getNumericalID(), minimumInfo->prev->edge->getNumericalID(), minEdge->getLength());
                }
                this->buildPathFrom(minimumInfo, into);
                this->endQuery(num_visited);
#ifdef DijkstraRouter_DEBUG_QUERY_PERF
                const double cost = this->recomputeCosts(into, vehicle, msTime);
                std::cout << "visited " + toString(num_visited) + " edges (final path length=" + toString(into.size()) + " cost=" << cost << " edges=" + toString(into) + ")\n";
#endif
#ifdef DijkstraRouter_DEBUG_QUERY_VISITED
                DijkstraRouter_DEBUG_QUERY_VISITED_OUT << " </interval>\n</edgeData>\n";
#endif
                int startSegmentTime = (int)depTime;
                int endSegmentTime = 0;
                int size = into.size();
                auto edge = into.at(0);
                double length = 0.0;

                // Actualizarea structurii K-ary Single Point conform rutei alese
                for (int i = 0; i < size; i++)
                {
                    edge = into.at(i);

                    // Estimam viteza de deplasare a unui vehicul pe un segment
                    const double realSpeed = mapKTree.getRealSpeed(startSegmentTime, edge->getID(), edge->getLength(), 
                                                                   VEHICLE_LENGTH, edge->mySpeed2, edge->numLanes);
                    //calculam distanta parcursa
                    length += edge->getLength();

                    double tlsDelay = mapKTree.getDelay(startSegmentTime, edge->getID(), VEHICLE_LENGTH, 
                                                        edge->getLength(), edge->numLanes, BASE_DELAY);
                    // Estimam timpul total de deplasare pe un segment
                    endSegmentTime = startSegmentTime + edge->getLength() / realSpeed + tlsDelay;
                    // Actualizam structura K-ary Single Point
                    mapKTree.update(edge->getID(), startSegmentTime, endSegmentTime);
                    startSegmentTime = endSegmentTime;
                }

                // Salvam distanta rutei intr-un fisier
                ofstream file;
                file.open("length.txt", fstream::app);
                file << vehicle->getID() << ":" << length << endl;
                file.close();

                return true;
            }
            std::pop_heap(this->myFrontierList.begin(), this->myFrontierList.end(), myComparator);
            this->myFrontierList.pop_back();
            this->myFound.push_back(minimumInfo);
            
            minimumInfo->visited = true;
            const double effortDeltaOld = this->getEffort(minEdge, vehicle, minimumInfo->leaveTime);
            // Actualizam efortul utilizand gradul de ocupare al drumurilor cu (delay)?
            const double effortDelta = mapKTree.getEdgeEffort(minimumInfo->leaveTime, minEdge->getID(), VEHICLE_LENGTH,
                                                                minEdge->getLength(), effortDeltaOld, minEdge->numLanes, CA_SUMO, BASE_DELAY);
            const double leaveTime = minimumInfo->leaveTime + this->getTravelTime(minEdge, vehicle, minimumInfo->leaveTime, effortDelta);

            if (myExternalEffort != nullptr) {
                myExternalEffort->update(minEdge->getNumericalID(), minimumInfo->prev->edge->getNumericalID(), minEdge->getLength());
            }
            // check all ways from the node with the minimal length
            for (const std::pair<const E*, const E*>& follower : minEdge->getViaSuccessors(vClass)) {
                visited_nr++;
                auto& followerInfo = this->myEdgeInfos[follower.first->getNumericalID()];
                // check whether it can be used
                if (followerInfo.prohibited || this->isProhibited(follower.first, vehicle)) {
                    continue;
                }
                double effort = minimumInfo->effort + effortDelta;
                double time = leaveTime;
                this->updateViaEdgeCost(follower.second, vehicle, time, effort, length);
                assert(effort >= minimumInfo->effort);
                assert(time >= minimumInfo->leaveTime);
                const double oldEffort = followerInfo.effort;
                if (!followerInfo.visited && effort < oldEffort) {
                    followerInfo.effort = effort;
                    followerInfo.leaveTime = time;
                    followerInfo.prev = minimumInfo;
                    if (oldEffort == std::numeric_limits<double>::max()) {
                        this->myFrontierList.push_back(&followerInfo);
                        std::push_heap(this->myFrontierList.begin(), this->myFrontierList.end(), myComparator);
                    } else {
                        std::push_heap(this->myFrontierList.begin(),
                                       std::find(this->myFrontierList.begin(), this->myFrontierList.end(), &followerInfo) + 1,
                                       myComparator);
                    }
                }
            }
        }
        this->endQuery(num_visited);
#ifdef DijkstraRouter_DEBUG_QUERY_PERF
        std::cout << "visited " + toString(num_visited) + " edges (unsuccessful path length: " + toString(into.size()) + ")\n";
#endif
        if (to != nullptr && !mySilent && !silent) {
            this->myErrorMsgHandler->informf("No connection between edge '%' and edge '%' found.", from->getID(), to->getID());
        }
#ifdef DijkstraRouter_DEBUG_QUERY_VISITED
        DijkstraRouter_DEBUG_QUERY_VISITED_OUT << " </interval>\n</edgeData>\n";
#endif
        return false;
    }

private:
    DijkstraRouter(const std::vector<typename SUMOAbstractRouter<E, V>::EdgeInfo>& edgeInfos, bool unbuildIsWarning,
                   typename SUMOAbstractRouter<E, V>::Operation effortOperation, typename SUMOAbstractRouter<E, V>::Operation ttOperation, bool silent, EffortCalculator* calc,
                   const bool havePermissions, const bool haveRestrictions) :
        SUMOAbstractRouter<E, V>("DijkstraRouter", unbuildIsWarning, effortOperation, ttOperation, havePermissions, haveRestrictions),
        mySilent(silent),
        myExternalEffort(calc) {
        for (const auto& edgeInfo : edgeInfos) {
            this->myEdgeInfos.push_back(typename SUMOAbstractRouter<E, V>::EdgeInfo(edgeInfo.edge));
        }
    }

private:
    /// @brief whether to suppress warning/error if no route was found
    bool mySilent;

    /// cache of the last query to enable automated bulk routing
    std::tuple<const E*, const V*, SUMOTime> myLastQuery;

    EffortCalculator* const myExternalEffort;

    EdgeInfoByEffortComparator myComparator;
};
