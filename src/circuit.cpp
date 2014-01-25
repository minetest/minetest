#include "circuit.h"
#include "circuit_element_list.h"
#include "debug.h"
#include "nodedef.h"
#include "mapnode.h"
#include "scripting_game.h"
#include "map.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"
#include "jthread/jmutexautolock.h"

#include <map>
#include <iomanip>
#include <cassert>
#include <string>
#include <sstream>

#define PP(x) ((x).X)<<" "<<((x).Y)<<" "<<((x).Z)<<" "

const char Circuit::elements_states_file[] = "circuit_elements_states";
const char Circuit::elements_func_file[] = "circuit_elements_func";

Circuit::Circuit(GameScripting* script, std::string savedir) :  circuit_elements_states(64u, false),
        m_script(script), m_min_update_delay(0.1f), m_since_last_update(0.0f), max_id(0ull), m_savedir(savedir)
{
	unsigned long element_id;
	unsigned char element_state;
	int elements_num = 0;
	std::istringstream in(std::ios_base::binary);
	std::string str;
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, savedir + DIR_DELIM + "circuit.db", &m_database);
	assert(status.ok());
	
	std::ifstream input_elements_func((savedir + DIR_DELIM + elements_func_file).c_str(), std::ios_base::binary);
	if(input_elements_func.good())
	{
		circuit_elements_states.deSerialize(input_elements_func);
	}

	// Filling list with empty elements
	leveldb::Iterator* it = m_database -> NewIterator(leveldb::ReadOptions());
	std::map<unsigned long, std::list<CircuitElement>::iterator> id_to_pointer;
	for(it -> SeekToFirst(); it -> Valid(); it -> Next()) {
		id_to_pointer[stoi(it -> key().ToString())] = elements.insert(elements.begin(), CircuitElement());
		++elements_num;
	}
	assert(it -> status().ok());
	
	std::ifstream input_elements_states((savedir + DIR_DELIM + elements_states_file).c_str());
	if(input_elements_states.good())
	{
		for(int i = 0; i < elements_num; ++i)
		{
			input_elements_states.read(reinterpret_cast<char*>(&element_id), sizeof(element_id));
			input_elements_states.read(reinterpret_cast<char*>(&element_state), sizeof(element_state));
			id_to_pointer[element_id] -> m_current_input_state = element_state;
		}
	}
	
	// Loading elements data
	for(it -> SeekToFirst(); it -> Valid(); it -> Next()) {
		in.clear();
		in.str(it -> value().ToString());
		std::string test = it -> value().ToString();
		element_id = stoi(it -> key().ToString());
		if(element_id + 1 > max_id)
		{
			max_id = element_id + 1;
		}
		std::list<CircuitElement>::iterator current_element = id_to_pointer[element_id];
		current_element -> deSerialize(in, id_to_pointer);
		current_element -> setFunc(circuit_elements_states.getFunc(current_element -> m_func_id));
		pos_to_id[current_element -> m_pos] = element_id;
		pos_to_iterator[current_element -> m_pos] = id_to_pointer[element_id];
	}
	assert(it -> status().ok());
	delete it;
}

Circuit::~Circuit()
{
	save();
	delete m_database;
}

void Circuit::addElement(Map& map, INodeDefManager* ndef, v3s16 pos, const unsigned char* func)
{
	JMutexAutoLock lock(m_elements_mutex);
	
	std::vector <std::pair <CircuitElement*, int > > connected;
	MapNode node = map.getNode(pos);
	pos_to_id[pos] = max_id;
	++max_id;

	std::pair<const unsigned char*, unsigned long> node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
		// If block is rotatable, then rotate it's function.
		node_func = circuit_elements_states.addState(func, node.param2);
	} else {
		node_func = circuit_elements_states.addState(func);
	}
	
	saveCircuitElementsStates();

	std::list<CircuitElement>::iterator current_element_iterator =
	    elements.insert(elements.begin(), CircuitElement(pos, node_func.first, node_func.second));
	CircuitElementContainer tmp_container;
	pos_to_iterator[pos] = current_element_iterator;

	// For each face add all other connected faces.
	for(int i = 0; i < 6; ++i) {
		connected.clear();
		CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(i), pos_to_iterator);

		tmp_container.shift = i;
		for(std::vector <std::pair <CircuitElement*, int> >::iterator j = connected.begin();
		    j != connected.end(); ++j) {
			bool exist_in_other = false;
			CircuitElementList* tmp_face = j -> first -> m_faces + j -> second;
			for(CircuitElementList::iterator k = tmp_face -> begin(); k != tmp_face -> end(); ++k) {
				if((tmp_face -> element == &(*current_element_iterator)) && (k -> shift == i)) {
					exist_in_other = true;
					break;
				}
			}
			if(!exist_in_other) {
				CircuitElementList::iterator first_iterator =
				    current_element_iterator -> m_faces[i].insert(
				        current_element_iterator -> m_faces[i].begin(), tmp_container);
				CircuitElementList::iterator second_iterator =
				    j -> first -> m_faces[j -> second].insert(j -> first -> m_faces[j -> second].begin(), tmp_container);

				first_iterator -> shift = j -> second;
				first_iterator -> list_iterator = second_iterator;
				first_iterator -> list_pointer = j -> first -> m_faces + j -> second;
				second_iterator -> shift = i;
				second_iterator -> list_iterator = first_iterator;
				second_iterator -> list_pointer = current_element_iterator -> m_faces + i;
			}
		}
	}
	
	saveElementConnections(current_element_iterator);
}

void Circuit::removeElement(v3s16 pos)
{
	JMutexAutoLock lock(m_elements_mutex);
	
	std::vector <CircuitElement*> neighbors_pointers;
	std::list <CircuitElement>::iterator current_element = pos_to_iterator[pos];
	current_element -> getNeighbors(neighbors_pointers);
	elements.erase(pos_to_iterator[pos]);
	leveldb::Status status = m_database -> Delete(leveldb::WriteOptions(), itos(pos_to_id[pos]));
	assert(status.ok());
	for(unsigned int i = 0; i < neighbors_pointers.size(); ++i)
	{
		std::ostringstream out(std::ios_base::binary);
		neighbors_pointers[i] -> serialize(out, pos_to_id);
		std::string str = out.str();
		status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[neighbors_pointers[i] -> m_pos]), str);
		assert(status.ok());
	}
	pos_to_iterator.erase(pos);
	pos_to_id.erase(pos);
}

void Circuit::addWire(Map& map, INodeDefManager* ndef, v3s16 pos)
{
	JMutexAutoLock lock(m_elements_mutex);
	
	// This is used for converting elements of current_face_connected to their ids in all_connected.
	std::map <std::pair <CircuitElement*, int > , int> pair_to_id_converter;
	std::vector <std::vector <bool> > is_joint_created;
	std::vector <std::pair <CircuitElement*, int > > all_connected;
	std::vector <std::pair <CircuitElement*, int > > current_face_connected;
	std::set <CircuitElement*> changed_elements;
	MapNode node = map.getNode(pos);
	CircuitElement::findConnected(all_connected, map, ndef, pos, node, pos_to_iterator);
	is_joint_created.resize(all_connected.size());
	for(unsigned int i = 0; i < all_connected.size(); ++i) {
		pair_to_id_converter[all_connected[i]] = i;
		is_joint_created[i].resize(all_connected.size());
	}

	// For each face connect faces, that are not yet connected.
	for(unsigned int i = 0; i < 6; ++i) {
		current_face_connected.clear();
		CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos, SHIFT_TO_FACE(i), pos_to_iterator);
		
		all_connected.clear();
		for(unsigned int j = 0; j < 6; ++j) {
			if((ndef -> get(node).wire_connections[i] & (SHIFT_TO_FACE(j))) && (i != j)) {
				CircuitElement::findConnectedWithFace(all_connected, map, ndef, pos, SHIFT_TO_FACE(j), pos_to_iterator);
			}
		}

		for(unsigned int j = 0; j < all_connected.size(); ++j) {
			bool exist_in_current = false;
			for(unsigned int k = 0; k < current_face_connected.size(); ++k) {
				if(all_connected[j] == current_face_connected[k]) {
					exist_in_current = true;
					break;
				}
			}

			if(!exist_in_current) {
				CircuitElementContainer tmp_container;
				// Add edges to the graph
				for(unsigned int k = 0; k < current_face_connected.size(); ++k) {
					if(!is_joint_created[pair_to_id_converter[all_connected[j]]]
					   [pair_to_id_converter[current_face_connected[k]]]) {
						// Create joint
						tmp_container.shift = current_face_connected[k].second;
						CircuitElementList::iterator first_iterator =
						    all_connected[j].first -> m_faces[all_connected[j].second].insert(
						        all_connected[j].first -> m_faces[all_connected[j].second].begin(), tmp_container);

						tmp_container.shift = all_connected[j].second;
						CircuitElementList::iterator second_iterator =
						    current_face_connected[k].first -> m_faces[current_face_connected[k].second].
						    insert(current_face_connected[k].first -> m_faces[current_face_connected[k].second].
						           begin(), tmp_container);

						first_iterator -> list_iterator = second_iterator;
						first_iterator -> list_pointer = current_face_connected[k].first
						                                 -> m_faces + current_face_connected[k].second;
						second_iterator -> list_iterator = first_iterator;
						second_iterator -> list_pointer = all_connected[j].first -> m_faces + i;

						// Set joint to created
						is_joint_created[pair_to_id_converter[all_connected[j]]]
						[pair_to_id_converter[current_face_connected[k]]] = true;
						is_joint_created[pair_to_id_converter[current_face_connected[k]]]
						[pair_to_id_converter[all_connected[j]]] = true;
						
						changed_elements.insert(current_face_connected[k].first);
						changed_elements.insert(all_connected[j].first);
					}
				}
			}
		}
	}
	
	leveldb::Status status;
	for(std::set<CircuitElement*>::iterator i = changed_elements.begin(); i != changed_elements.end(); ++i)
	{
		std::ostringstream out(std::ios_base::binary);
		(*i) -> serialize(out, pos_to_id);
		std::string str = out.str();
		status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[(*i) -> m_pos]), str);
		assert(status.ok());
	}
}

void Circuit::removeWire(Map& map, INodeDefManager* ndef, v3s16 pos, MapNode& node)
{
	JMutexAutoLock lock(m_elements_mutex);
	
	// This is used for converting elements of current_face_connected to their ids in all_connected.
	std::map <std::pair <CircuitElement*, int > , int> pair_to_id_converter;
	std::vector <std::pair <CircuitElement*, int > > all_connected;
	std::vector <std::pair <CircuitElement*, int > > current_face_connected;
	std::set <CircuitElement*> changed_elements;
	
	CircuitElement::findConnected(all_connected, map, ndef, pos, node, pos_to_iterator);
	for(unsigned int i = 0; i < all_connected.size(); ++i) {
		pair_to_id_converter[all_connected[i]] = i;
	}

	// For each face remove connections that depend on this block of wire.
	for(unsigned int i = 0; i < 6; ++i) {
		current_face_connected.clear();
		CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos, SHIFT_TO_FACE(i), pos_to_iterator);
		for(unsigned int j = 0; j < all_connected.size(); ++j) {
			bool exist_in_current = false;
			for(unsigned int k = 0; k < current_face_connected.size(); ++k) {
				if(all_connected[j] == current_face_connected[k]) {
					exist_in_current = true;
					break;
				}
			}
			if(!exist_in_current) {
				// Remove edges from graph
				for(unsigned int k = 0; k < current_face_connected.size(); ++k) {
					CircuitElementList* face =
					    current_face_connected[k].first -> m_faces + current_face_connected[k].second;
					for(CircuitElementList::iterator l = face -> begin();
					    l != face -> end();) {
						if(l -> list_pointer == all_connected[j].first -> m_faces + all_connected[j].second) {
							CircuitElementList::iterator tmp_l = l;
							
							changed_elements.insert(tmp_l -> list_pointer -> element);
							
							/*
							 * Save and increment iterator because erase invalidates
							 * only iterators to the erased elements.
							 */
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
	
	leveldb::Status status;
	for(std::set<CircuitElement*>::iterator i = changed_elements.begin(); i != changed_elements.end(); ++i)
	{
		std::ostringstream out(std::ios_base::binary);
		(*i) -> serialize(out, pos_to_id);
		std::string str = out.str();
		status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[(*i) -> m_pos]), str);
		assert(status.ok());
	}
}

void Circuit::update(float dtime, Map& map,  INodeDefManager* ndef)
{
	if(m_since_last_update > m_min_update_delay) {
		JMutexAutoLock lock(m_elements_mutex);
		
		m_since_last_update -= m_min_update_delay;
		// Each element send signal to other connected elements.
		for(std::list <CircuitElement>::iterator i = elements.begin();
		    i != elements.end(); ++i) {
			i -> update();
		}
		// Update state of each element.
		for(std::list <CircuitElement>::iterator i = elements.begin();
		    i != elements.end(); ++i) {
			i -> updateState(m_script, map, ndef);
		}
#ifdef CIRCUIT_DEBUG
		dstream << "Dt: " << dtime << " " << m_since_last_update << std::endl;
		for(std::list <CircuitElement>::iterator i = elements.begin();
		    i != elements.end(); ++i) {
			dstream << PP(i -> m_pos) << " " << &(*i) << ": ";
			for(int j = 0; j < 6; ++j) {
				dstream << " (";
				CircuitElementList tmp_face = i -> getFace(j);
				for(CircuitElementList::iterator k = tmp_face.begin();
				    k != tmp_face.end(); ++k) {
					dstream << k -> list_pointer->element << ", ";
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


void Circuit::updateElement(MapNode& node, v3s16 pos, INodeDefManager* ndef, const unsigned char* func)
{
	std::pair<const unsigned char*, unsigned long> node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
		node_func = circuit_elements_states.addState(func, node.param2);
	} else {
		node_func = circuit_elements_states.addState(func);
	}
	pos_to_iterator[pos] -> m_func = node_func.first;
	pos_to_iterator[pos] -> m_func_id = node_func.second;
}

void Circuit::pushElementToQueue(v3s16 pos)
{
	elements_queue.push_back(pos);
	pos_to_id[pos] = max_id;
	++max_id;
}
void Circuit::processElementsQueue(Map& map, INodeDefManager* ndef)
{
	if(elements_queue.size() > 0) {
		JMutexAutoLock lock(m_elements_mutex);
		
		std::map <v3s16, int> pos_to_id_converter;
		std::map <v3s16, int>::iterator current_element_iterator;
		std::vector <std::pair <CircuitElement*, int > > connected;
		std::vector <std::list<CircuitElement>::iterator> elements_queue_iterators(elements_queue.size());
		v3s16 pos;
		MapNode node;
		CircuitElementContainer tmp_container;
		std::vector <bool> processed(elements_queue.size(), false);

		for(unsigned int i = 0; i < elements_queue.size(); ++i) {
			pos_to_id_converter[elements_queue[i]] = i;
			node = map.getNode(elements_queue[i]);
			std::pair <const unsigned char*, unsigned long> node_func;
			if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
				// If block is rotatable, then rotate it's function.
				node_func = circuit_elements_states.addState(ndef->get(node).circuit_element_states, node.param2);
			} else {
				node_func = circuit_elements_states.addState(ndef->get(node).circuit_element_states);
			}
			pos_to_iterator[elements_queue[i]] = elements.insert(elements.begin(), CircuitElement(elements_queue[i], node_func.first, node_func.second));
			elements_queue_iterators[i] = pos_to_iterator[elements_queue[i]];
			map.setNode(elements_queue[i], node);
		}

		for(unsigned int i = 0; i < elements_queue.size(); ++i) {
			pos = elements_queue[i];
			// For each face create joint if it doesn't exist yet.
			for(int j = 0; j < 6; ++j) {
				connected.clear();
				CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(j), pos_to_iterator);
				for(unsigned int k = 0; k < connected.size(); ++k) {
					current_element_iterator = pos_to_id_converter.find(connected[k].first -> m_pos);
					if(!((current_element_iterator != pos_to_id_converter.end()) && processed[current_element_iterator -> second])) {
						// Create joint
						tmp_container.shift = connected[k].second;
						CircuitElementList::iterator first_iterator =
						    elements_queue_iterators[i] -> m_faces[j].insert(
						        elements_queue_iterators[i] -> m_faces[j].begin(), tmp_container);

						tmp_container.shift = j;
						CircuitElementList::iterator second_iterator =
						    connected[k].first -> m_faces[connected[k].second].
						    insert(connected[k].first -> m_faces[connected[k].second].
						           begin(), tmp_container);

						first_iterator -> list_iterator = second_iterator;
						first_iterator -> list_pointer = connected[k].first -> m_faces + connected[k].second;
						second_iterator -> list_iterator = first_iterator;
						second_iterator -> list_pointer = elements_queue_iterators[i] -> m_faces + j;
					}
				}
			}
			processed[i] = true;
		}
		
		for(unsigned int i = 0; i < elements_queue.size(); ++i) {
			std::ostringstream out(std::ios_base::binary);
			pos_to_iterator[elements_queue[i]] -> serialize(out, pos_to_id);
			std::string str = out.str();
			leveldb::Status status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[elements_queue[i]]), str);
			assert(status.ok());
		}

		elements_queue.clear();

		saveCircuitElementsStates();
	}
}

void Circuit::save()
{
	JMutexAutoLock lock(m_elements_mutex);
	std::ostringstream ostr(std::ios_base::binary);
	std::ofstream out((m_savedir + DIR_DELIM + elements_states_file).c_str(), std::ios_base::binary);
	for(std::list<CircuitElement>::iterator i = elements.begin(); i != elements.end(); ++i) {
		i -> serializeState(ostr, pos_to_id);
	}
	out << ostr.str();
}

void Circuit::saveElementConnections(std::list<CircuitElement>::iterator id)
{
	std::ostringstream out(std::ios_base::binary);
	id -> serialize(out, pos_to_id);
	std::string str = out.str();
	leveldb::Status status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[id -> m_pos]), str);
	assert(status.ok());
	for(int i = 0; i < 6; ++i) {
		for(CircuitElementList::iterator j = id -> m_faces[i].begin(); j != id -> m_faces[i].begin(); ++j) {
			out.clear();
			j -> list_pointer -> element -> serialize(out, pos_to_id);
			str = out.str();
			leveldb::Status status = m_database -> Put(leveldb::WriteOptions(), itos(pos_to_id[j -> list_pointer -> element -> m_pos]), str);
			assert(status.ok());
		}
	}
}

void Circuit::saveCircuitElementsStates()
{
	std::ostringstream ostr(std::ios_base::binary);
	std::ofstream out((m_savedir + DIR_DELIM + elements_func_file).c_str(), std::ios_base::binary);
	circuit_elements_states.serialize(ostr);
	out << ostr.str();
}

bool Circuit::isElementKey(std::string s)
{
	for(unsigned int i = 0; i < s.length(); ++i) {
		if((s[i] < '0') || (s[i] > '9')) {
			return false;
		}
	}
	return true;
}