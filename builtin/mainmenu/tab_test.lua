--Minetest
--Copyright (C) 2013 sapier
--
--This program is free software; you can redistribute it and/or modify
--it under the terms of the GNU Lesser General Public License as published by
--the Free Software Foundation; either version 2.1 of the License, or
--(at your option) any later version.
--
--This program is distributed in the hope that it will be useful,
--but WITHOUT ANY WARRANTY; without even the implied warranty of
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--GNU Lesser General Public License for more details.
--
--You should have received a copy of the GNU Lesser General Public License along
--with this program; if not, write to the Free Software Foundation, Inc.,
--51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

--------------------------------------------------------------------------------

tab_test = {
	name = "test",
	caption = fgettext("Test"),
	cbf_formspec = function (tabview, name, tabdata)
			local logofile = defaulttexturedir .. "logo.png"
			return
				"button[0,0;1,0.5;btn_test_2;BTN2]" ..
				"button[1,1;2,0;btn_test_1;BTN1]" ..
				 "field[1,2;2,0;te_test_1;;]" ..
				 "field[3.25,1;2.5,0;te_test_2;;1000]" ..
				 "button[6,0;2,0.5;btn_test_3;BTN3]" ..
				 "dropdown[6,1;2;dd_test1;0,10,20,30,40,50;0]" ..
				 "checkbox[3.25,2;cb_test1;Checkbox 1;false;Some tooltip]" ..
				 "textarea[6,2;2,3;te_multiline;Some text;]" ..
				 "label[3.25,0;Some test above 1000 textfield]" ..
				 "button[8.25,3;3,0.5;btn_test_4;BTN4]" ..
				 "checkbox[3.25,3;cb_test2;Checkbox 2 some very long caption to align with button]" ..
				 "textlist[1,4;4.75,2;tl_test_1;item1,item2,item3]" ..
				 "scrollbar[0,3;3,0.5;horizontal;sb_test1;500]" ..
				 "button[8.25,0;2,2;btn_test_5;BIGBTN]" ..
				 "vertlabel[10.5,0;VertText]" ..
				 "scrollbar[0,1;0.5,2;vertical;sb_test2;100]" ..
				 "image[8.25,4;3,1;" .. logofile .. "]"
			end
	}
