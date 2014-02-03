#include "circuit.h"
#include "circuit_element.h"
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

const unsigned long Circuit::circuit_simulator_version = 1ul;
const char Circuit::elements_states_file[] = "circuit_elements_states";
const char Circuit::elements_func_file[] = "circuit_elements_func";

Circuit::Circuit(GameScripting* script, std::string savedir) :  m_circuit_elements_states(64u, false),
                                                                m_script(script), m_min_update_delay(0.2f),
                                                                m_since_last_update(0.0f), m_max_id(0ul), m_max_virtual_id(1ul),
                                                                m_savedir(savedir), m_updating_process(false)
{
	load();
}

Circuit::~Circuit()
{
	save();
	m_elements.clear();
	delete m_database;
	delete m_virtual_database;
}

void Circuit::addElement(Map& map, INodeDefManager* ndef, v3s16 pos, const unsigned char* func)
{
	JMutexAutoLock lock(m_elements_mutex);

	bool already_existed[6];
	bool connected_faces[6] = {0};
	
	std::vector <std::pair <std::list <CircuitElement>::iterator, int > > connected;
	MapNode node = map.getNode(pos);

	std::pair <const unsigned char*, unsigned long> node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
		// If block is rotatable, then rotate it's function.
		node_func = m_circuit_elements_states.addState(func, node.param2);
	} else {
		node_func = m_circuit_elements_states.addState(func);
	}
	saveCircuitElementsStates();
	std::list <CircuitElement>::iterator current_element_iterator =
		m_elements.insert(m_elements.begin(), CircuitElement(pos, node_func.first, node_func.second,
		                                                     m_max_id++, ndef->get(node).circuit_element_delay));
	m_pos_to_iterator[pos] = current_element_iterator;

	// For each face add all other connected faces.
	for(int i = 0; i < 6; ++i) {
		if(!connected_faces[i]) {
			connected.clear();
			CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(i), m_pos_to_iterator, connected_faces);
			if(connected.size() > 0) {
				std::list <CircuitElementVirtual>::iterator virtual_element_it;
				bool found = false;
				for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator j = connected.begin();
				    j != connected.end(); ++j) {
					if(j->first->getFace(j->second).is_connected) {
						virtual_element_it = j->first->getFace(j->second).list_pointer;
						found = true;
						break;
					}
				}

				// If virtual element already exist
				if(found) {
					already_existed[i] = true;
				} else {
					already_existed[i] = false;
					virtual_element_it = m_virtual_elements.insert(m_virtual_elements.begin(),
					                                               CircuitElementVirtual(m_max_virtual_id++));
				}

				std::list <CircuitElementVirtualContainer>::iterator it;
				for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator j = connected.begin();
				    j != connected.end(); ++j) {
					if(!j->first->getFace(j->second).is_connected) {
						it = virtual_element_it->insert(virtual_element_it->begin(), CircuitElementVirtualContainer());
						it->shift = j->second;
						it->element_pointer = j->first;
						j->first->connectFace(j->second, it, virtual_element_it);
					}
				}
				it = virtual_element_it->insert(virtual_element_it->begin(), CircuitElementVirtualContainer());
				it->shift = i;
				it->element_pointer = current_element_iterator;
				current_element_iterator->connectFace(i, it, virtual_element_it);
			}

		}
	}

	for(int i = 0; i < 6; ++i) {
		if(current_element_iterator->getFace(i).is_connected && !already_existed[i]) {
			saveVirtualElement(current_element_iterator->getFace(i).list_pointer, true);
		}
	}
	saveElement(current_element_iterator, true);
}

void Circuit::removeElement(v3s16 pos)
{
	JMutexAutoLock lock(m_elements_mutex);

	std::vector <std::list <CircuitElementVirtual>::iterator> virtual_elements_for_update;
	std::list <CircuitElement>::iterator current_element = m_pos_to_iterator[pos];
	leveldb::Status status = m_database->Delete(leveldb::WriteOptions(), itos(current_element->getId()));	
	assert(status.ok());

	current_element->getNeighbors(virtual_elements_for_update);
	
	m_elements.erase(current_element);
	
	for(std::vector <std::list <CircuitElementVirtual>::iterator>::iterator i = virtual_elements_for_update.begin();
	    i != virtual_elements_for_update.end(); ++i) {
		if((*i)->size() > 1) {
			std::ostringstream out(std::ios_base::binary);
			(*i)->serialize(out);
			status = m_virtual_database->Put(leveldb::WriteOptions(), itos((*i)->getId()), out.str());
			assert(status.ok());
		} else {
			status = m_virtual_database->Delete(leveldb::WriteOptions(), itos((*i)->getId()));
			assert(status.ok());
			std::list <CircuitElement>::iterator element_to_save;
			for(std::list <CircuitElementVirtualContainer>::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
				element_to_save = j->element_pointer;
			}
			m_virtual_elements.erase(*i);
			saveElement(element_to_save, false);
		}
	}

	m_pos_to_iterator.erase(pos);
}

void Circuit::addWire(Map& map, INodeDefManager* ndef, v3s16 pos)
{
	JMutexAutoLock lock(m_elements_mutex);

	// This is used for converting elements of current_face_connected to their ids in all_connected.
	std::vector <std::pair <std::list <CircuitElement>::iterator, int> > all_connected;
	std::vector <std::list <CircuitElementVirtual>::iterator> created_virtual_elements;

	bool used[6][6];
	bool connected_faces[6];
	for(int i = 0; i < 6; ++i) {
		for(int j = 0; j < 6; ++j) {
			used[i][j] = false;
		}
	}

	MapNode node = map.getNode(pos);

	// For each face connect faces, that are not yet connected.
	for(int i = 0; i < 6; ++i) {
		all_connected.clear();
		for(unsigned int j = 0; j < 6; ++j) {
			if((ndef->get(node).wire_connections[i] & (SHIFT_TO_FACE(j))) && !used[i][j]) {
				CircuitElement::findConnectedWithFace(all_connected, map, ndef, pos, SHIFT_TO_FACE(j),
				                                      m_pos_to_iterator, connected_faces);
				used[i][j] = true;
				used[j][i] = true;
			}
		}

		if(all_connected.size() > 1) {
			CircuitElementContainer element_with_virtual;
			bool found_virtual = false;
			for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator i = all_connected.begin();
			    i != all_connected.end(); ++i) {
				if(i->first->getFace(i->second).is_connected) {
					element_with_virtual = i->first->getFace(i->second);
					found_virtual = true;
					break;
				}
			}

			if(found_virtual) {
				// Clear old connections (remove some virtual elements)
				for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator i = all_connected.begin();
				    i != all_connected.end(); ++i) {
					if(i->first->getFace(i->second).is_connected
					   && (i->first->getFace(i->second).list_pointer != element_with_virtual.list_pointer)) {
						leveldb::Status status = m_virtual_database->
							Delete(leveldb::WriteOptions(), itos(i->first->getFace(i->second).list_pointer->getId()));
						i->first->disconnectFace(i->second);
						m_virtual_elements.erase(i->first->getFace(i->second).list_pointer);
					}
				}
			} else {
				element_with_virtual.list_pointer = m_virtual_elements.insert(m_virtual_elements.begin(),
				                                                              CircuitElementVirtual(m_max_virtual_id++));
			}
			created_virtual_elements.push_back(element_with_virtual.list_pointer);

			// Create new connections
			for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator i = all_connected.begin();
			    i != all_connected.end(); ++i) {
				if(!(i->first->getFace(i->second).is_connected)) {
					std::list <CircuitElementVirtualContainer>::iterator it = element_with_virtual.list_pointer->insert(
						element_with_virtual.list_pointer->begin(), CircuitElementVirtualContainer());
					it->element_pointer = i->first;
					it->shift = i->second;
					i->first->connectFace(i->second, it, element_with_virtual.list_pointer);
				}
			}
		}
	}
	
	for(unsigned int i = 0; i < created_virtual_elements.size(); ++i) {
		saveVirtualElement(created_virtual_elements[i], true);
	}
}

void Circuit::removeWire(Map& map, INodeDefManager* ndef, v3s16 pos, MapNode& node)
{
	JMutexAutoLock lock(m_elements_mutex);
	
	std::vector <std::pair <std::list <CircuitElement>::iterator, int> > current_face_connected;

	bool connected_faces[6];
	for(int i = 0; i < 6; ++i) {
		connected_faces[i] = false;
	}

	// Find and remove virtual elements
	bool found_virtual_elements = false;
	for(int i = 0; i < 6; ++i) {
		if(!connected_faces[i]) {		
			current_face_connected.clear();
			CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos,
			                                      SHIFT_TO_FACE(i), m_pos_to_iterator, connected_faces);
			for(unsigned int j = 0; j < current_face_connected.size(); ++j) {
				CircuitElementContainer current_edge = current_face_connected[j].first->getFace(current_face_connected[j].second);
				if(current_edge.is_connected) {
					found_virtual_elements = true;
					leveldb::Status status = m_virtual_database->
						Delete(leveldb::WriteOptions(), itos(current_edge.list_pointer->getId()));
					assert(status.ok());

					m_virtual_elements.erase(current_edge.list_pointer);
					break;
				}
			}

			for(unsigned int j = 0; j < current_face_connected.size(); ++j) {
				saveElement(current_face_connected[j].first, false);
			}

		}
	}

	for(int i = 0; i < 6; ++i) {
		connected_faces[i] = false;
	}
	
	if(found_virtual_elements){
		// Restore some previously deleted connections.
		for(int i = 0; i < 6; ++i) {
			if(!connected_faces[i]) {
				current_face_connected.clear();
				CircuitElement::findConnectedWithFace(current_face_connected, map, ndef, pos, SHIFT_TO_FACE(i),
				                                      m_pos_to_iterator, connected_faces);

				if(current_face_connected.size() > 1) {
					std::list <CircuitElementVirtual>::iterator new_virtual_element = m_virtual_elements.insert(
						m_virtual_elements.begin(), CircuitElementVirtual(m_max_virtual_id++));

					for(unsigned int j = 0; j < current_face_connected.size(); ++j) {
						std::list <CircuitElementVirtualContainer>::iterator new_container = new_virtual_element->insert(
							new_virtual_element->begin(), CircuitElementVirtualContainer());
						new_container->element_pointer = current_face_connected[j].first;
						new_container->shift = current_face_connected[j].second;
						current_face_connected[j].first->connectFace(current_face_connected[j].second,
						                                             new_container, new_virtual_element);

						saveElement(current_face_connected[j].first, false);
					}

					saveVirtualElement(new_virtual_element, false);
				}
			}
		}
	}
}

void Circuit::update(float dtime, Map& map,  INodeDefManager* ndef)
{
	if(m_since_last_update > m_min_update_delay) {
		JMutexAutoLock lock(m_elements_mutex);
		m_updating_process = true;
		
		m_since_last_update -= m_min_update_delay;
		// Each element send signal to other connected virtual elements.
		for(std::list <CircuitElement>::iterator i = m_elements.begin();
		    i != m_elements.end(); ++i) {
			i->update();
		}

		// Each virtual element send signal to other connected elements.
		for(std::list <CircuitElementVirtual>::iterator i = m_virtual_elements.begin();
		    i != m_virtual_elements.end(); ++i) {
			i->update();
		}

		// Update state of each element.
		for(std::list <CircuitElement>::iterator i = m_elements.begin();
		    i != m_elements.end(); ++i) {
			i->updateState(m_script, map, ndef);
		}

		m_updating_process = false;
#ifdef CIRCUIT_DEBUG
		dstream << "Dt: " << dtime << " " << m_since_last_update << std::endl;
		for(std::list <CircuitElement>::iterator i = m_elements.begin();
		    i != m_elements.end(); ++i) {
			dstream << PP(i->getPos()) << " " << i->getId() << ": ";
			for(int j = 0; j < 6; ++j) {
				CircuitElementContainer tmp_face = i->getFace(j);
				if(tmp_face.list_pointer) {
					dstream << tmp_face.list_pointer->getId();
				}
				dstream << ", ";
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
	if(!m_updating_process) {
		m_elements_mutex.Lock();
	}

	std::list <CircuitElement>::iterator current_element = m_pos_to_iterator[pos];
	std::pair<const unsigned char*, unsigned long> node_func;
	if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
		node_func = m_circuit_elements_states.addState(func, node.param2);
	} else {
		node_func = m_circuit_elements_states.addState(func);
	}
	current_element->setFunc(node_func.first, node_func.second);
	current_element->setDelay(ndef->get(node).circuit_element_delay);
	saveCircuitElementsStates();
	saveElement(current_element, false);

	if(!m_updating_process) {
		m_elements_mutex.Unlock();
	}
}

void Circuit::pushElementToQueue(v3s16 pos)
{
	m_elements_queue.push_back(pos);
}

void Circuit::processElementsQueue(Map& map, INodeDefManager* ndef)
{
	if(m_elements_queue.size() > 0) {
		JMutexAutoLock lock(m_elements_mutex);

		std::vector <std::pair <std::list <CircuitElement>::iterator, int> > connected;
		std::vector <std::list <CircuitElementVirtual>::iterator> created_virtual_elements;
		MapNode node;
		bool connected_faces[6];

		// Filling with empty elements
		for(unsigned int i = 0; i < m_elements_queue.size(); ++i) {
			node = map.getNode(m_elements_queue[i]);
			std::pair <const unsigned char*, unsigned long> node_func;
			if(ndef->get(node).param_type_2 == CPT2_FACEDIR) {
				node_func = m_circuit_elements_states.addState(ndef->get(node).circuit_element_states, node.param2);
			} else {
				node_func = m_circuit_elements_states.addState(ndef->get(node).circuit_element_states);
			}
			m_pos_to_iterator[m_elements_queue[i]] = m_elements.insert(m_elements.begin(),
			                                                           CircuitElement(m_elements_queue[i],
				    node_func.first, node_func.second,
				    m_max_id++, ndef->get(node).circuit_element_delay));
		}

		for(unsigned int i = 0; i < m_elements_queue.size(); ++i) {
			v3s16 pos = m_elements_queue[i];
			std::list <CircuitElement>::iterator current_element_it = m_pos_to_iterator[pos];
			for(int j = 0; j < 6; ++j) {
				connected_faces[j] = false;
			}
			for(int j = 0; j < 6; ++j) {
				if(!current_element_it->getFace(j).is_connected && !connected_faces[j]) {
					connected.clear();
					CircuitElement::findConnectedWithFace(connected, map, ndef, pos, SHIFT_TO_FACE(j),
					                                      m_pos_to_iterator, connected_faces);

					if(!connected.empty()) {
						std::list <CircuitElementVirtual>::iterator virtual_element_it;
						bool found = false;
						for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator k = connected.begin();
						    k != connected.end(); ++k) {
							if(k->first->getFace(k->second).is_connected) {
								virtual_element_it = k->first->getFace(k->second).list_pointer;
								found = true;
								break;
							}
						}

						if(!found) {
							virtual_element_it = m_virtual_elements.insert(m_virtual_elements.begin(),
							                                               CircuitElementVirtual(m_max_virtual_id++));
						}

						std::list <CircuitElementVirtualContainer>::iterator it;
						for(std::vector <std::pair <std::list <CircuitElement>::iterator, int> >::iterator k = connected.begin();
						    k != connected.end(); ++k) {
							if(!k->first->getFace(k->second).is_connected) {
								it = virtual_element_it->insert(virtual_element_it->begin(), CircuitElementVirtualContainer());
								it->shift = k->second;
								it->element_pointer = k->first;
								k->first->connectFace(k->second, it, virtual_element_it);
							}
						}
						it = virtual_element_it->insert(virtual_element_it->begin(), CircuitElementVirtualContainer());
						it->shift = j;
						it->element_pointer = current_element_it;
						current_element_it->connectFace(j, it, virtual_element_it);
					}
				}
			}
		}
		
		for(unsigned int i = 0; i < created_virtual_elements.size(); ++i) {
			saveVirtualElement(created_virtual_elements[i], true);
		}

		for(unsigned int i = 0; i < m_elements_queue.size(); ++i) {
			saveElement(m_pos_to_iterator[m_elements_queue[i]], false);
		}

		m_elements_queue.clear();

		saveCircuitElementsStates();

	}
}

void Circuit::load()
{
	unsigned long element_id;
	unsigned long version = 0;
	std::istringstream in(std::ios_base::binary);
	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::Status status = leveldb::DB::Open(options, m_savedir + DIR_DELIM + "circuit.db", &m_database);
	assert(status.ok());
	status = leveldb::DB::Open(options, m_savedir + DIR_DELIM + "circuit_virtual.db", &m_virtual_database);
	assert(status.ok());
	
	std::ifstream input_elements_func((m_savedir + DIR_DELIM + elements_func_file).c_str(), std::ios_base::binary);
	if(input_elements_func.good()) {
		input_elements_func.read(reinterpret_cast<char*>(&version), sizeof(version));
		m_circuit_elements_states.deSerialize(input_elements_func);
	}

	// Filling list with empty virtual elements
	leveldb::Iterator* virtual_it = m_virtual_database->NewIterator(leveldb::ReadOptions());
	std::map <unsigned long, std::list <CircuitElementVirtual>::iterator> id_to_virtual_pointer;
	for(virtual_it->SeekToFirst(); virtual_it->Valid(); virtual_it->Next()) {
		element_id = stoi(virtual_it->key().ToString());
		id_to_virtual_pointer[element_id] =
			m_virtual_elements.insert(m_virtual_elements.begin(), CircuitElementVirtual(element_id));
		if(element_id + 1 > m_max_virtual_id) {
			m_max_virtual_id = element_id + 1;
		}
	}
	assert(virtual_it->status().ok());

	// Filling list with empty elements
	leveldb::Iterator* it = m_database->NewIterator(leveldb::ReadOptions());
	std::map <unsigned long, std::list <CircuitElement>::iterator> id_to_pointer;
	for(it->SeekToFirst(); it->Valid(); it->Next()) {
		element_id = stoi(it->key().ToString());
		id_to_pointer[element_id] =
			m_elements.insert(m_elements.begin(), CircuitElement(element_id));
		if(element_id + 1 > m_max_id) {
			m_max_id = element_id + 1;
		}
	}
	assert(it->status().ok());

	// Loading states of elements
	std::ifstream input_elements_states((m_savedir + DIR_DELIM + elements_states_file).c_str());
	if(input_elements_states.good()) {
		for(unsigned long i = 0; i < m_elements.size(); ++i) {
			input_elements_states.read(reinterpret_cast<char*>(&element_id), sizeof(element_id));
			if(id_to_pointer.find(element_id) != id_to_pointer.end()) {
				id_to_pointer[element_id]->deSerializeState(input_elements_states);
			} else {
				throw SerializationError(static_cast<std::string>("File \"")
				                         + elements_states_file + "\" seems to be corrupted.");
			}
		}
	}

	// Loading elements data
	for(it->SeekToFirst(); it->Valid(); it->Next()) {
		in.str(it->value().ToString());
		element_id = stoi(it->key().ToString());
		std::list <CircuitElement>::iterator current_element = id_to_pointer[element_id];
		current_element->deSerialize(in, id_to_virtual_pointer);
		current_element->setFunc(m_circuit_elements_states.getFunc(current_element->getFuncId()), current_element->getFuncId());
		m_pos_to_iterator[current_element->getPos()] = current_element;
	}
	assert(it->status().ok());
	delete it;

	// Loading virtual elements data
	for(virtual_it->SeekToFirst(); virtual_it->Valid(); virtual_it->Next()) {
		in.str(virtual_it->value().ToString());
		element_id = stoi(virtual_it->key().ToString());
		std::list <CircuitElementVirtual>::iterator current_element = id_to_virtual_pointer[element_id];
		current_element->deSerialize(in, current_element, id_to_pointer);
	}
	assert(virtual_it->status().ok());

	delete virtual_it;
}

void Circuit::save()
{
	JMutexAutoLock lock(m_elements_mutex);
	std::ostringstream ostr(std::ios_base::binary);
	std::ofstream out((m_savedir + DIR_DELIM + elements_states_file).c_str(), std::ios_base::binary);
	for(std::list<CircuitElement>::iterator i = m_elements.begin(); i != m_elements.end(); ++i) {
		i->serializeState(ostr);
	}
	out << ostr.str();
}

inline void Circuit::saveElement(std::list<CircuitElement>::iterator element, bool save_edges)
{
	std::ostringstream out(std::ios_base::binary);
	element->serialize(out);
	leveldb::Status status = m_database->Put(leveldb::WriteOptions(), itos(element->getId()), out.str());
	assert(status.ok());
	if(save_edges) {
		for(int i = 0; i < 6; ++i) {
			CircuitElementContainer tmp_container = element->getFace(i);
			if(tmp_container.is_connected) {
				std::ostringstream out(std::ios_base::binary);
				tmp_container.list_pointer->serialize(out);
				status = m_virtual_database->Put(leveldb::WriteOptions(),
				                                 itos(tmp_container.list_pointer->getId()), out.str());
			}
		}
	}
}

inline void Circuit::saveVirtualElement(std::list <CircuitElementVirtual>::iterator element, bool save_edges)
{
	std::ostringstream out(std::ios_base::binary);
	element->serialize(out);
	leveldb::Status status = m_virtual_database->Put(leveldb::WriteOptions(), itos(element->getId()), out.str());
	assert(status.ok());
	if(save_edges) {
		for(std::list <CircuitElementVirtualContainer>::iterator i = element->begin(); i != element->end(); ++i) {
			std::ostringstream out(std::ios_base::binary);
			i->element_pointer->serialize(out);
			status = m_database->Put(leveldb::WriteOptions(), itos(i->element_pointer->getId()), out.str());
		}
	}
}

void Circuit::saveCircuitElementsStates()
{
	std::ostringstream ostr(std::ios_base::binary);
	std::ofstream out((m_savedir + DIR_DELIM + elements_func_file).c_str(), std::ios_base::binary);
	ostr.write(reinterpret_cast<const char*>(&circuit_simulator_version), sizeof(circuit_simulator_version));
	m_circuit_elements_states.serialize(ostr);
	out << ostr.str();
}
