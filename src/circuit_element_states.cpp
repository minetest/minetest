#include "circuit_element_states.h"
#include "circuit_element.h"
#include "log.h"
#include "debug.h"

CircuitElementStates::CircuitElementStates(unsigned int states_num, bool use_shifts) : m_states_num(states_num),
    m_use_shifts(use_shifts)
{
}

CircuitElementStates::~CircuitElementStates()
{
	for(unsigned int i = 0; i < m_states.size(); ++i) {
		delete m_states[i];
	}
}

std::pair<const unsigned char*, unsigned long> CircuitElementStates::addState(const unsigned char* state)
{
	unsigned int id = getId(state);
	if(id != m_states.size()) {
		return std::make_pair(m_states[id], static_cast<unsigned long>(id));
	} else {
		unsigned char* tmp_state = new unsigned char[m_states_num];
		for(unsigned int i = 0; i < m_states_num; ++i) {
			tmp_state[i] = state[i];
		}
		m_states.push_back(tmp_state);
		return std::make_pair(m_states[m_states.size() - 1], static_cast<unsigned long>(m_states.size() - 1));
	}
}

std::pair<const unsigned char*, unsigned long> CircuitElementStates::addState(const unsigned char* state, unsigned char facedir)
{
	FaceId face = FACEDIR_TO_FACE(facedir);
	unsigned char rotated_state[m_states_num];
	for(unsigned int i = 0; i < m_states_num; ++i) {
		rotated_state[i] = 0;
	}
	rotateStatesArray(state, rotated_state, face);
	return addState(rotated_state);
}

void CircuitElementStates::rotateStatesArray(const unsigned char* input_state, unsigned char* output_state, FaceId face)
{
	int id;
	unsigned char value;
	if(m_use_shifts) {
		for(unsigned int i = 0; i < m_states_num; ++i) {
			id = rotateState(SHIFT_TO_FACE(static_cast<unsigned char>(i)), face);
			id = FACE_TO_SHIFT(id);
			value = rotateState(input_state[i], face);
			output_state[id] = value;
		}
	} else {
		for(unsigned int i = 0; i < m_states_num; ++i) {
			id = rotateState(static_cast<unsigned char>(i), face);
			value = rotateState(input_state[i], face);
			output_state[id] = value;
		}
	}
}

unsigned char CircuitElementStates::rotateState(const unsigned char state, FaceId face)
{
	unsigned char result = 0;
	switch(face) {
	case FACE_BOTTOM:
		result |= (state & FACE_BOTTOM) << 1;
		result |= (state & FACE_BACK) << 2;
		result |= (state & FACE_LEFT);
		result |= (state & FACE_TOP) << 1;
		result |= (state & FACE_FRONT) >> 4;
		result |= (state & FACE_RIGHT);
		break;
	case FACE_BACK:
		result |= (state & FACE_BOTTOM);
		result |= (state & FACE_BACK) << 3;
		result |= (state & FACE_LEFT) << 3;
		result |= (state & FACE_TOP);
		result |= (state & FACE_FRONT) >> 3;
		result |= (state & FACE_RIGHT) >> 3;
		break;
	case FACE_LEFT:
		result |= (state & FACE_BOTTOM);
		result |= (state & FACE_BACK) << 4;
		result |= (state & FACE_LEFT) >> 1;
		result |= (state & FACE_TOP);
		result |= (state & FACE_FRONT) >> 2;
		result |= (state & FACE_RIGHT) >> 1;
		break;
	case FACE_TOP:
		result |= (state & FACE_BOTTOM) << 4;
		result |= (state & FACE_BACK) >> 1;
		result |= (state & FACE_LEFT);
		result |= (state & FACE_TOP) >> 2;
		result |= (state & FACE_FRONT) >> 1;
		result |= (state & FACE_RIGHT);
		break;
	case FACE_FRONT:
		result |= (state & FACE_BOTTOM);
		result |= (state & FACE_BACK);
		result |= (state & FACE_LEFT);
		result |= (state & FACE_TOP);
		result |= (state & FACE_FRONT);
		result |= (state & FACE_RIGHT);
		break;
	case FACE_RIGHT:
		result |= (state & FACE_BOTTOM);
		result |= (state & FACE_BACK) << 1;
		result |= (state & FACE_LEFT) << 2;
		result |= (state & FACE_TOP);
		result |= (state & FACE_FRONT) << 1;
		result |= (state & FACE_RIGHT) >> 4;
		break;
	default:
		errorstream << "CircuitElementStates::rotateState(): Unknown face." << std::endl;
	}
	return result;
}

unsigned int CircuitElementStates::getId(const unsigned char* state)
{
	unsigned int result = m_states.size();
	for(unsigned int i = 0; i < m_states.size(); ++i) {
		bool equal = true;
		for(unsigned int j = 0; j < m_states_num; ++j) {
			if(m_states[i][j] != state[j]) {
				equal = false;
				break;
			}
		}
		if(equal) {
			result = i;
			break;
		}
	}
	return result;
}

unsigned char* CircuitElementStates::getFunc(unsigned int id)
{
	return m_states[id];
}

void CircuitElementStates::serialize(std::ostream& out)
{
	unsigned long states_size = m_states.size();
	out.write(reinterpret_cast<char*>(&states_size), sizeof(states_size));
	for(unsigned int i = 0; i < m_states.size(); ++i)
	{
		for(unsigned int j = 0; j < m_states_num; ++j)
		{
			out.write(reinterpret_cast<char*>(&m_states[i][j]), sizeof(m_states[i][j]));
		}
	}
}

void CircuitElementStates::deSerialize(std::istream& in)
{
	unsigned long states_size = m_states.size();
	in.read(reinterpret_cast<char*>(&states_size), sizeof(states_size));
	m_states.resize(states_size);
	for(unsigned int i = 0; i < m_states.size(); ++i)
	{
		m_states[i] = new unsigned char[m_states_num];
		for(unsigned int j = 0; j < m_states_num; ++j)
		{
			in.read(reinterpret_cast<char*>(&m_states[i][j]), sizeof(m_states[i][j]));
		}
	}
}