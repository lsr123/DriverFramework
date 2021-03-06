/**********************************************************************
* Copyright (c) 2015 Mark Charlebois
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*  * Neither the name of Dronecode Project nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include "DriverFramework.hpp"
#include "SyncObj.hpp"
#include "DFList.hpp"

namespace DriverFramework
{
class WorkItems
{
public:
	static bool isValidIndex(unsigned int index);

	static WorkItems &instance()
	{
		static WorkItems *instance = nullptr;

		if (!instance) {
			instance = new WorkItems;
		}

		return *instance;
	}

	static int  getIndex(WorkCallback cb, void *arg, uint32_t delay_usec, unsigned int &index);
	static void processExpiredWorkItems(uint64_t &next);
	static int  schedule(unsigned int index);
	static void unschedule(unsigned int index);
	static void finalize();

private:
	WorkItems() {}

	virtual ~WorkItems()
	{}

	void removeItem(WorkHandle &wh);
	void addItem(WorkHandle &wh);

	// These version do not call m_lock.lock()
	int  _schedule(unsigned int index);
	void _unschedule(unsigned int index);
	void _finalize();
	void _processExpiredWorkItems(uint64_t &next);
	int  _getIndex(WorkCallback cb, void *arg, uint32_t delay_usec, unsigned int &index);
	bool _isValidIndex(unsigned int index);

	class WorkItem
	{
	public:
		WorkItem()
		{
			resetStats();
		}
		~WorkItem() {}

		void schedule();
		void updateStats(unsigned int cur_usec);
		void resetStats();
		void dumpStats();

		void set(WorkCallback callback, void *arg, uint32_t delay_usec)
		{
			m_arg = arg;
			m_queue_time = 0;
			m_callback = callback;
			m_delay_usec = delay_usec;
			m_in_use = false;

			resetStats();
		}

		void 		*m_arg;
		uint64_t	m_queue_time;
		WorkCallback	m_callback;
		uint32_t	m_delay_usec;
		//WorkHandle 	m_handle;

		// statistics
		unsigned long 	m_last;
		unsigned long 	m_min;
		unsigned long 	m_max;
		unsigned long 	m_total;
		unsigned long 	m_count;

		bool		m_in_use = false;

	};

	// The list of WorkItem only grows via push_back so the order is preserved and
	// a index from 0-n can be used to retrieve an entry via getAt to iterate through
	// the list to that point.
	// This is somewhat inefficuent but in general the depth of the list is expected
	// to be small
	bool getAt(unsigned int index, WorkItem **item)
	{
		DFManagedList<WorkItem>::Index idx = nullptr;
		idx = m_work_items.next(idx);

		for (unsigned int i = 0; i < index; ++i) {

			// if the index is invalid m_work_items.next() will return nullptr
			if (idx == nullptr) {
				return false;
			}

			idx = m_work_items.next(idx);
		}

		*item = m_work_items.get(idx);
		return true;
	}


	DFUIntList		m_work_list; 	// List of active work items
	DFManagedList<WorkItem> m_work_items;	// List of all created work items
	SyncObj			m_lock;
};

class RunStatus
{
public:
	RunStatus() {};
	~RunStatus() {};

	// check() returns true if no terminate was requested
	bool check();

	// terminate() requests the framework to terminate
	void terminate();

private:
	bool 	m_run = true;
	SyncObj	m_lock;
};

extern RunStatus *g_run_status;

};

