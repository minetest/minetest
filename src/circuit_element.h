#ifndef CIRCUIT_ELEMENT_H
#define CIRCUIT_ELEMENT_H

#include "irr_v3d.h"
#include "mapnode.h"

#include <list>
#include <vector>

#define OPPOSITE_SHIFT(x) (CircuitElement::opposite_shift[(x)])
#define OPPOSITE_FACE(x) ((((x)<<3) | ((x)>>3)) & 0x3f)
#define SHIFT_TO_FACE(x) (CircuitElement::shift_to_face[(x)])
#define FACE_TO_SHIFT(x) (CircuitElement::face_to_shift[(x)])
#define FACEDIR_TO_FACE(x) (CircuitElement::facedir_to_face[(x)])

class Map;
class MapNode;
class INodeDefManager;

enum FaceId
{
	FACE_BOTTOM = 0x1,
	FACE_BACK   = 0x2,
	FACE_LEFT   = 0x4,
	FACE_TOP    = 0x8,
	FACE_FRONT  = 0x10,
	FACE_RIGHT  = 0x20,
};

class CircuitElement;
class Circuit;
class GameScripting;

struct CircuitElementContainer
{
	/*
	 * Pointer to CircuitElement, which contains this object.
	*/
	CircuitElement* element;
	/*
	 * shift = face to which this object is connected
	 */
	unsigned char shift;
	/*
	 * iterator of CircuitElement::faces[index], which contains pointer to this object
	 */
	std::list <CircuitElementContainer>::iterator list_iterator;
	/*
	 * pointer to the CircuitElement::faces[index], which contains pointer to this object
	 */
	std::list <CircuitElementContainer>* list_pointer;
};

class CircuitElement
{
public:
	CircuitElement(v3s16 pos, MapNode& node, const unsigned char* func);
	~CircuitElement();
	void addConnectedElement();
	void update();
	void updateState(GameScripting* m_script, Map& map, INodeDefManager* ndef);
	// First - pointer to object to which connected.
	// Second - face id.
	static void findConnected(std::vector <std::pair <CircuitElement*, int > >& connected,
	                          Map& map, INodeDefManager* ndef, v3s16 pos, MapNode& node);
	static void findConnectedWithFace(std::vector <std::pair <CircuitElement*, int > >& connected,
	                                  Map& map, INodeDefManager* ndef, v3s16 pos, FaceId face);
	std::list <CircuitElementContainer>& getFace(int id);
	
	static unsigned char face_to_shift[33];
	static unsigned char opposite_shift[6];
	static FaceId shift_to_face[6];
	static FaceId facedir_to_face[6];
	friend Circuit;
private:
	v3s16 m_pos;
	MapNode m_node;
	const unsigned char* m_func;
	unsigned char m_current_input_state;
	unsigned char m_next_input_state;
	unsigned char m_current_output_state;
	unsigned char m_next_output_state;
	std::list <CircuitElementContainer> m_faces[6];
};

#endif