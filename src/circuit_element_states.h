#ifndef CIRCUIT_ELEMENT_STATES_H
#define CIRCUIT_ELEMENT_STATES_H

#include "circuit_element.h"

#include <vector>

class CircuitElementStates
{
public:
	CircuitElementStates(int states_num, bool use_shifts);
	~CircuitElementStates();
	const unsigned char* addState(const unsigned char* state);
	const unsigned char* addState(const unsigned char* state, unsigned char facedir);
	void rotateStatesArray(const unsigned char* input_state, unsigned char* output_state, FaceId face);
	static unsigned char rotateState(const unsigned char state, FaceId face);
	unsigned int getId(const unsigned char* state);
private:
	std::vector<const unsigned char*> m_states;
	const int m_states_num;
	const bool m_use_shifts;
};

#endif