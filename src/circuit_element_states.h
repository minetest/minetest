#ifndef CIRCUIT_ELEMENT_STATES_H
#define CIRCUIT_ELEMENT_STATES_H

#include "circuit_element.h"

#include <vector>

class CircuitElementStates
{
public:
	CircuitElementStates(unsigned int states_num, bool use_shifts);
	~CircuitElementStates();
	
	/*
	 * first - pointer to the function
	 * second - function id in the array
	 */
	std::pair<const unsigned char*, unsigned long> addState(const unsigned char* state);
	std::pair<const unsigned char*, unsigned long> addState(const unsigned char* state, unsigned char facedir);
	
	void rotateStatesArray(const unsigned char* input_state, unsigned char* output_state, FaceId face) const;
	static unsigned char rotateState(const unsigned char state, FaceId face);
	
	unsigned int getId(const unsigned char* state) const;
	const unsigned char* getFunc(unsigned int id) const;
	
	void serialize(std::ostream& out);
	void deSerialize(std::istream& in);
private:
	std::vector<unsigned char*> m_states;
	const unsigned int m_states_num;
	const bool m_use_shifts;
};

#endif
