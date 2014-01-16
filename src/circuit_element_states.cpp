#include "circuit_element_states.h"
#include "circuit_element.h"
#include "log.h"
#include "debug.h"

CircuitElementStates::CircuitElementStates(int states_num, bool use_shifts) : m_states_num(states_num),
    m_use_shifts(use_shifts)
{
}

CircuitElementStates::~CircuitElementStates()
{
	for(unsigned int i = 0; i < m_states.size(); ++i) {
		delete m_states[i];
	}
}

const unsigned char* CircuitElementStates::addState(const unsigned char* state)
{
	unsigned int id = getId(state);
	if(id != m_states.size()) {
		return m_states[id];
	} else {
		unsigned char* tmp_state = new unsigned char[m_states_num];
		for(int i = 0; i < m_states_num; ++i) {
			tmp_state[i] = state[i];
		}
		m_states.push_back(tmp_state);
		return m_states[m_states.size() - 1];
	}
}

const unsigned char* CircuitElementStates::addState(const unsigned char* state, unsigned char facedir)
{
	FaceId face = FACEDIR_TO_FACE(facedir);
	unsigned char rotated_state[m_states_num];
	for(int i = 0; i < m_states_num; ++i) {
		rotated_state[i] = 0;
	}
	rotateStatesArray(state, rotated_state, face);
	return addState(rotated_state);
}

unsigned int CircuitElementStates::getId(const unsigned char* state)
{
	unsigned int result = m_states.size();
	for(unsigned int i = 0; i < m_states.size(); ++i) {
		bool equal = true;
		for(int j = 0; j < m_states_num; ++j) {
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

void CircuitElementStates::rotateStatesArray(const unsigned char* input_state, unsigned char* output_state, FaceId face)
{
	int id;
	unsigned char value;
	if(m_use_shifts) {
		for(int i = 0; i < m_states_num; ++i) {
			id = rotateState(SHIFT_TO_FACE(static_cast<unsigned char>(i)), face);
			id = FACE_TO_SHIFT(id);
			value = rotateState(input_state[i], face);
			output_state[id] = value;
		}
	} else {
		for(int i = 0; i < m_states_num; ++i) {
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