#include "distancevector.h"

DistanceVector::DistanceVector(unsigned n, SimulationContext* c, double b, double l) :
    Node(n, c, b, l)
{}

DistanceVector::DistanceVector(const DistanceVector & rhs) :
    Node(rhs)
{
    *this = rhs;
}

DistanceVector & DistanceVector::operator=(const DistanceVector & rhs) {
    Node::operator=(rhs);
    return *this;
}

DistanceVector::~DistanceVector() {}


/** Write the following functions.  They currently have dummy implementations **/

/*
 Handle an update to a link connected to this node
*/
void DistanceVector::LinkHasBeenUpdated(Link* l) {
    
	cerr << *this << ": Link Update: " << *l << endl;

	// Get the updated latency to the speicifed destination node
	int new_link_latency = l->GetLatency();		// Maybe change back to double and unsigned?
	int link_destination = l->GetDest();

	// Update this link in the links_to_neighbors map
	// Then set the cost to this destination in the dv to -1, alerts the distance vector update function to a potential change, initializes dv
	routing_table.links_to_neighbors[link_destination].cost = new_link_latency;
	routing_table.my_distance_vector[link_destination].cost = -1;
	routing_table.topo[link_destination][link_destination].cost = 0;
	
	// Only need to send message to neighbors if there's been a distance vector change
	bool message_needs_sent = Update();
	if(message_needs_sent) {
		cerr << "RoutingMessage from " << number << " being sent out to neighbors" << endl;
		SendToNeighbors(new RoutingMessage(number, routing_table.my_distance_vector));
	}
}

/*
 Handle incoming messages to this node from other nodes
*/
void DistanceVector::ProcessIncomingRoutingMessage(RoutingMessage *m) {
    cerr << *this << " got a routing message: " << *m << " (ignored)" << endl;

	// Get information from the received message, update routing table with new distance vector
	int message_is_from = m->source_node;
	map<int, TopoLink> incoming_dv = m->distance_vector;
	routing_table.topo[message_is_from] = incoming_dv;

	// Look at all of the nodes in the received dv and initialize any newcomers to this node's dv
	// (using method mentioned in project-3-hints.txt)
	map<int, TopoLink>::iterator itr;
	for(itr = incoming_dv.begin(); itr != incoming_dv.end(); itr++) {
		if(routing_table.my_distance_vector[itr->first].cost == -1) {}
	}

	// Only need to send message to neighbors if there's been a distance vector change
	bool message_needs_sent = Update();
	if(message_needs_sent) {
		cerr << "RoutingMessage from " << number << " being sent out to neighbors" << endl; 
		SendToNeighbors(new RoutingMessage(number, routing_table.my_distance_vector));
	}
}

void DistanceVector::TimeOut() {
    cerr << *this << " got a timeout: (ignored)" << endl;
}

/*
 Return a copy of the next hop to the destination
*/
Node* DistanceVector::GetNextHop(Node *destination) { 
	cerr << "GetNextHop() called going to " << destination->GetNumber() << ". Returning " << routing_table.next_hop[destination->GetNumber()] << endl;	
	
	// Iterate through all neighbors to find the node matching destination, grab bw and lat
	Node *copy_node;
	int looking_for = routing_table.next_hop[destination->GetNumber()];
	deque<Node*> neighbors = *Node::GetNeighbors();
	deque<Node*>::iterator neighbors_itr;
	
	for(neighbors_itr = neighbors.begin(); neighbors_itr != neighbors.end(); neighbors_itr++) {
		if(unsigned(looking_for) == (*neighbors_itr)->GetNumber()) {
			double latency_copy = (*neighbors_itr)->GetLatency();
			double bw_copy = (*neighbors_itr)->GetBW();	
			//cerr << latency_copy << " " << bw_copy << endl;		
			copy_node = new Node(looking_for, context, bw_copy, latency_copy);
		}
	}

	return copy_node;
}

Table* DistanceVector::GetRoutingTable() {
	cerr << "GetRoutingTable() called" << endl;
    	Table* copy_table = new Table(routing_table);
	return copy_table;
}

/*
 Updates this node's table (if need be) and returns a bool indicating whether or not a message needs to be sent to neighbors
*/
bool DistanceVector::Update() {
	
	cerr << "Updating the table for " << number << endl;
	

	// Variable that will be returned. Let's caller know if a message needs to be sent
	bool message_needs_sent = false;	

	// Now loop through each destination node and update (potentially) its distance vector
	map<int, TopoLink>::iterator dv_itr;
	for(dv_itr = routing_table.my_distance_vector.begin(); dv_itr != routing_table.my_distance_vector.end(); dv_itr++) {
		
		int current_destination = dv_itr->first;

		// If the node being viewed is the node acting, set its value in the dv to 0
		if((unsigned)current_destination == number) {
			routing_table.my_distance_vector[current_destination].cost = 0;
		}

		// Otherwise, perform update loop with all neighboring nodes [using Bellman-Ford equatio -> Dx(y) = minv{c(x,v) + Dv(y)]
		else {
			map<int, TopoLink>::iterator neighbors_itr;			
			int current_best_cost = POS_INF, next_cost = 0, next_hop = -1;

			for(neighbors_itr = routing_table.links_to_neighbors.begin(); neighbors_itr != routing_table.links_to_neighbors.end(); neighbors_itr++) {
				int current_neighbor = neighbors_itr->first;

				// Check to make sure that Dv(y) and c(x,v) are both available, then calculate next cost to be compared
				if(routing_table.topo[current_neighbor][current_destination].cost != -1 && routing_table.links_to_neighbors[current_neighbor].cost != -1) {
					next_cost = routing_table.links_to_neighbors[current_neighbor].cost + routing_table.topo[current_neighbor][current_destination].cost;
				
					cerr << "Inside if inside for loop" << endl;

					// Compare new value to to currently stored best value, potentially update next hop and current_best_cost
					if(next_cost < current_best_cost) {
						next_hop = current_neighbor;					
						current_best_cost = next_cost;
					}

				}
			}	
			
			cerr << "Best cost: " << current_best_cost << endl;
			// If there has been an update in the distance vector, set that a message needs to be sent, next_hop map value, distance vector to destination
			if(current_best_cost != POS_INF && current_best_cost != routing_table.my_distance_vector[current_destination].cost) {
				routing_table.my_distance_vector[current_destination].cost = current_best_cost;
				routing_table.next_hop[current_destination] = next_hop;
				message_needs_sent = true;
			}
		}

	}

	//cerr << "Does a message need sent: " << message_needs_sent << endl;
	return message_needs_sent;
}


ostream & DistanceVector::Print(ostream &os) const { 
    Node::Print(os);
    return os;
}
