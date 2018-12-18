#include "linkstate.h"
#include <queue>

LinkState::LinkState(unsigned n, SimulationContext* c, double b, double l) :
    Node(n, c, b, l)
{}

LinkState::LinkState(const LinkState & rhs) :
    Node(rhs)
{
    *this = rhs;
}

LinkState & LinkState::operator=(const LinkState & rhs) {
    Node::operator=(rhs);
    return *this;
}

LinkState::~LinkState() {}


/** Write the following functions.  They currently have dummy implementations **/
void LinkState::LinkHasBeenUpdated(Link* l) {
    cerr << *this << ": Link Update: " << *l << endl;
	int test = (*l).GetSrc();
	// Update the routing table's topology with the new link, perform Dijkstra's to recalculate next hops
	//routing_table.topo[number] = this->GetOutgoingLinks();
	routing_table.topo[test] = this->GetOutgoingLinks();
	Dijkstra();

	// Assemble the routing message with info about node's new links and send it to all neighbors
	SendToNeighbors(new RoutingMessage(number, routing_table.topo[number]));
}

void LinkState::ProcessIncomingRoutingMessage(RoutingMessage *m) {
    cerr << *this << " got a routing message: " << *m << " (ignored)" << endl;

	// If this isn't the node's own message coming back and it's not duplicate info, perform Dijkstra's and forward message
	if(!AlreadyReceived(m)) {
		routing_table.topo[m->message_from] = m->links_to_neighbors;		
		Dijkstra();
		SendToNeighbors(m);
	}

}

void LinkState::TimeOut() {
    cerr << *this << " got a timeout: (ignored)" << endl;
}

Node* LinkState::GetNextHop(Node *destination) { 
	cerr<<"GetNextHop not yet implemented"<<endl;    
	
	// Iterate through all neighbors to find the node matching destination, grab bw and lat
	Node *copy_node; 
	int looking_for = routing_table.next_hop[destination->GetNumber()];	
	deque<Node*> neighbors = *Node::GetNeighbors();
	deque<Node*>::iterator neighbors_itr;

	for(neighbors_itr = neighbors.begin(); neighbors_itr != neighbors.end(); neighbors_itr++) {
		if(unsigned(looking_for) == (*neighbors_itr)->GetNumber()) {
			double latency_copy = (*neighbors_itr)->GetLatency();
			double bw_copy = (*neighbors_itr)->GetBW();	
			copy_node = new Node(looking_for, context, bw_copy, latency_copy);
		}
	}

	return copy_node;
}

Table* LinkState::GetRoutingTable() {
	cerr << "GetRoutingTable() called" << endl;
    Table* copy_table = new Table(routing_table);
	return copy_table;
}

ostream & LinkState::Print(ostream &os) const { 
    Node::Print(os);
    return os;
}


//--------------------------------//
// Node used in Dijkstra's Method //
//--------------------------------//
class DNode{
public:
   int node_num;
   int distance;
};

struct Comp{
   bool operator()(const DNode& a, const DNode& b){
       return a.distance>b.distance;
   }
};


/*
 Perform Dijkstra's Algorithm on a graph representation
*/
void LinkState::Dijkstra() {

	// Break if the node doesn't know the whole network
	if(!KnowsNetwork()) {
		cerr<<"The whole network isn't known yet"<<endl;
		return;
	}

	priority_queue< DNode, vector<DNode>, Comp> pq;			// priority queue holding unvisited nodes
	priority_queue< DNode, vector<DNode>, Comp> temp_pq;	// hack, used to iterating over pq
	map<int, int> prev_node;								// maps previous node along shortest paths
	map<int, bool> visited;									// keeps track of visited nodes
	
	// Loop through topology to build initial data structures (pq, prev_node, visited)
	for(map< int, deque<Link*> *>::iterator topo_itr = routing_table.topo.begin(); topo_itr != routing_table.topo.end(); topo_itr++) {
		
		// Create node, check to see if it's source or a destination, add it to visited as false, push onto queue, set prev_node[num] = -1
		DNode n;
		n.node_num = topo_itr->first;
		n.distance = POS_INF;
		if(n.node_num == int(number)) {
			n.distance = 0;
		}
		
		pq.push(n);
		visited[n.node_num] = false;
		prev_node[n.node_num] = -1;
	}

	// Perform Dijkstra's. It continues until the priority queue is empty
	while(!pq.empty()) {
		
		DNode current_node = pq.top();
		deque<Link*> *neighbors = routing_table.topo[current_node.node_num];
			
		// Loop through neighbors of current node, need to perform popping magic to find the matching DNode and update it
		for(unsigned int i = 0; i < neighbors->size(); i++) {
			Link *current_link = neighbors->at(i);
			int curr_neighbor_num = current_link->GetDest();
			int current_distance = current_node.distance;

			// If looking at an already visited neighbor, ignore
			if(visited[curr_neighbor_num]) {
				continue;
			}
			
			// Magic->
			//	1) Move through all elements of pq
			//  2) If the node is the neighbor, compare its distance to the current path distance + link cost, update if necessary (distance and prev), push onto temp, break
			//  3) If the node currently being viewed isn't the neighbor, push onto temp pq
			//  4) Push contents of temp back onto pq
			DNode temp_node;
			int size = pq.size();			
			for(int i = 0; i < size; i++){														// (1)
				temp_node = pq.top();
				if(temp_node.node_num == curr_neighbor_num) {									// (2)					

					if(current_distance + current_link->GetLatency() < temp_node.distance) {
						temp_node.distance = current_distance + current_link->GetLatency();	
						prev_node[temp_node.node_num] = current_node.node_num;					
					}
					
					temp_pq.push(temp_node);
					pq.pop();
					break;
				}
				
				temp_pq.push(temp_node);														// (3)
				pq.pop();
			}

			size = temp_pq.size();
			for(int i = 0; i < size; i++) {														// (4)
				temp_node = temp_pq.top();
				temp_pq.pop();
				pq.push(temp_node);
			}
		}

		// Pop the current node off of the queue and set visited to true
		pq.pop();
		visited[current_node.node_num] = true;
	}

	// Now work backwards with prev_node values to fill next_hop (forwarding table)
	for(map<int, deque<Link*> *>::iterator topo_itr = routing_table.topo.begin(); topo_itr != routing_table.topo.end(); topo_itr++) {
	
		// We don't need to get source to source
		if(topo_itr->first == int(number)) {
			continue;
		}		

		int destination = topo_itr->first;
		int curr_node = topo_itr->first;
		int prev = -1;
			
		// Backtrack through network by plugging in values to the prev_node map
		while(curr_node != int(number)) {
			prev = curr_node;			
			curr_node = prev_node[curr_node];
			
			// If backtracking is complete
			if(curr_node == int(number)) {
				routing_table.next_hop[destination] = prev;
			}
		}
	}
}

//---------------------------------


/*
 Checks to see if the node has complete information about the known network
*/
bool LinkState::KnowsNetwork() {
	
	// Loop through every key in the topo map, then check to make sure every neighbor is represented in the map
	for(map<int, deque<Link*> *>::iterator topo_itr = routing_table.topo.begin(); topo_itr != routing_table.topo.end(); topo_itr++) {
		
		deque<Link*> *links = topo_itr->second;
		for(unsigned int i = 0; i < links->size(); i++) {
			if(routing_table.topo.count(links->at(i)->GetSrc()) == 0 || routing_table.topo.count(links->at(i)->GetDest()) == 0) { // Potentially check for null values
				return false;
			}
		}
	}


	return true;
}


/*
 Compare the contents of two Link deques to see if the info had already been received
*/
bool LinkState::AlreadyReceived(RoutingMessage *m) {
	// If the message is from itself, return true
	if(int(number) == m->message_from) {
		return true;
	}

	// Otherwise, check message size (if), potentially do a comparison of all links (else)	
	else {
		if(routing_table.topo.count(m->message_from) == 0) {
			return false;
		}

		deque<Link*> *current_info = routing_table.topo[m->message_from];


		// If there's no entry for this node yet just add it
		
		// Otherwise, compare the sizes of the deques, if they're different you know it's new info
		if(current_info->size() != m->links_to_neighbors->size()) {
			return false;
		}
		// If they're the same size, loop through and do a link by link comparison between old info and new
		else {
			for(unsigned int i = 0; i < current_info->size(); i++) {
				if(current_info->at(i)->GetSrc() != m->links_to_neighbors->at(i)->GetSrc() ||
				   current_info->at(i)->GetDest() != m->links_to_neighbors->at(i)->GetDest() ||
				   current_info->at(i)->GetLatency() != m->links_to_neighbors->at(i)->GetLatency()) {
					return false;
				}
			}

			return true;
		}
	}

}


