#include "circuit.h"
#include "debug.h"
#include "nodedef.h"
#include "mapnode.h"
#include "scripting_game.h"

#include <map>
#include <iomanip>

#define PP(x) ((x).X)<<" "<<((x).Y)<<" "<<((x).Z)<<" "

void printStateFunc(const unsigned char* func)
{
	dstream << "Func: " << std::endl;
	for(int i = 0; i < 64; ++i)
	{
		dstream << std::hex << i << " " << static_cast<unsigned int>(func[i]) << std::endl;
	}
}

Circuit::Circuit(GameScripting* script) : m_script(script), m_min_update_delay(0.1f),
                                          m_since_last_update(0.0f)
{
}

void Circuit::addElement(Map& map, INodeDefManager* ndef, v3s16 pos, const unsigned char* func)
{
	std::vector <std::pair <CircuitElement*, int > > connected;
	MapNode node = map.getNode(pos);
	
	const unsigned char* node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR)
	{
		node_func = circuit_element_states.addState(func, node.param2);
	} else {
		node_func = circuit_element_states.addState(func);
	}
	
	std::list <CircuitElement>::iterator current_element_iterator =
	 elements.insert(elements.begin(), CircuitElement(pos, node, node_func));
	CircuitElementContainer tmp_container;

	
	node.circuit_element_iterator = current_element_iterator;
	map.addNodeWithEvent(pos, node);
	
	for(int i = 0; i < 6; ++i)
	{	
		connected.clear();
		CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(i));
		
		tmp_container.shift = i;
		for(std::vector <std::pair <CircuitElement*, int> >::iterator j = connected.begin();
		    j != connected.end(); ++j)
		{
			if(j -> first -> m_faces[j -> second].begin() -> element != &(*current_element_iterator))
			{
				CircuitElement* tmp_element = j -> first;
				std::list <CircuitElementContainer>::iterator first_iterator =
				         current_element_iterator -> m_faces[i].insert(
				         current_element_iterator -> m_faces[i].begin(), tmp_container);
				std::list <CircuitElementContainer>::iterator second_iterator =
				         j -> first -> m_faces[j -> second].insert(j -> first -> m_faces[j -> second].begin(), tmp_container);
				
				first_iterator -> shift = j -> second;
				first_iterator -> element = tmp_element;
				first_iterator -> list_iterator = second_iterator;
				first_iterator -> list_pointer = j -> first -> m_faces + j -> second;
				second_iterator -> shift = i;
				second_iterator -> element = &(*current_element_iterator);
				second_iterator -> list_iterator = first_iterator;
				second_iterator -> list_pointer = current_element_iterator -> m_faces + i;
			}
		}
	}
}

void Circuit::removeElement(std::list <CircuitElement>::iterator iter)
{
	elements.erase(iter);
}

void Circuit::addWire(Map& map, INodeDefManager* ndef, v3s16 pos)
{
	// This is used for converting elements of current_face_connected to their ids in all_connected.
	std::map <std::pair <CircuitElement*, int > , int> pair_to_id_converter;
	std::vector <std::vector <bool> > is_joint_created;
	std::vector <std::pair <CircuitElement*, int > > all_connected;
	std::vector <std::pair <CircuitElement*, int > > current_face_connected;
	CircuitElement::findConnected(all_connected, map, ndef, pos);
	is_joint_created.resize(all_connected.size());
	for(unsigned int i = 0; i < all_connected.size(); ++i)
	{
		pair_to_id_converter[all_connected[i]] = i;
		is_joint_created[i].resize(all_connected.size());
	}
	
	for(unsigned int i = 0; i < 6; ++i)
	{
		current_face_connected.clear();
		CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos, SHIFT_TO_FACE(i));
		
		for(unsigned int j = 0; j < all_connected.size(); ++j)
		{
			bool exist_in_current = false;
			for(unsigned int k = 0; k < current_face_connected.size(); ++k)
			{
				if(all_connected[j] == current_face_connected[k])
				{
					exist_in_current = true;
					break;
				}
			}
			
			if(!exist_in_current)
			{
				CircuitElementContainer tmp_container;
				// Add edges to the graph
				for(unsigned int k = 0; k < current_face_connected.size(); ++k)
				{
					if(!is_joint_created[pair_to_id_converter[all_connected[j]]]
					    [pair_to_id_converter[current_face_connected[k]]])
					{
						// Create joint
						tmp_container.shift = current_face_connected[k].second;
						std::list <CircuitElementContainer>::iterator first_iterator =
						           all_connected[j].first -> m_faces[all_connected[j].second].insert(
							    all_connected[j].first -> m_faces[all_connected[j].second].begin(), tmp_container);
						
						tmp_container.shift = all_connected[j].second;
						std::list <CircuitElementContainer>::iterator second_iterator =
						           current_face_connected[k].first -> m_faces[current_face_connected[k].second].
						            insert(current_face_connected[k].first -> m_faces[current_face_connected[k].second].
						             begin(), tmp_container);
					
						first_iterator -> element = current_face_connected[k].first;
						first_iterator -> list_iterator = second_iterator;
						first_iterator -> list_pointer = current_face_connected[k].first
										-> m_faces + current_face_connected[k].second;
						second_iterator -> element = all_connected[j].first;
						second_iterator -> list_iterator = first_iterator;
						second_iterator -> list_pointer = all_connected[j].first -> m_faces + i;
						
						// Set joint to created
						is_joint_created[pair_to_id_converter[all_connected[j]]]
						 [pair_to_id_converter[current_face_connected[k]]] = true;
						is_joint_created[pair_to_id_converter[current_face_connected[k]]]
						 [pair_to_id_converter[all_connected[j]]] = true;
					}
				}
			}
		}
	}
}

void Circuit::removeWire(Map& map, INodeDefManager* ndef, v3s16 pos)
{
	// This is used for converting elements of current_face_connected to their ids in all_connected.
	std::map <std::pair <CircuitElement*, int > , int> pair_to_id_converter;
	std::vector <std::vector <bool> > is_joint_created;
	std::vector <std::pair <CircuitElement*, int > > all_connected;
	std::vector <std::pair <CircuitElement*, int > > current_face_connected;
	CircuitElement::findConnected(all_connected, map, ndef, pos);
	is_joint_created.resize(all_connected.size());
	for(unsigned int i = 0; i < all_connected.size(); ++i)
	{
		pair_to_id_converter[all_connected[i]] = i;
		is_joint_created[i].resize(all_connected.size());
	}
	
	for(unsigned int i = 0; i < 6; ++i)
	{
		current_face_connected.clear();
		CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos, SHIFT_TO_FACE(i));
		for(unsigned int j = 0; j < all_connected.size(); ++j)
		{
			bool exist_in_current = false;
			for(unsigned int k = 0; k < current_face_connected.size(); ++k)
			{	
				if(all_connected[j] == current_face_connected[k])
				{
					exist_in_current = true;
					break;
				}
			}
			if(!exist_in_current)
			{
				// Remove edges from graph
				for(unsigned int k = 0; k < current_face_connected.size(); ++k)
				{
					std::list <CircuitElementContainer>* face =
					 current_face_connected[k].first -> m_faces + current_face_connected[k].second;
					for(std::list <CircuitElementContainer>::iterator l = face -> begin();
					    l != face -> end();)
					{
						if(l -> element == all_connected[j].first)
						{
							/*
							 * Save and increment iterator because erase invalidates
							 * only iterators to the erased elements.
							 */
							std::list <CircuitElementContainer>::iterator tmp_l = l;
							++l;
							tmp_l -> list_pointer -> erase(tmp_l -> list_iterator);
							face -> erase(tmp_l);
						} else {
							++l;
						}
					}
				}
			}
		}
	}
}

void Circuit::update(float dtime, Map& map,  INodeDefManager* ndef)
{
	if(m_since_last_update > m_min_update_delay)
	{
		m_since_last_update -= m_min_update_delay;
		for(std::list <CircuitElement>::iterator i = elements.begin();
		    i != elements.end(); ++i)
		{
			i -> update();
		}
		for(std::list <CircuitElement>::iterator i = elements.begin();
		i != elements.end(); ++i)
		{
			i -> updateState(m_script, map, ndef);
		}
#ifdef CIRCUIT_DEBUG
		dstream << "Dt: " << dtime << " " << m_since_last_update << std::endl;
		for(std::list <CircuitElement>::iterator i = elements.begin();
		    i != elements.end(); ++i)
		{
			dstream << PP(i -> m_pos) << " " << &(*i) << ": ";
			for(int j = 0; j < 6; ++j)
			{
				dstream << " (";
				std::list <CircuitElementContainer> tmp_face = i -> getFace(j);
				for(std::list <CircuitElementContainer>::iterator k = tmp_face.begin();
				    k != tmp_face.end(); ++k)
				{
					dstream << k -> element << ", ";
				}
				dstream << "), ";
			}
			dstream << std::endl;
		}
#endif
	} else {
		m_since_last_update += dtime;
	}
}


void Circuit::updateElement(MapNode& node, INodeDefManager* ndef, const unsigned char* func)
{
	const unsigned char* node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR)
	{
		node_func = circuit_element_states.addState(func, node.param2);
	} else {
		node_func = circuit_element_states.addState(func);
	}
	node.circuit_element_iterator -> m_func = node_func;
	node.circuit_element_iterator -> m_node = node;
}

void Circuit::pushElementToQueue(v3s16 pos)
{
	elements_queue.push_back(pos);
}
void Circuit::processElementsQueue(Map& map, INodeDefManager* ndef)
{
	if(elements_queue.size() > 0)
	{
		std::map <v3s16, int> pos_to_id_converter;
		std::map <v3s16, int>::iterator current_element_iterator;
		std::vector <std::pair <CircuitElement*, int > > connected;
		std::vector <std::list<CircuitElement>::iterator> elements_queue_iterators(elements_queue.size());
		v3s16 pos;
		MapNode node;
		CircuitElementContainer tmp_container;
		std::vector <bool> processed(elements_queue.size(), false);
		const unsigned char* node_func;
		
		for(unsigned int i = 0; i < elements_queue.size(); ++i)
		{
			pos_to_id_converter[elements_queue[i]] = i;
			node = map.getNode(elements_queue[i]);
			if(ndef->get(node).param_type_2 == CPT2_FACEDIR)
			{
				node_func = circuit_element_states.addState(ndef->get(node).circuit_element_states, node.param2);
			} else {
				node_func = circuit_element_states.addState(ndef->get(node).circuit_element_states);
			}
			node.circuit_element_iterator = elements.insert(elements.begin(), CircuitElement(elements_queue[i], node, node_func));
			elements_queue_iterators[i] = node.circuit_element_iterator;
			map.setNode(elements_queue[i], node);
		}
		
		for(unsigned int i = 0; i < elements_queue.size(); ++i)
		{
			pos = elements_queue[i];
			for(int j = 0; j < 6; ++j)
			{
				connected.clear();
				CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(j));
				for(unsigned int k = 0; k < connected.size(); ++k)
				{
					current_element_iterator = pos_to_id_converter.find(connected[k].first -> m_pos);
					if(!((current_element_iterator != pos_to_id_converter.end()) && processed[current_element_iterator -> second]))
					{
						// Create joint
						tmp_container.shift = connected[k].second;
						std::list <CircuitElementContainer>::iterator first_iterator =
						           elements_queue_iterators[i] -> m_faces[j].insert(
							    elements_queue_iterators[i] -> m_faces[j].begin(), tmp_container);
						
						tmp_container.shift = j;
						std::list <CircuitElementContainer>::iterator second_iterator =
						           connected[k].first -> m_faces[connected[k].second].
						            insert(connected[k].first -> m_faces[connected[k].second].
						             begin(), tmp_container);
					
						first_iterator -> element = connected[k].first;
						first_iterator -> list_iterator = second_iterator;
						first_iterator -> list_pointer = connected[k].first -> m_faces + connected[k].second;
						second_iterator -> element = &(*elements_queue_iterators[i]);
						second_iterator -> list_iterator = first_iterator;
						second_iterator -> list_pointer = elements_queue_iterators[i] -> m_faces + i;
					}
				}
			}
			processed[i] = true;
		}
		
		elements_queue.clear();
	}
}
