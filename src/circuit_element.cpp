#include "circuit_element.h"
#include "circuit_element_states.h"
#include "nodedef.h"
#include "mapnode.h"
#include "map.h"
#include "scripting_game.h"

#include <set>
#include <queue>
#include <iomanip>
#include <cassert>
#include <map>

#define PP(x) ((x).X)<<" "<<((x).Y)<<" "<<((x).Z)<<" "

unsigned char CircuitElement::face_to_shift[] = {
	0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
	4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5
};

unsigned char CircuitElement::opposite_shift[] = {
	3, 4, 5, 0, 1, 2
};

FaceId CircuitElement::shift_to_face[] = {
	FACE_BOTTOM, FACE_BACK, FACE_LEFT,
	FACE_TOP, FACE_FRONT, FACE_RIGHT
};

FaceId CircuitElement::facedir_to_face[] = {
	FACE_FRONT, FACE_LEFT, FACE_BACK,
	FACE_RIGHT, FACE_BOTTOM, FACE_TOP
};

CircuitElement::CircuitElement(v3s16 pos, MapNode& node, const unsigned char* func) :
	m_pos(pos), m_node(node), m_func(func),
	m_current_input_state(0), m_next_input_state(0),
	m_current_output_state(0), m_next_output_state(0)
{
	m_current_output_state = m_func[m_current_input_state];
	for(int i = 0; i < 6; ++i)
	{
		m_faces[i].element = this;
	}
#ifdef CIRCUIT_DEBUG
	dstream << (OPPOSITE_FACE(FACE_TOP) == FACE_BOTTOM);
	dstream << (OPPOSITE_FACE(FACE_BACK) == FACE_FRONT);
	dstream << (OPPOSITE_FACE(FACE_LEFT) == FACE_RIGHT);
	dstream << (OPPOSITE_FACE(FACE_BOTTOM) == FACE_TOP);
	dstream << (OPPOSITE_FACE(FACE_FRONT) == FACE_BACK);
	dstream << (OPPOSITE_FACE(FACE_RIGHT) == FACE_LEFT);
	dstream << std::endl;
#endif
}

CircuitElement::CircuitElement(const CircuitElement& element)
{
	m_pos = element.m_pos;
	m_node = element.m_node;
	m_func = element.m_func;
	m_current_input_state = element.m_current_input_state;
	m_current_output_state = element.m_current_output_state;
	m_next_input_state = element.m_next_input_state;
	m_next_output_state = element.m_next_output_state;
	for(int i = 0; i < 6; ++i)
	{
		m_faces[i] = element.m_faces[i];
		m_faces[i].element = this;
	}
}

CircuitElement::~CircuitElement()
{
	for(int i = 0; i < 6; ++i) {
		for(CircuitElementList::iterator j = m_faces[i].begin();
		    j != m_faces[i].end(); ++j) {
			j -> list_pointer -> erase(j -> list_iterator);
		}
	}
}

void CircuitElement::update()
{
	if(m_current_output_state) {
		for(int i = 0; i < 6; ++i) {
			if(m_current_output_state & SHIFT_TO_FACE(i)) {
				for(CircuitElementList::iterator j = m_faces[i].begin();
				    j != m_faces[i].end(); ++j) {
					j -> list_pointer -> element -> m_next_input_state |= static_cast<bool>(m_current_output_state & SHIFT_TO_FACE(i)) << (j -> shift);
				}
			}
		}
	}
}

void CircuitElement::updateState(GameScripting* m_script, Map& map, INodeDefManager* ndef)
{
// 	dstream << "Pos: " << std::dec<< m_pos.X << " " << m_pos.Y << " " << m_pos.Z
// 	    << " State: " << std::hex << static_cast<unsigned int>(m_current_input_state) << " "
// 	    << std::hex << static_cast<unsigned int>(m_current_output_state)  << std::endl;
	m_current_output_state = m_func[m_next_input_state];
	if(m_next_input_state && !m_current_input_state && ndef->get(m_node).has_on_activate) {
		m_script -> node_on_activate(m_pos, m_node);
	} else if(!m_next_input_state && m_current_input_state && ndef->get(m_node).has_on_deactivate) {
		m_script -> node_on_deactivate(m_pos, m_node);
	}
	m_current_input_state = m_next_input_state;
	m_next_input_state = 0;
}


void CircuitElement::findConnected(std::vector <std::pair <CircuitElement*, int > >& connected,
                                   Map& map, INodeDefManager* ndef, v3s16 pos, MapNode current_node,
                                   std::map<v3s16, std::list<CircuitElement>::iterator>& pos_to_iterator)
{
	static v3s16 directions[6] = {v3s16(0, -1, 0),
	                              v3s16(0, 0, 1),
	                              v3s16(-1, 0, 0),
	                              v3s16(0, 1, 0),
	                              v3s16(0, 0, -1),
	                              v3s16(1, 0, 0)};
	std::map <v3s16, unsigned char> used;
	used[pos] = 0;
	// first - position
	// second - faces that can send signal
	std::queue <std::pair <v3s16, unsigned char> > q;
	v3s16 current_pos;
	v3s16 next_pos;
	unsigned char acceptable_faces;
	std::map <v3s16, unsigned char>::iterator current_used_iterator;
	std::map <v3s16, unsigned char>::iterator tmp_used_iterator;
	ContentFeatures node_features;
	ContentFeatures current_node_features;
	MapNode next_node;

	q.push(std::make_pair(pos, 0x3F));
	while(!q.empty()) {
		current_pos = q.front().first;
		acceptable_faces = q.front().second;
		q.pop();

		for(int i = 0; i < 6; ++i) {
			if(acceptable_faces & SHIFT_TO_FACE(i)) {
				next_pos.X = current_pos.X + directions[i].X;
				next_pos.Y = current_pos.Y + directions[i].Y;
				next_pos.Z = current_pos.Z + directions[i].Z;
				used[current_pos] |= SHIFT_TO_FACE(i);
				next_node = map.getNodeNoEx(next_pos);
				node_features = ndef->get(next_node);
				if(current_pos == pos) {
					current_node_features = ndef->get(current_node);
				} else {
					current_node = map.getNodeNoEx(current_pos);
					current_node_features = ndef->get(current_node);
				}
				current_used_iterator = used.find(next_pos);
				if((current_used_iterator == used.end()) || !(current_used_iterator->second & SHIFT_TO_FACE(OPPOSITE_SHIFT(i)))) {
					if(current_node_features.is_connector || node_features.is_connector
					   || (node_features.is_wire && (next_node.getContent() == current_node.getContent()))) {
						if(node_features.param_type_2 == CPT2_FACEDIR) {
							q.push(std::make_pair(next_pos, CircuitElementStates::rotateState(
							        node_features.wire_connections[OPPOSITE_SHIFT(i)], FACEDIR_TO_FACE(next_node.param2))));
						} else {
							q.push(std::make_pair(next_pos, node_features.wire_connections[OPPOSITE_SHIFT(i)]));
						}
						tmp_used_iterator = used.find(next_pos);
						if(tmp_used_iterator != used.end()) {
							tmp_used_iterator -> second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
						} else {
							used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
						}
					}
					
					if(node_features.is_circuit_element) {
						tmp_used_iterator = used.find(next_pos);
						if(tmp_used_iterator != used.end()) {
							tmp_used_iterator -> second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
						} else {
							used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
						}
						connected.push_back(std::make_pair(&(*pos_to_iterator[next_pos]),
										OPPOSITE_SHIFT(i)));
					}
				}
			}
		}
	}

}

void CircuitElement::findConnectedWithFace(std::vector <std::pair <CircuitElement*, int > >& connected,
        Map& map, INodeDefManager* ndef, v3s16 pos, FaceId face,
        std::map<v3s16, std::list<CircuitElement>::iterator>& pos_to_iterator)
{
	static v3s16 directions[6] = {v3s16(0, -1, 0),
	                              v3s16(0, 0, 1),
	                              v3s16(-1, 0, 0),
	                              v3s16(0, 1, 0),
	                              v3s16(0, 0, -1),
	                              v3s16(1, 0, 0),
	                             };
	std::map <v3s16, unsigned char> used;
	used[pos] = face;
	std::queue <std::pair <v3s16, unsigned char> > q;
	v3s16 current_pos;
	v3s16 next_pos;
	unsigned char acceptable_faces;
	std::map <v3s16, unsigned char>::iterator current_used_iterator;
	std::map <v3s16, unsigned char>::iterator tmp_used_iterator;
	ContentFeatures node_features, current_node_features;
	MapNode next_node, current_node;

	int face_id = FACE_TO_SHIFT(face);
	current_pos.X = pos.X + directions[face_id].X;
	current_pos.Y = pos.Y + directions[face_id].Y;
	current_pos.Z = pos.Z + directions[face_id].Z;
	next_node = map.getNodeNoEx(current_pos);
	node_features = ndef->get(next_node);
	q.push(std::make_pair(current_pos, node_features.wire_connections[FACE_TO_SHIFT(OPPOSITE_FACE(face))]));

	if(ndef->get(map.getNodeNoEx(current_pos)).is_wire || ndef->get(map.getNodeNoEx(current_pos)).is_connector) {
		while(!q.empty()) {
			current_pos = q.front().first;
			acceptable_faces = q.front().second;
			q.pop();

			for(int i = 0; i < 6; ++i) {
				if(acceptable_faces & (SHIFT_TO_FACE(i))) {
					next_pos.X = current_pos.X + directions[i].X;
					next_pos.Y = current_pos.Y + directions[i].Y;
					next_pos.Z = current_pos.Z + directions[i].Z;
					used[current_pos] |= SHIFT_TO_FACE(i);
					next_node = map.getNodeNoEx(next_pos);
					node_features = ndef->get(next_node);
					current_node = map.getNodeNoEx(current_pos);
					current_node_features = ndef->get(current_node);
					current_used_iterator = used.find(next_pos);
					if((current_used_iterator == used.end()) || !(current_used_iterator->second & SHIFT_TO_FACE(OPPOSITE_SHIFT(i)))) {
						if(current_node_features.is_connector || node_features.is_connector
						   || (node_features.is_wire && (next_node.getContent() == current_node.getContent()))) {
							if(node_features.param_type_2 == CPT2_FACEDIR) {
								q.push(std::make_pair(next_pos, CircuitElementStates::rotateState(
								       node_features.wire_connections[OPPOSITE_SHIFT(i)], FACEDIR_TO_FACE(next_node.param2))));
							} else {
								q.push(std::make_pair(next_pos, node_features.wire_connections[OPPOSITE_SHIFT(i)]));
							}
							tmp_used_iterator = used.find(next_pos);
							if(tmp_used_iterator != used.end()) {
								tmp_used_iterator -> second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							} else {
								used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							}
						}
						
						if(node_features.is_circuit_element) {
							tmp_used_iterator = used.find(next_pos);
							if(tmp_used_iterator != used.end()) {
								tmp_used_iterator -> second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							} else {
								used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							}
							connected.push_back(std::make_pair(&(*pos_to_iterator[next_pos]),
							                                   OPPOSITE_SHIFT(i)));
						}
					}
				}
			}
		}
	} else if(ndef->get(map.getNodeNoEx(current_pos)).is_circuit_element) {
		connected.push_back(std::make_pair(&(*pos_to_iterator[current_pos]),
		                                   FACE_TO_SHIFT(OPPOSITE_FACE(face))));
	}
}

CircuitElementList& CircuitElement::getFace(int id)
{
	return m_faces[id];
}