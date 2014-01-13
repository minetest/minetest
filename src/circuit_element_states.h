#ifndef CIRCUIT_ELEMENT_STATES_H
#define CIRCUIT_ELEMENT_STATES_H

#include "circuit_element.h"

#include <vector>

class CircuitElementStates
{
public:
	CircuitElementStates();
	~CircuitElementStates();
	const unsigned char* addState(const unsigned char* state);
	const unsigned char* addState(const unsigned char* state, unsigned char facedir);
	void rotateStatesArray(const unsigned char* input_state, unsigned char* output_state, FaceId face);
	unsigned char rotateState(const unsigned char state, FaceId face);
	unsigned int getId(const unsigned char* state);
private:
	std::vector<const unsigned char*> states;
	static const int states_num;
};

#endif