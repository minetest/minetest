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

CircuitElement::CircuitElement(v3s16 pos, const unsigned char* func, unsigned long func_id,
                               unsigned long element_id, unsigned int delay) :
	m_pos(pos), m_func(func), m_func_id(func_id), m_current_input_state(0), m_next_input_state(0),
	m_current_output_state(0), m_next_output_state(0)
{
	m_current_output_state = m_func[m_current_input_state];
	m_element_id = element_id;
	for(int i = 0; i < 6; ++i)
	{
		m_faces[i].is_connected = false;
	}
	setDelay(delay);
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
	m_element_id = element.m_element_id;
	m_func = element.m_func;
	m_func_id = element.m_func_id;
	m_current_input_state = element.m_current_input_state;
	m_current_output_state = element.m_current_output_state;
	m_next_input_state = element.m_next_input_state;
	m_next_output_state = element.m_next_output_state;
	for(int i = 0; i < 6; ++i) {
		m_faces[i].list_iterator = element.m_faces[i].list_iterator;
		m_faces[i].list_pointer  = element.m_faces[i].list_pointer;
		m_faces[i].is_connected  = element.m_faces[i].is_connected;
	}
	setDelay(element.m_states_queue.size());
}

CircuitElement::CircuitElement(unsigned long element_id) : m_pos(v3s16(0, 0, 0)),
                                   m_func(0), m_func_id(0), m_current_input_state(0), m_next_input_state(0),
                                   m_current_output_state(0), m_next_output_state(0)
{
	m_element_id = element_id;
	for(int i = 0; i < 6; ++i) {
		m_faces[i].is_connected = false;
	}
}

CircuitElement::~CircuitElement()
{
	for(int i = 0; i < 6; ++i) {
		if(m_faces[i].is_connected) {
			m_faces[i].list_pointer->erase(m_faces[i].list_iterator);
		}
	}
}

void CircuitElement::update()
{
	if(m_current_output_state) {
		for(int i = 0; i < 6; ++i) {
			if(m_faces[i].is_connected) {
				m_faces[i].list_pointer->addState(static_cast<bool>(m_current_output_state & SHIFT_TO_FACE(i)));
			}
		}
	}
}

void CircuitElement::updateState(GameScripting* m_script, Map& map, INodeDefManager* ndef)
{
	MapNode node = map.getNodeNoEx(m_pos);
	// Update delay (may be not synchronized)
	unsigned long delay = ndef->get(node).circuit_element_delay;
	if(delay != m_states_queue.size()) {
		setDelay(delay);
	}

	m_states_queue.push_back(m_next_input_state);
	m_next_input_state = m_states_queue.front();
	m_states_queue.pop_front();
	m_current_output_state = m_func[m_next_input_state];
	if(m_next_input_state && !m_current_input_state && ndef->get(node).has_on_activate) {
		m_script->node_on_activate(m_pos, node);
	}
	if(!m_next_input_state && m_current_input_state && ndef->get(node).has_on_deactivate) {
		m_script->node_on_deactivate(m_pos, node);
	}
	m_current_input_state = m_next_input_state;
	m_next_input_state = 0;
}

void CircuitElement::serialize(std::ostream& out) const
{
	out.write(reinterpret_cast<const char*>(&m_pos), sizeof(m_pos));
	out.write(reinterpret_cast<const char*>(&m_func_id), sizeof(m_func_id));
	for(int i = 0; i < 6; ++i) {
		unsigned long tmp = 0ul;
		if(m_faces[i].is_connected) {
			tmp = m_faces[i].list_pointer->getId();
		}
		out.write(reinterpret_cast<const char*>(&tmp), sizeof(tmp));
	}
}

void CircuitElement::serializeState(std::ostream& out) const
{
	out.write(reinterpret_cast<const char*>(&m_element_id), sizeof(m_element_id));
	out.write(reinterpret_cast<const char*>(&m_current_input_state), sizeof(m_current_input_state));
	unsigned long queue_size = m_states_queue.size();
	out.write(reinterpret_cast<const char*>(&queue_size), sizeof(queue_size));
	for(std::deque <unsigned char>::const_iterator i = m_states_queue.begin(); i != m_states_queue.end(); ++i) {
		out.write(reinterpret_cast<const char*>(&(*i)), sizeof(*i));
	}
}

void CircuitElement::deSerialize(std::istream& in,
                                 std::map <unsigned long, std::list <CircuitElementVirtual>::iterator>& id_to_virtual_pointer)
{
	unsigned long current_element_id;
	in.read(reinterpret_cast<char*>(&m_pos), sizeof(m_pos));
	in.read(reinterpret_cast<char*>(&m_func_id), sizeof(m_func_id));
	for(int i = 0; i < 6; ++i) {
		in.read(reinterpret_cast<char*>(&current_element_id), sizeof(current_element_id));
		if(current_element_id > 0){
			m_faces[i].list_pointer = id_to_virtual_pointer[current_element_id];
			m_faces[i].is_connected = true;
		} else {
			m_faces[i].is_connected = false;
		}
	}
}

void CircuitElement::deSerializeState(std::istream& in)
{
	unsigned long queue_size;
	unsigned char input_state;
	in.read(reinterpret_cast<char*>(&m_current_input_state), sizeof(m_current_input_state));
	in.read(reinterpret_cast<char*>(&queue_size), sizeof(queue_size));
	for(unsigned long i = 0; i < queue_size; ++i) {
		in.read(reinterpret_cast<char*>(&input_state), sizeof(input_state));
		m_states_queue.push_back(input_state);
	}
}

void CircuitElement::getNeighbors(std::vector <std::list <CircuitElementVirtual>::iterator>& neighbors) const
{
	for(int i = 0; i < 6; ++i) {
		if(m_faces[i].is_connected) {
			bool found = false;
			for(unsigned int j = 0; j < neighbors.size(); ++j) {
				if(neighbors[j] == m_faces[i].list_pointer) {
					found = true;
					break;
				}
			}
			if(!found) {
				neighbors.push_back(m_faces[i].list_pointer);
			}
		}
	}
}

void CircuitElement::findConnectedWithFace(std::vector <std::pair <std::list<CircuitElement>::iterator, int > >& connected,
                                           Map& map, INodeDefManager* ndef, v3s16 pos, FaceId face,
                                           std::map<v3s16, std::list<CircuitElement>::iterator>& pos_to_iterator,
                                           bool connected_faces[6])
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
	connected_faces[face_id] = true;
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

					// If start element, mark some of it's faces
					if(next_pos == pos) {
						connected_faces[OPPOSITE_SHIFT(i)] = true;
					}

					if((current_used_iterator == used.end()) ||
					   !(current_used_iterator->second & SHIFT_TO_FACE(OPPOSITE_SHIFT(i)))) {
						if(current_node_features.is_connector || node_features.is_connector
						   || (node_features.is_wire && (next_node.getContent() == current_node.getContent()))) {
							if(node_features.param_type_2 == CPT2_FACEDIR) {
								q.push(std::make_pair(next_pos, CircuitElementStates::rotateState(
									                      node_features.wire_connections[OPPOSITE_SHIFT(i)],
									                      FACEDIR_TO_FACE(next_node.param2))));
							} else {
								q.push(std::make_pair(next_pos, node_features.wire_connections[OPPOSITE_SHIFT(i)]));
							}
							tmp_used_iterator = used.find(next_pos);
							if(tmp_used_iterator != used.end()) {
								tmp_used_iterator->second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							} else {
								used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							}
						}
						
						if(node_features.is_circuit_element) {
							tmp_used_iterator = used.find(next_pos);
							if(tmp_used_iterator != used.end()) {
								tmp_used_iterator->second |= SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							} else {
								used[next_pos] = SHIFT_TO_FACE(OPPOSITE_SHIFT(i));
							}
							connected.push_back(std::make_pair(pos_to_iterator[next_pos],
							                                   OPPOSITE_SHIFT(i)));
						}
					}
				}
			}
		}
	} else if(ndef->get(map.getNodeNoEx(current_pos)).is_circuit_element) {
		connected.push_back(std::make_pair(pos_to_iterator[current_pos],
		                                   FACE_TO_SHIFT(OPPOSITE_FACE(face))));
	}
}

CircuitElementContainer CircuitElement::getFace(int id) const
{
	return m_faces[id];
}

unsigned long CircuitElement::getFuncId() const
{
	return m_func_id;
}

v3s16 CircuitElement::getPos() const
{
	return m_pos;
}

unsigned long CircuitElement::getId() const
{
	return m_element_id;
}

void CircuitElement::connectFace(int id, std::list <CircuitElementVirtualContainer>::iterator it,
                                 std::list <CircuitElementVirtual>::iterator pt)
{
	m_faces[id].list_iterator = it;
	m_faces[id].list_pointer  = pt;
	m_faces[id].is_connected  = true;
}

void CircuitElement::disconnectFace(int id)
{
	m_faces[id].is_connected = false;
}

void CircuitElement::setId(unsigned long id)
{
	m_element_id = id;
}

void CircuitElement::setInputState(unsigned char state)
{
	m_current_input_state = state;
}

void CircuitElement::setFunc(const unsigned char* func, unsigned long func_id)
{
	m_func = func;
	m_func_id = func_id;
	m_current_output_state = m_func[m_current_input_state];
}

void CircuitElement::setDelay(unsigned int delay)
{
	if(m_states_queue.size() >= delay) {
		while(m_states_queue.size() > delay) {
			m_states_queue.pop_front();
		}
	} else {
		while(m_states_queue.size() < delay) {
			m_states_queue.push_back(0);
		}		
	}
}
