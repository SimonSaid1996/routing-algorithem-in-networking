#ifndef _messages
#define _messages

#include <iostream>
#include "node.h"
#include "link.h"

struct RoutingMessage {
    RoutingMessage();
    RoutingMessage(const RoutingMessage &rhs);
    RoutingMessage &operator=(const RoutingMessage &rhs);

    ostream & Print(ostream &os) const;

    // Anything else you need

    #if defined(LINKSTATE)
	RoutingMessage(int m_f, deque<Link*> *l);
	int message_from;
	deque<Link*> *links_to_neighbors;
	
    #endif
    #if defined(DISTANCEVECTOR)
	RoutingMessage(unsigned src, map<int, TopoLink> dv);	// Special constructor for distance vector message
	unsigned source_node;									// The node that the message is coming from
	map<int, TopoLink> distance_vector;						// That node's potentially updated distance vector
    #endif
};

inline ostream & operator<<(ostream &os, const RoutingMessage & m) { return m.Print(os);}

#endif
