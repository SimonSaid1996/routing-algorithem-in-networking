#include "messages.h"

RoutingMessage::RoutingMessage()
{

}

RoutingMessage::RoutingMessage(const RoutingMessage &rhs) {
    *this = rhs;
}

RoutingMessage & RoutingMessage::operator=(const RoutingMessage & rhs) {
    /* For now.  Change if you add data members to the struct */
    return *this;
}

#if defined(GENERIC)
ostream &RoutingMessage::Print(ostream &os) const
{
    os << "Generic RoutingMessage()";
    return os;
}
#endif

#if defined(LINKSTATE)
RoutingMessage::RoutingMessage(int m_f, deque<Link*> *l) {
	message_from = m_f;
	links_to_neighbors = l;
}

ostream &RoutingMessage::Print(ostream &os) const
{
    os << "LinkState RoutingMessage()";
    return os;
}
#endif

#if defined(DISTANCEVECTOR)

// Constructor for distance-vector-specific message
RoutingMessage::RoutingMessage(unsigned src, map<int, TopoLink> dv) {
	source_node = src;
	distance_vector = dv;
}

ostream &RoutingMessage::Print(ostream &os) const
{
    os << "DistanceVector RoutingMessage()";
    return os;
}
#endif
