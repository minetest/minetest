#ifndef CIRCUIT_ELEMENT_VIRTUAL_H
#define CIRCUIT_ELEMENT_VIRTUAL_H

#include <list>
#include <map>
#include <sstream>

class CircuitElement;

struct CircuitElementVirtualContainer
{
	unsigned char shift;
	std::list<CircuitElement>::iterator element_pointer;
};

class CircuitElementVirtual : public std::list <CircuitElementVirtualContainer>
{
public:
	CircuitElementVirtual(unsigned long id);
	~CircuitElementVirtual();

	void update();

	void serialize(std::ostream& out);
	void deSerialize(std::istream& is, std::list <CircuitElementVirtual>::iterator current_element_it,
	                 std::map <unsigned long, std::list<CircuitElement>::iterator>& id_to_pointer);

	void setId(unsigned long id);

	unsigned long getId();

	inline void addState(const bool state)
		{
			m_state |= state;
		}

private:
	unsigned long m_element_id;
	bool m_state;
};

#endif
