#ifndef _table
#define _table

#include <iostream>
#include <map>
#include <deque>
#include "link.h"

using namespace std;

struct TopoLink {
    TopoLink(): cost(-1), age(0) {}

    TopoLink(const TopoLink & rhs) {
        *this = rhs;
    }

    TopoLink & operator=(const TopoLink & rhs) {
        this->cost = rhs.cost;
        this->age = rhs.age;

        return *this;
    }

    int cost;
    int age;
};

// Students should write this class
class Table {
    private:
        
    public:
        Table();
        Table(const Table &);
        Table & operator=(const Table &);

        ostream & Print(ostream &os) const;

        // Anything else you need
	

        #if defined(LINKSTATE)
			map < int, deque<Link*> *> topo;		// the topology that this node knows
			map<int, int> next_hop;				// used to determine the best next hop
        #endif
        #if defined(DISTANCEVECTOR)
			map < int, map < int, TopoLink > > topo;	// the actual 2-D array (routing table)
			map<int, TopoLink> links_to_neighbors;		// all links to neighboring nodes
			map<int, TopoLink> my_distance_vector; 		// this node's dv (for sending)
			map<int, int> next_hop;						// helps with GetNextHop()
        #endif
};

inline ostream & operator<<(ostream &os, const Table & t) { return t.Print(os);}


#endif
