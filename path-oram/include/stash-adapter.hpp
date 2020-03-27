#pragma once

#include "definitions.h"

#include <iostream>
#include <unordered_map>

namespace PathORAM
{
	using namespace std;

	class AbsStashAdapter
	{
		public:
		virtual unordered_map<number, bytes> getAll() = 0;
		virtual void add(number block, bytes data)	= 0;
		virtual void update(number block, bytes data) = 0;
		virtual bytes get(number block)				 = 0;
		virtual void remove(number block)			 = 0;

		virtual ~AbsStashAdapter() = 0;

		protected:
		virtual bool exists(number block) = 0;

		friend class ORAMTest_ReadPath_Test;
		friend class ORAMTest_PutMany_Test;
	};

	class InMemoryStashAdapter : public AbsStashAdapter
	{
		private:
		unordered_map<number, bytes> stash;
		number capacity;

		void checkOverflow(number block);
		bool exists(number block);

		friend class ORAMTest_ReadPath_Test;

		public:
		InMemoryStashAdapter(number capacity);
		~InMemoryStashAdapter() final;

		unordered_map<number, bytes> getAll() final;
		void add(number block, bytes data) final;
		void update(number block, bytes data) final;
		bytes get(number block) final;
		void remove(number block) final;
	};
}
