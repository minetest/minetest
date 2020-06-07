#include "gui/guiInventoryList.h"
#include "network/networkprotocol.h"
#include "parsers.h"
#include "FormspecParseException.h"
#include "FieldSpec.h"

namespace {
	void parseContainer(ParserState &state, const FormspecElement &element)
	{
		element.checkLength(state.formspec_version > FORMSPEC_API_VERSION, 1);

		v2f32 c_offset = element.getV2f(0);
		state.container_stack.push(state.pos_offset);
		state.pos_offset += c_offset;
	}

	void parseList(ParserState &state, const FormspecElement &element)
	{
//		if (m_client == 0) {
//			throw FormspecParseException(false, "Not supported when not a client");
//		}

		if (element.size() != 4 && element.size() != 5) {
			element.checkLength(state.formspec_version > FORMSPEC_API_VERSION, 5);
		}

		std::string location = element.getString(0);
		std::string listname = element.getString(1);
		v2f32 givenPos = element.getV2f(2);
		v2s32 geom = element.getV2s(3);

		std::string startindex;
		if (element.size() == 5)
			startindex = element.getString(4);

		InventoryLocation loc;

		if (location == "context" || location == "current_name")
			loc = state.inventoryLocation;
		else
			loc.deSerialize(location);

		s32 start_i = 0;
		if (!startindex.empty())
			start_i = stoi(startindex);

		if (geom.X < 0 || geom.Y < 0 || start_i < 0)
			throw FormspecParseException(true, "Negative size or start index");

		if (!state.explicit_size)
			warningstream << "invalid use of list without a size[] element" << std::endl;

		auto &spec = state.makeSpec(3);
		v2f32 slot_spacing = state.real_coordinates ?
							 v2f32(state.imgsize.X * 1.25f, state.imgsize.Y * 1.25f) : state.spacing;

		v2s32 pos = state.getPosition(givenPos);
		core::rect<s32> rect = core::rect<s32>(pos.X, pos.Y,
			pos.X + (geom.X - 1) * slot_spacing.X + state.imgsize.X,
			pos.Y + (geom.Y - 1) * slot_spacing.Y + state.imgsize.Y);

//		GUIInventoryList *e = new GUIInventoryList(state.Environment, state.getParentElement(),
//				spec.fid, rect, m_invmgr, loc, listname, geom, start_i, state.imgsize,
//				slot_spacing, state.getParentElement(), state.inventorylist_options, m_font);
//
//		m_inventorylists.push_back(e);
	}
}


#define bind(name, func) \
	parsers[(name)] = std::bind(&func, std::placeholders::_1, std::placeholders::_2)

void initParsers(std::map<std::string, FormspecElementParser> &parsers)
{
	bind("container", parseContainer);
	bind("list", parseList);
}
