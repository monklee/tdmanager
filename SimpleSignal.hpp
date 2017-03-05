/*
#ifndef SIMPLESIGNAL_HPP
#define SIMPLESIGNAL_HPP

#include <functional>
#include <map>
#include <mutex>


template <typename... Args>
class SimpleSignal
{

public:

	SimpleSignal();
	SimpleSignal(SimpleSignal const&); // Copy constructor, creates new SimpleSignal
	SimpleSignal& operator=(SimpleSignal const&); // Assignment, creates new SimpleSignal
	template<typename Member, typename... A> int connectMember(Member&&, A&& ...); // Connect a member function of a given object
	int  connect(std::function<void(Args...)> const&); // Connect a std::function
	void disconnect(int);
	void disconnectAll();
	void emit(Args...);

protected:

	typedef std::map<int, std::function<void(Args...)>> SlotContainerType;

	SlotContainerType callbacks;
	int currentId;

	std::recursive_mutex mtx;
	
};

#endif
*/

#ifndef SIMPLESIGNAL_HPP
#define SIMPLESIGNAL_HPP

#include <functional>
#include <map>
#include <mutex>

#include <iostream>


template <typename... Args>
class SimpleSignal
{

public:

	SimpleSignal() : currentId(0) {}
	
	// Copy constructor, creates new SimpleSignal
	SimpleSignal(SimpleSignal const& _signal) : currentId(0) {}
	
	// Assignment, creates new SimpleSignal
	SimpleSignal& operator=(SimpleSignal const& signal)
	{
		mtx.lock();
		disconnectAll();
		mtx.unlock();
	}
	
	// Connect a member function of a given object
	/*
	template<typename Member, typename... A>
	int connectMember(Member&& member, A&& ... args)
	{
		mtx.lock();
		//callbacks.insert(std::make_pair(++currentId, std::bind(member, args...)));
		callbacks.insert({++currentId, std::bind(member, args...)});
		mtx.unlock();
		return currentId;
	}
	*/
	
	// Connect a std::function
	int connect(std::function<void(Args...)> const& _callback)
	{
		mtx.lock();
		callbacks.insert(std::make_pair(++currentId, _callback));
		//callbacks.insert({++currentId, _callback});
		mtx.unlock();
		return currentId;
	}
	
	void disconnect(int id)
	{
		mtx.lock();
		callbacks.erase(id);
		mtx.unlock();
	}
	
	void disconnectAll()
	{
		mtx.lock();
		callbacks.clear();
		mtx.unlock();
	}
	
	void emit(Args... p)
	{
		mtx.lock();
		for (auto it : callbacks)
			it.second(p...);
		mtx.unlock();
	}

protected:

	std::map<int, std::function<void(Args...)> > callbacks;
	int currentId;

	std::recursive_mutex mtx;
	
};



#include <Wt/WApplication>
#include <string>

/*
namespace Wt
{
	class WApplication;
}
*/

template <typename... Args>
class SignalSessions
{

protected:

	struct DataElement
	{
		Wt::WApplication*            session;
		std::string                  sessionId;
		std::function<void(Args...)> callback;
	};

	std::map<int, DataElement> sessions;
	int currentId;
	std::string activeSessionId;
	std::recursive_mutex mtx;
	
	static std::map<int, int> instanceCount;

public:

	SignalSessions() : currentId(0) {}
	
	// Copy constructor, creates new SimpleSignal
	SignalSessions(SignalSessions const& _signal) : currentId(0) {}
	
	// Assignment, creates new SimpleSignal
	SignalSessions& operator=(SignalSessions const& signal)
	{
		mtx.lock();
		disconnectAll();
		mtx.unlock();
	}
	
	// Connect a member function of a given object
	/*
	template<typename Member, typename... A>
	int connectMember(Wt::WApplication* sessionPtr, Member&& member, A&& ... args)
	{
		mtx.lock();
		//sessionSlots.insert(std::make_pair(++currentId, SlotDataElement(sessionPtr, std::bind(member, args...))));

		sessions.insert( std::make_pair(++currentId, sessionPtr));
		callbacks.insert(std::make_pair(  currentId, std::bind<Member, A...>(member, args...)));

		//sessions[++currentId] = sessionPtr;
		//callbacks[currentId]      = std::bind(member, args...);

		//sessionSlots.insert(std::make_pair(++currentId, SlotDataElement(sessionPtr, std::function<void(Args...)>(member, args...))));
		mtx.unlock();
		return currentId;
	}
	*/
	
	// Connect a std::function
	int connect(Wt::WApplication* session, std::function<void(Args...)> const& _callback)
	{
		mtx.lock();
		sessions[currentId] = DataElement{session, session->sessionId(), _callback};

		/*
		if (instanceCount.count(currentId) == 0)
			instanceCount[currentId] = 1;
		else
			instanceCount[currentId]++;
		std::cout << "SignalSessions::connect(...) Session " << currentId << " " << sessions[currentId].sessionId << "  # of instances active " << instanceCount[currentId] << std::endl;
		*/

		mtx.unlock();
		return currentId++;
	}

	void disconnect(int id)
	{
		mtx.lock();

		/*
		instanceCount[id]--;
		std::cout << "SignalSessions::disconnect(...) Session " << id << " " << sessions[id].sessionId << "  # of instances left " << instanceCount[id] << std::endl;
		*/

		if (sessions[id].sessionId == activeSessionId)
			activeSessionId.clear();
		sessions.erase(id);
		mtx.unlock();
	}
	
	void disconnectAll()
	{
		mtx.lock();
		sessions.clear();
		activeSessionId.clear();
		mtx.unlock();
	}
	
	void lockFromSession(std::string _activeSessionId)
	{
		mtx.lock();
		activeSessionId = _activeSessionId;
	}
	
	bool unlockFromSession(std::string _activeSessionId)
	{
		bool retval = false;
		mtx.lock();
		if (_activeSessionId == activeSessionId)
		{
			mtx.unlock();
			activeSessionId.clear();
			retval = true;
		}
		mtx.unlock();
		return retval;
	}
	
	// Emit locks each session before emitting
	void emit(Args... p)
	{
		//std::cout << "SignalSessions::emit(...) Entry point." << std::endl;

		mtx.lock();
		//std::cout << "SignalSessions::emit(...) Mutex locked." << std::endl;

		/*
		std::cout << "SignalSessions::emit(...) ";
		for (auto session : sessions)
			std::cout << session.first << ":" << session.second.sessionId << " ";
		std::cout << std::endl;
		*/
		
		for (auto session : sessions)
		{
			//std::cout << "SignalSessions::emit(...) Sessions loop entry point. Session " << session.first << " " << session.second.sessionId << std::endl;

			if (session.second.sessionId == activeSessionId)
			{
				//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Pre callback." << std::endl;
				session.second.callback(p...);
				//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Post callback." << std::endl;
			}
			else
			{
				//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Pre session lock." << std::endl;
				Wt::WApplication::UpdateLock lock(session.second.session);
				//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Session locked." << std::endl;
				if (lock)
				{
					//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Lock verified." << std::endl;
					//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Pre callback." << std::endl;
					session.second.callback(p...);
					//std::cout << "SignalSessions::emit(...) Session " << session.first << " " << session.second.sessionId << ". Post callback." << std::endl;
				}
			}

		}

		//std::cout << "SignalSessions::emit(...) Pre mutex unlock." << std::endl;
		mtx.unlock();
		//std::cout << "SignalSessions::emit(...) Mutex unlocked." << std::endl;
	}

};


template <typename... Args>
std::map<int, int> SignalSessions<Args...>::instanceCount;

#endif
