/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "content_cao.h"

/*
	PlayerCAO
*/

class PlayerCAO : public ClientActiveObject
{
private:
	core::aabbox3d<f32> m_selection_box;
	scene::IMeshSceneNode *m_node;
	scene::ITextSceneNode* m_text;
	std::string m_name;
	v3f m_position;
	float m_yaw;
	SmoothTranslator pos_translator;
	bool m_is_local_player;
	LocalPlayer *m_local_player;
	float m_damage_visual_timer;

public:
	PlayerCAO(IGameDef *gamedef, ClientEnvironment *env):
			ClientActiveObject(0, gamedef, env),
			m_selection_box(-BS/3.,0.0,-BS/3., BS/3.,BS*2.0,BS/3.),
			m_node(NULL),
			m_text(NULL),
			m_position(v3f(0,10*BS,0)),
			m_yaw(0),
			m_is_local_player(false),
			m_local_player(NULL),
			m_damage_visual_timer(0)
		{
			if(gamedef == NULL)
				ClientActiveObject::registerType(getType(), create);
		}

	void initialize(const std::string &data)
		{
			infostream<<"PlayerCAO: Got init data"<<std::endl;

			std::istringstream is(data, std::ios::binary);
			// version
			u8 version = readU8(is);
			// check version
			if(version != 0)
				return;
			// name
			m_name = deSerializeString(is);
			// pos
			m_position = readV3F1000(is);
			// yaw
			m_yaw = readF1000(is);

			pos_translator.init(m_position);

			Player *player = m_env->getPlayer(m_name.c_str());
			if(player && player->isLocal()){
				m_is_local_player = true;
				m_local_player = (LocalPlayer*)player;
			}
		}

	~PlayerCAO()
		{
			if(m_node)
				m_node->remove();
		}

	inline u8 getType() const	{	return ACTIVEOBJECT_TYPE_PLAYER;	}

	static ClientActiveObject* create(IGameDef *gamedef, ClientEnvironment *env)
		{
			return new PlayerCAO(gamedef, env);
		}


	core::aabbox3d<f32>* getSelectionBox()
		{
			if(m_is_local_player)
				return NULL;
			return &m_selection_box;
		}


	void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
				IrrlichtDevice *irr)
		{
			if(m_node != NULL)
				return;
			if(m_is_local_player)
				return;

			//video::IVideoDriver* driver = smgr->getVideoDriver();
			gui::IGUIEnvironment* gui = irr->getGUIEnvironment();

			scene::SMesh *mesh = new scene::SMesh();
			{ // Front
			scene::IMeshBuffer *buf = new scene::SMeshBuffer();
			video::SColor c(255,255,255,255);
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
				video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
			};
			u16 indices[] = {0,1,2,2,3,0};
			buf->append(vertices, 4, indices, 6);
			// Set material
			buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
			buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
			buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
			buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			// Add to mesh
			mesh->addMeshBuffer(buf);
			buf->drop();
			}
			{ // Back
			scene::IMeshBuffer *buf = new scene::SMeshBuffer();
			video::SColor c(255,255,255,255);
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
				video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
				video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
				video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
			};
			u16 indices[] = {0,1,2,2,3,0};
			buf->append(vertices, 4, indices, 6);
			// Set material
			buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
			buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
			buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
			buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			// Add to mesh
			mesh->addMeshBuffer(buf);
			buf->drop();
			}
			m_node = smgr->addMeshSceneNode(mesh, NULL);
			mesh->drop();
			// Set it to use the materials of the meshbuffers directly.
			// This is needed for changing the texture in the future
			m_node->setReadOnlyMaterials(true);
			updateNodePos();

			// Add a text node for showing the name
			std::wstring wname = narrow_to_wide(m_name);
			m_text = smgr->addTextSceneNode(gui->getBuiltInFont(),
					wname.c_str(), video::SColor(255,255,255,255), m_node);
			m_text->setPosition(v3f(0, (f32)BS*2.1, 0));

			updateTextures("");
			updateNodePos();
		}

	void removeFromScene()
		{
			if(m_node == NULL)
				return;

			m_node->remove();
			m_node = NULL;
		}

	void updateLight(u8 light_at_pos)
		{
			if(m_node == NULL)
				return;

			m_node->setVisible(true);

			u8 li = decode_light(light_at_pos);
			video::SColor color(255,li,li,li);
			setMeshColor(m_node->getMesh(), color);
		}

	v3s16 getLightPosition()
		{
			return floatToInt(m_position+v3f(0,BS*1.5,0), BS);
		}

	void updateNodePos()
		{
			if(m_node == NULL)
				return;

			m_node->setPosition(pos_translator.vect_show);

			v3f rot = m_node->getRotation();
			rot.Y = -m_yaw;
			m_node->setRotation(rot);
		}

	void step(float dtime, ClientEnvironment *env)
		{
			pos_translator.translate(dtime);
			updateNodePos();

			if(m_damage_visual_timer > 0){
				m_damage_visual_timer -= dtime;
				if(m_damage_visual_timer <= 0){
					updateTextures("");
				}
			}

		}

	void processMessage(const std::string &data)
		{
			//infostream<<"PlayerCAO: Got message"<<std::endl;
			std::istringstream is(data, std::ios::binary);
			// command
			u8 cmd = readU8(is);
			if(cmd == AO_Message_type::SetPosition) // update position
			{
				// pos
				m_position = readV3F1000(is);
				// yaw
				m_yaw = readF1000(is);

				pos_translator.update(m_position, false);

				updateNodePos();
			}
			else if(cmd == AO_Message_type::Punched) // punched
			{
				// damage
				s16 damage = readS16(is);

				if(m_is_local_player)
					m_env->damageLocalPlayer(damage, false);

				m_damage_visual_timer = 0.5;
				updateTextures("^[brighten");
			}
		}

	void updateTextures(const std::string &mod)
		{
			if(!m_node)
				return;
			ITextureSource *tsrc = m_gamedef->tsrc();
			scene::IMesh *mesh = m_node->getMesh();
			if(mesh){
				{
					std::string tname = "player.png";
					tname += mod;
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(0);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw(tname));
				}
				{
					std::string tname = "player_back.png";
					tname += mod;
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(1);
					buf->getMaterial().setTexture(0,
							tsrc->getTextureRaw(tname));
				}
			}
		}

	inline v3f getPosition() 	{	return m_position;	}

};

// Prototype
PlayerCAO proto_PlayerCAO(NULL, NULL);
