/* -*- mode: C++; indent-tabs-mode: nil; -*-
*
* This file is a part of LEMON, a generic C++ optimization library.
*
* Copyright (C) 2003-2013
* Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
* (Egervary Research Group on Combinatorial Optimization, EGRES).
*
* Permission to use, modify and distribute this software is granted
* provided that this copyright notice appears in all copies. For
* precise terms see the accompanying LICENSE file.
*
* This software is provided "AS IS" with no warranty of any kind,
* express or implied, and with no claim as to its suitability for any
* purpose.
*
*/

#include <lemon/concepts/digraph.h>
#include <lemon/smart_graph.h>
#include <lemon/list_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/dfs.h>
#include <lemon/path.h>
#include <lemon/list_graph.h>
#include <lemon/maps.h>
#include <lemon/dfs.h>
#include <lemon/connectivity.h>
#include <lemon/dijkstra.h>
#include <lemon/edge_set.h>
#include <numeric>


using namespace lemon;
using namespace std;

struct R {
	int id;
	int iTime;
	int iCostM;
	int iCostG;
	R(int i, int t, int m, int g) : id(i), iTime(t), iCostM(m), iCostG(g) {};
	R():id(0), iTime(0), iCostM(0), iCostG(0) {};
};


class MyOps : public DijkstraDefaultOperationTraits<int> {


};

class BuildPlanner : public DfsVisitor<ListDigraph>
{
	typedef SmartArcSet<ListDigraph>::Arc Arc;
public:
	BuildPlanner( ListDigraph &aGraph): iBuildMap(aGraph)
	{
	}

	void discover(const Arc &arc) {
		iBuildMap.addArc(iBuildMap.target(arc), iBuildMap.source(arc));
	}

	void examine(const Arc &arc) {
		iBuildMap.addArc(iBuildMap.target(arc), iBuildMap.source(arc));
	}

	SmartArcSet<ListDigraph>  iBuildMap;
};

class MyDfsVisitor : public DfsVisitor<ListDigraph>
{
public:
	MyDfsVisitor(map<int, int> &aTimeMap, ListDigraph &aGraph, vector<R> &aNodeMap) :iTimeMap(aTimeMap), iGraph(aGraph), iNodeMap(aNodeMap)
	{
	}
	
	void start(const Node &node) {
		iTimeMap[iGraph.id(node)] = max( iTimeMap[iGraph.id(node)], iNodeMap[iGraph.id(node)].iTime );
		DfsVisitor<ListDigraph>::start(node);
	}

	void discover(const Arc &arc) {
		int source = iGraph.id(iGraph.source(arc));		
		int target = iGraph.id(iGraph.target(arc));
		// cumulative time to build source + time to build target
		int iCumalativeTime = iTimeMap[source] + iNodeMap[target].iTime;
		iTimeMap[target] =  iCumalativeTime;	
	}

	void examine(const Arc &arc) {
		int source = iGraph.id(iGraph.source(arc));
		int target = iGraph.id(iGraph.target(arc));
		//  cumulative time to build source + time to build target
		int iCumalativeTime = iTimeMap[source] + iNodeMap[target].iTime;
		// new cumulative time is the larger of any cumulative paths to the target
		if (iCumalativeTime > iTimeMap[target])
		{
			iTimeMap[target] = max(iCumalativeTime, iTimeMap[target]);
			DfsVisit<ListDigraph, MyDfsVisitor> v(iGraph, MyDfsVisitor(iTimeMap, iGraph, iNodeMap));
			v.init();
			v.run(iGraph.source(arc));
		}
	}

	map<int, int> &iTimeMap;
	ListDigraph &iGraph;
	vector<R>   &iNodeMap;
};


int main()
{
	ListDigraph d;

	
	int n_nodes = 12;
	vector<R>   lNodeMap(n_nodes);

	while (n_nodes--) {
		d.addNode();
	}

	lNodeMap[0] = R(0, 0, 50, 0);
	lNodeMap[1] = R(1, 30, 100, 100);
	lNodeMap[2] = R(2, 30, 50, 50);
	lNodeMap[3] = R(3, 45, 100, 0);
	lNodeMap[4] = R(4, 15, 100, 0);
	lNodeMap[5] = R(5, 30, 75, 50);
	lNodeMap[6] = R(6, 30, 125, 0);
	lNodeMap[7] = R(7, 60, 350, 1000);
	lNodeMap[8] = R(8, 10, 50, 0);
	lNodeMap[9] = R(9, 20, 150, 0);
	// new
	
	lNodeMap[10] = R(10, 20, 75, 75);
	lNodeMap[11] = R(11, 20, 50, 50);
					  

	d.addArc(7,1);
	d.addArc(7, 5);
	d.addArc(1,2);
	d.addArc(2, 3);
	d.addArc(5, 3);
	d.addArc(7, 8);
	d.addArc(8, 4);
	d.addArc(7, 9);


	//BuildPlanner planner(d);
	//DfsVisit<SmartArcSet<ListDigraph>, BuildPlanner, DfsVisitDefaultTraits<SmartArcSet<ListDigraph>>> bp(d, planner);
	//bp.init();
	//bp.run(4);
	//
	//for (SmartArcSet<ListDigraph>::ArcIt a(planner.iBuildMap); a != INVALID; ++a) {
	//	cout << planner.iBuildMap.source(a) << " --> " << planner.iBuildMap.target(a) << '\n';
	//}

	//

	map<int, int> lTimeMap;
	DfsVisit<ListDigraph, MyDfsVisitor> v(d, MyDfsVisitor(lTimeMap, d, lNodeMap));
	v.init();
	v.run(d.nodeFromId(7));

	struct compare_times {
		bool operator() (const pair<int,int> &lhs, const pair<int,int> &rhs) { return lhs.second > rhs.second; }
	};
	multiset < pair< int, int>, compare_times> myset(lTimeMap.begin(), lTimeMap.end());
	
	int lmin=0, lgas=0;
	std::cout << "node\tstart\tbtime\tgas\tmin\ttgas\ttgas\n";
	const char tab = '\t';
	int total_time = myset.begin()->second;
	for (auto i : myset) {
		lmin += lNodeMap[i.first].iCostM;
		lgas += lNodeMap[i.first].iCostG;
		std::cout << i.first << tab << total_time - i.second << tab << lNodeMap[i.first].iTime << tab << lNodeMap[i.first].iCostG << tab << lNodeMap[i.first].iCostM << tab << lmin << tab << lgas << '\n';
	}

	std::cout << endl;

	return 0;
}

