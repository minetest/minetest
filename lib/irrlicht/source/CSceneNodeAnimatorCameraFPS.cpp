// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneNodeAnimatorCameraFPS.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "Keycodes.h"
#include "ICursorControl.h"
#include "ICameraSceneNode.h"
#include "ISceneNodeAnimatorCollisionResponse.h"

namespace irr
{
namespace scene
{

//! constructor
CSceneNodeAnimatorCameraFPS::CSceneNodeAnimatorCameraFPS(gui::ICursorControl* cursorControl,
		f32 rotateSpeed, f32 moveSpeed, f32 jumpSpeed,
		SKeyMap* keyMapArray, u32 keyMapSize, bool noVerticalMovement, bool invertY)
: CursorControl(cursorControl), MaxVerticalAngle(88.0f),
	MoveSpeed(moveSpeed), RotateSpeed(rotateSpeed), JumpSpeed(jumpSpeed),
	MouseYDirection(invertY ? -1.0f : 1.0f),
	LastAnimationTime(0), firstUpdate(true), firstInput(true), NoVerticalMovement(noVerticalMovement)
{
	#ifdef _DEBUG
	setDebugName("CCameraSceneNodeAnimatorFPS");
	#endif

	if (CursorControl)
		CursorControl->grab();

	allKeysUp();

	// create key map
	if (!keyMapArray || !keyMapSize)
	{
		// create default key map
		KeyMap.push_back(SKeyMap(EKA_MOVE_FORWARD, irr::KEY_UP));
		KeyMap.push_back(SKeyMap(EKA_MOVE_BACKWARD, irr::KEY_DOWN));
		KeyMap.push_back(SKeyMap(EKA_STRAFE_LEFT, irr::KEY_LEFT));
		KeyMap.push_back(SKeyMap(EKA_STRAFE_RIGHT, irr::KEY_RIGHT));
		KeyMap.push_back(SKeyMap(EKA_JUMP_UP, irr::KEY_KEY_J));
	}
	else
	{
		// create custom key map
		setKeyMap(keyMapArray, keyMapSize);
	}
}


//! destructor
CSceneNodeAnimatorCameraFPS::~CSceneNodeAnimatorCameraFPS()
{
	if (CursorControl)
		CursorControl->drop();
}


//! It is possible to send mouse and key events to the camera. Most cameras
//! may ignore this input, but camera scene nodes which are created for
//! example with scene::ISceneManager::addMayaCameraSceneNode or
//! scene::ISceneManager::addFPSCameraSceneNode, may want to get this input
//! for changing their position, look at target or whatever.
bool CSceneNodeAnimatorCameraFPS::OnEvent(const SEvent& evt)
{
	switch(evt.EventType)
	{
	case EET_KEY_INPUT_EVENT:
		for (u32 i=0; i<KeyMap.size(); ++i)
		{
			if (KeyMap[i].KeyCode == evt.KeyInput.Key)
			{
				CursorKeys[KeyMap[i].Action] = evt.KeyInput.PressedDown;
				return true;
			}
		}
		break;

	case EET_MOUSE_INPUT_EVENT:
		if (evt.MouseInput.Event == EMIE_MOUSE_MOVED)
		{
			CursorPos = CursorControl->getRelativePosition();
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}


void CSceneNodeAnimatorCameraFPS::animateNode(ISceneNode* node, u32 timeMs)
{
	if (!node || node->getType() != ESNT_CAMERA)
		return;

	ICameraSceneNode* camera = static_cast<ICameraSceneNode*>(node);

	if (firstUpdate)
	{
		camera->updateAbsolutePosition();
		if (CursorControl )
		{
			CursorControl->setPosition(0.5f, 0.5f);
			CursorPos = CenterCursor = CursorControl->getRelativePosition();
		}

		LastAnimationTime = timeMs;

		firstUpdate = false;
	}

	// If the camera isn't the active camera, and receiving input, then don't process it.
	if(!camera->isInputReceiverEnabled())
	{
		firstInput = true;
		return;
	}

	if ( firstInput )
	{
		allKeysUp();
		firstInput = false;
	}

	scene::ISceneManager * smgr = camera->getSceneManager();
	if(smgr && smgr->getActiveCamera() != camera)
		return;

	// get time
	f32 timeDiff = (f32) ( timeMs - LastAnimationTime );
	LastAnimationTime = timeMs;

	// update position
	core::vector3df pos = camera->getPosition();

	// Update rotation
	core::vector3df target = (camera->getTarget() - camera->getAbsolutePosition());
	core::vector3df relativeRotation = target.getHorizontalAngle();

	if (CursorControl)
	{
		if (CursorPos != CenterCursor)
		{
			relativeRotation.Y -= (0.5f - CursorPos.X) * RotateSpeed;
			relativeRotation.X -= (0.5f - CursorPos.Y) * RotateSpeed * MouseYDirection;

			// X < MaxVerticalAngle or X > 360-MaxVerticalAngle

			if (relativeRotation.X > MaxVerticalAngle*2 &&
				relativeRotation.X < 360.0f-MaxVerticalAngle)
			{
				relativeRotation.X = 360.0f-MaxVerticalAngle;
			}
			else
			if (relativeRotation.X > MaxVerticalAngle &&
				relativeRotation.X < 360.0f-MaxVerticalAngle)
			{
				relativeRotation.X = MaxVerticalAngle;
			}

			// Do the fix as normal, special case below
			// reset cursor position to the centre of the window.
			CursorControl->setPosition(0.5f, 0.5f);
			CenterCursor = CursorControl->getRelativePosition();

			// needed to avoid problems when the event receiver is disabled
			CursorPos = CenterCursor;
		}

		// Special case, mouse is whipped outside of window before it can update.
		video::IVideoDriver* driver = smgr->getVideoDriver();
		core::vector2d<u32> mousepos(u32(CursorControl->getPosition().X), u32(CursorControl->getPosition().Y));
		core::rect<u32> screenRect(0, 0, driver->getScreenSize().Width, driver->getScreenSize().Height);

		// Only if we are moving outside quickly.
		bool reset = !screenRect.isPointInside(mousepos);

		if(reset)
		{
			// Force a reset.
			CursorControl->setPosition(0.5f, 0.5f);
			CenterCursor = CursorControl->getRelativePosition();
			CursorPos = CenterCursor;
 		}
	}

	// set target

	target.set(0,0, core::max_(1.f, pos.getLength()));
	core::vector3df movedir = target;

	core::matrix4 mat;
	mat.setRotationDegrees(core::vector3df(relativeRotation.X, relativeRotation.Y, 0));
	mat.transformVect(target);

	if (NoVerticalMovement)
	{
		mat.setRotationDegrees(core::vector3df(0, relativeRotation.Y, 0));
		mat.transformVect(movedir);
	}
	else
	{
		movedir = target;
	}

	movedir.normalize();

	if (CursorKeys[EKA_MOVE_FORWARD])
		pos += movedir * timeDiff * MoveSpeed;

	if (CursorKeys[EKA_MOVE_BACKWARD])
		pos -= movedir * timeDiff * MoveSpeed;

	// strafing

	core::vector3df strafevect = target;
	strafevect = strafevect.crossProduct(camera->getUpVector());

	if (NoVerticalMovement)
		strafevect.Y = 0.0f;

	strafevect.normalize();

	if (CursorKeys[EKA_STRAFE_LEFT])
		pos += strafevect * timeDiff * MoveSpeed;

	if (CursorKeys[EKA_STRAFE_RIGHT])
		pos -= strafevect * timeDiff * MoveSpeed;

	// For jumping, we find the collision response animator attached to our camera
	// and if it's not falling, we tell it to jump.
	if (CursorKeys[EKA_JUMP_UP])
	{
		const ISceneNodeAnimatorList& animators = camera->getAnimators();
		ISceneNodeAnimatorList::ConstIterator it = animators.begin();
		while(it != animators.end())
		{
			if(ESNAT_COLLISION_RESPONSE == (*it)->getType())
			{
				ISceneNodeAnimatorCollisionResponse * collisionResponse =
					static_cast<ISceneNodeAnimatorCollisionResponse *>(*it);

				if(!collisionResponse->isFalling())
					collisionResponse->jump(JumpSpeed);
			}

			it++;
		}
	}

	// write translation
	camera->setPosition(pos);

	// write right target
	target += pos;
	camera->setTarget(target);
}


void CSceneNodeAnimatorCameraFPS::allKeysUp()
{
	for (u32 i=0; i<EKA_COUNT; ++i)
		CursorKeys[i] = false;
}


//! Sets the rotation speed
void CSceneNodeAnimatorCameraFPS::setRotateSpeed(f32 speed)
{
	RotateSpeed = speed;
}


//! Sets the movement speed
void CSceneNodeAnimatorCameraFPS::setMoveSpeed(f32 speed)
{
	MoveSpeed = speed;
}


//! Gets the rotation speed
f32 CSceneNodeAnimatorCameraFPS::getRotateSpeed() const
{
	return RotateSpeed;
}


// Gets the movement speed
f32 CSceneNodeAnimatorCameraFPS::getMoveSpeed() const
{
	return MoveSpeed;
}


//! Sets the keyboard mapping for this animator
void CSceneNodeAnimatorCameraFPS::setKeyMap(SKeyMap *map, u32 count)
{
	// clear the keymap
	KeyMap.clear();

	// add actions
	for (u32 i=0; i<count; ++i)
	{
		KeyMap.push_back(map[i]);
	}
}

void CSceneNodeAnimatorCameraFPS::setKeyMap(const core::array<SKeyMap>& keymap)
{
	KeyMap=keymap;
}

const core::array<SKeyMap>& CSceneNodeAnimatorCameraFPS::getKeyMap() const
{
	return KeyMap;
}


//! Sets whether vertical movement should be allowed.
void CSceneNodeAnimatorCameraFPS::setVerticalMovement(bool allow)
{
	NoVerticalMovement = !allow;
}


//! Sets whether the Y axis of the mouse should be inverted.
void CSceneNodeAnimatorCameraFPS::setInvertMouse(bool invert)
{
	if (invert)
		MouseYDirection = -1.0f;
	else
		MouseYDirection = 1.0f;
}


ISceneNodeAnimator* CSceneNodeAnimatorCameraFPS::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorCameraFPS * newAnimator =
		new CSceneNodeAnimatorCameraFPS(CursorControl,	RotateSpeed, MoveSpeed, JumpSpeed,
											0, 0, NoVerticalMovement);
	newAnimator->setKeyMap(KeyMap);
	return newAnimator;
}


} // namespace scene
} // namespace irr

