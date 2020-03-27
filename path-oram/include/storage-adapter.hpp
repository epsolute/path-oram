#pragma once

#include "definitions.h"

#include <fstream>
namespace PathORAM
{
	using namespace std;

	class AbsStorageAdapter
	{
		private:
		void checkCapacity(number location);
		void checkBlockSize(number dataSize);

		bytes key;

		public:
		pair<number, bytes> get(number location);
		void set(number location, pair<number, bytes> data);

		AbsStorageAdapter(number capacity, number userBlockSize, bytes key);
		virtual ~AbsStorageAdapter() = 0;

		protected:
		number capacity;
		number blockSize;
		number userBlockSize;

		virtual void setInternal(number location, bytes raw) = 0;
		virtual bytes getInternal(number location)			 = 0;
	};

	class InMemoryStorageAdapter : public AbsStorageAdapter
	{
		private:
		uchar **blocks;

		public:
		InMemoryStorageAdapter(number capacity, number userBlockSize, bytes key);
		~InMemoryStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;
	};

	class FileSystemStorageAdapter : public AbsStorageAdapter
	{
		private:
		fstream file;

		public:
		FileSystemStorageAdapter(number capacity, number userBlockSize, bytes key, string filename, bool override);
		~FileSystemStorageAdapter() final;

		protected:
		void setInternal(number location, bytes raw) final;
		bytes getInternal(number location) final;
	};
}
