/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CALLBACKMANAGER_HEADER
#define CALLBACKMANAGER_HEADER

#include <vector>
#include <algorithm>

class CallbackHandler;
class CallbackManager;

typedef void (*CallbackFunction)();


struct CallbackRegistration
{
	CallbackRegistration(CallbackManager *manager, CallbackFunction function)
	{
		this->manager = manager;
		this->function = function;
	}

	/**< The CallbackManager that \p function is registered with. Note that
	 *   a single callback function can be registered with more than one
	 *   callback manager (but not more that once with an individual manager)
	 */
	CallbackManager *manager;


	/**< The function to be registered with \p manager. A function may only
	 *   be registered with an individual CallbackManager (attempting to
	 *   register the same function more than once will fail) however it may
	 *   be registered with one or more other CallbackManagers.
	 */
	CallbackFunction function;
};


/*--------------------------------------------------------------------------*/
/**
 * This class maintains a list of callback functions that get called when
 * process_callbacks() is called; the callback functions are not called
 * automatically so that where in the code they are processed can be
 * controlled. This class would normally be subclassed but this is not
 * essential. Used in conjunction with class CallbackHandler these pair of
 * classes can enable registration and removal of callbacks when the handler
 * class is destroyed (for example, to avoid dangling function pointers
 * within an object of this class), and this is the recommended use case.
 *
 */
class CallbackManager
{
	friend class CallbackHandler;

public:

	/**
	 * Add (register) \p function to this callback manager. Registered
	 * functions are called when process_callbacks() is called.
	 */
	bool add(CallbackFunction function)
	{
		bool success;
		if (!(success = is_registered(function)))
			m_callbacks.push_back(function);

		return success;
	}

	/**
	 * \return Returns true if \p function is registered with this callback
	 * manager.
	 */
	bool is_registered(CallbackFunction function)
	{
		return std::find(m_callbacks.begin(), m_callbacks.end(), function)
				!= m_callbacks.end();
	}

	/**
	 * Call each of the callback functions registered. The order that the
	 * callbacks are called is undefined and may change in future.
	 */
	void process_callbacks()
	{
		for (std::vector<CallbackFunction>::iterator it = m_callbacks.begin();
			 it != m_callbacks.end(); ++it) {
			(*it)();
		}
	}

protected:

	/**
	 * Removes a callback from the function callback list. This should not
	 * be used by user code and CallbackHandler::remove_callback() used
	 * instead.
	 */
	bool remove(CallbackFunction function)
	{
		std::vector<CallbackFunction>::iterator it = std::find(
				m_callbacks.begin(), m_callbacks.end(), function);
		if (it != m_callbacks.end())
			m_callbacks.erase(it);
		return it != m_callbacks.end;
	}

private:
	std::vector<CallbackFunction> m_callbacks;
};


/*--------------------------------------------------------------------------*/
/**
 * Used to register and manage callback functions with a CallbackManager
 * object. Although CallbackFunction classes can be used without this class it
 * is not recommended; CallbackHandler will ensure that the CallbackManager
 * that functions are registered with from this class are not left as
 * dangling pointers (they will be removed by this class's destructor from
 * the CallbackManager). For normal use, this class is intended to be
 * subclassed for classes that wish to register managed callbacks.
 */
class CallbackHandler
{
public:
	CallbackHandler() { }


	/**
	 * Destroys the object and removes (de-registers) all callback functions
	 * that are registered with a callback manager.
	 */
	~CallbackHandler()
	{
		for (std::vector<CallbackRegistration>::iterator it = m_registrations.begin();
				it != m_registrations.end(); ++it) {
			it->manager->remove(it->function);
		}
	}


	/**
	 * Register a callback \p function with the CallbackManager pointed to
	 * by \p manager.
	 * Function registered with the callback manager will be removed from the
	 * manager when the CallbackHandler object is destroyed or they are
	 * manually removed by calling remove_callback().
	 * \return Returns true if the function was properly registered, otherwise
	 * false. A registration can fail if \p function is already registered
	 * with the callback manager.
	 */
	bool register_callback(CallbackManager *manager, CallbackFunction function)
	{
		bool success;
		if ((success = manager->add(function)))
			m_registrations.push_back(CallbackRegistration(manager, function));
		return success;
	}


	/**
	 * Remove (de-register) a callback \p function registered with the
	 * CallbackManager pointed to by \p manager. If the function is not
	 * registered with the supplied CallbackManager no changes are made and
	 * this function \return returns false. This function can also \return
	 * return false if the callback was registered with the function manager
	 * but this handler does not know about it. Under most circumstances this
	 * function is probably not required (the callbacks are de-registered by
	 * the destructor).
	 */
	bool remove_callback(CallbackManager *manager, CallbackFunction function)
	{
		if (manager->remove(function)) {
			for (std::vector<CallbackRegistration>::iterator it = m_registrations.begin();
					it != m_registrations.end(); ++it) {
				if (it->manager == manager && it->function == function)
					return true;
			}
			// TODO: Trigger error: the function was registered with the manager
			//       but not in our registration list
		}
		return false;
	}

private:
	std::vector<CallbackRegistration> m_registrations;
};


#endif
