#include "circuit_element.h"
#include "nodedef.h"
#include "mapnode.h"
#include "map.h"
#include "scripting_game.h"

#include <set>
#include <queue>
#include <iomanip>
#include <cassert>
#include <map>

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

CircuitElement::~CircuitElement()
{
	for(int i = 0; i < 6; ++i) {
		for(std::list <CircuitElementContainer>::iterator j = m_faces[i].begin();
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
				for(std::list <CircuitElementContainer>::iterator j = m_faces[i].begin();
				    j != m_faces[i].end(); ++j) {
					j -> element -> m_next_input_state |= static_cast<bool>(m_current_output_state & SHIFT_TO_FACE(i)) << (j -> shift);
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
	m_current_output_state = m_func[m_next_input_state]/* & (~m_current_input_state)*/;
	if(m_next_input_state && !m_current_input_state && ndef->get(m_node).has_on_activate) {
		m_script -> node_on_activate(m_pos, m_node);
	} else if(!m_next_input_state && m_current_input_state && ndef->get(m_node).has_on_deactivate) {
		m_script -> node_on_deactivate(m_pos, m_node);
	}
	m_current_input_state = m_next_input_state;
	m_next_input_state = 0;
}


void CircuitElement::findConnected(std::vector <std::pair <CircuitElement*, int > >& connected,
                                   Map& map, INodeDefManager* ndef, v3s16 pos, MapNode& current_node,
                                   std::map<v3s16, std::list<CircuitElement>::iterator>& pos_to_iterator)
{
	static v3s16 directions[6] = {v3s16(0, -1, 0),
	                              v3s16(0, 0, 1),
	                              v3s16(-1, 0, 0),
	                              v3s16(0, 1, 0),
	                              v3s16(0, 0, -1),
	                              v3s16(1, 0, 0),
	                             };
	content_t node_type = current_node.getContent();
	std::set <v3s16> used;
	used.insert(pos);
	std::queue <v3s16> q;
	v3s16 current_pos;
	v3s16 next_pos;
	ContentFeatures node_features;
	MapNode node;

	q.push(pos);
	while(!q.empty()) {
		current_pos = q.front();
		q.pop();

		for(int i = 0; i < 6; ++i) {
			next_pos.X = current_pos.X + directions[i].X;
			next_pos.Y = current_pos.Y + directions[i].Y;
			next_pos.Z = current_pos.Z + directions[i].Z;
			node = map.getNodeNoEx(next_pos);
			node_features = ndef->get(node);
			if(used.find(next_pos) == used.end()) {
				if(node_features.is_wire && (node.getContent() == node_type)) {
// 					dstream << "Wire at: " << next_pos.X << " " << next_pos.Y << " " << next_pos.Z << std::endl;
					used.insert(next_pos);
					q.push(next_pos);
				}
			}
			if(node_features.is_circuit_element) {
				connected.push_back(std::make_pair(&(*pos_to_iterator[next_pos]),
				                                   OPPOSITE_SHIFT(i)));
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
	std::set <v3s16> used;
	used.insert(pos);
	std::queue <v3s16> q;
	v3s16 current_pos;
	v3s16 next_pos;
	ContentFeatures node_features;
	MapNode node;

	int face_id = FACE_TO_SHIFT(face);
	current_pos.X = pos.X + directions[face_id].X;
	current_pos.Y = pos.Y + directions[face_id].Y;
	current_pos.Z = pos.Z + directions[face_id].Z;
	q.push(current_pos);
	content_t node_type = map.getNodeNoEx(current_pos).getContent();

	if(ndef->get(map.getNodeNoEx(current_pos)).is_wire) {
		while(!q.empty()) {
			current_pos = q.front();
			q.pop();

			for(int i = 0; i < 6; ++i) {
				next_pos.X = current_pos.X + directions[i].X;
				next_pos.Y = current_pos.Y + directions[i].Y;
				next_pos.Z = current_pos.Z + directions[i].Z;
				node = map.getNodeNoEx(next_pos);
				node_features = ndef->get(node);
				if(used.find(next_pos) == used.end()) {
					if(node_features.is_wire && (node.getContent() == node_type)) {
						used.insert(next_pos);
						q.push(next_pos);
					}
				}
				if(node_features.is_circuit_element && !((next_pos == pos) && (OPPOSITE_FACE(face) == SHIFT_TO_FACE(i)))) {
					connected.push_back(std::make_pair(&(*pos_to_iterator[next_pos]),
					                                   OPPOSITE_SHIFT(i)));
				}
			}
		}
	} else if(ndef->get(map.getNodeNoEx(current_pos)).is_circuit_element) {
		connected.push_back(std::make_pair(&(*pos_to_iterator[current_pos]),
		                                   FACE_TO_SHIFT(OPPOSITE_FACE(face))));
	}
}

std::list <CircuitElementContainer>& CircuitElement::getFace(int id)
{
	return m_faces[id];
}