#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <list>
#include <vector>
#include <map>
#include "leveldb/db.h"

#include "circuit_element.h"
#include "circuit_element_virtual.h"
#include "irr_v3d.h"
#include "map.h"
#include "circuit_element_states.h"

class INodeDefManager;
class GameScripting;

class Circuit
{
public:
	Circuit(GameScripting* script, std::string savedir);
	~Circuit();
	void addElement(Map& map, INodeDefManager* ndef, v3s16 pos, const unsigned char* func);
	void removeElement(v3s16 pos);
	void addWire(Map& map, INodeDefManager* ndef, v3s16 pos);
	void removeWire(Map& map, INodeDefManager* ndef, v3s16 pos, MapNode& node);
	void update(float dtime, Map& map,  INodeDefManager* ndef);
	void updateElement(MapNode& node, v3s16 pos, INodeDefManager* ndef, const unsigned char* func);
	void pushElementToQueue(v3s16 pos);
	void processElementsQueue(Map& map, INodeDefManager* ndef);
	
	void save();
	void saveElement(std::list <CircuitElement>::iterator element, bool save_edges);
	void saveVirtualElement(std::list <CircuitElementVirtual>::iterator element, bool save_edges);
	void saveCircuitElementsStates();
	
private:
	std::list <CircuitElement> elements;
	std::list <CircuitElementVirtual> virtual_elements;

	std::map <v3s16, std::list<CircuitElement>::iterator> pos_to_iterator;
	std::map <const unsigned char*, unsigned long> func_to_id;

	std::vector <v3s16> elements_queue;
	CircuitElementStates circuit_elements_states;
	GameScripting* m_script;
	float m_min_update_delay;
	float m_since_last_update;

	unsigned long max_id;
	unsigned long max_virtual_id;
	
	std::string m_savedir;
	
	leveldb::DB *m_database;
	leveldb::DB *m_virtual_database;
	
	JMutex m_elements_mutex;
	
	static const unsigned long circuit_simulator_version;
	static const char elements_states_file[];
	static const char elements_func_file[];
};

#endif

