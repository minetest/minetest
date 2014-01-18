#ifndef CIRCUIT_ELEMENT_LIST_H
#define CIRCUIT_ELEMENT_LIST_H

#include "circuit_element.h"
#include <list>

struct CircuitElementContainer;

class CircuitElementList : public std::list<CircuitElementContainer>
{
public:
	CircuitElement* element;
};

#endif